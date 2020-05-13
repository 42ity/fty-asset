/*  =========================================================================
    fty_asset_manipulation - Helper functions to perform asset manipulation

    Copyright (C) 2016 - 2020 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

/*
@header
    fty_asset_manipulation - Helper functions to perform asset manipulation
@discuss
@end
*/

#define AGENT_ASSET_ACTIVATOR   "etn-licensing-credits"

#define TIME_STR_SIZE 100

#include "dbhelpers.h"
#include "fty_asset_manipulation.h"
#include <stdlib.h>

#include <string>

#include <tntdb/row.h>
#include <tntdb/connect.h>

#include <openssl/ssl.h>
#include <uuid/uuid.h>

#include <ftyproto.h>
#include <zhash.h>

// needed for UUID gen
#define EATON_NS "\x93\x3d\x6c\x80\xde\xa9\x8c\x6b\xd1\x11\x8b\x3b\x46\xa1\x81\xf1"

/// create asset name from type/subtype and integer ID
static std::string createAssetName(const std::string& type, const std::string& subtype, long index)
{
    std::string assetName;

    if(type == fty::TYPE_DEVICE) {
        assetName = subtype + "-" + std::to_string(index);
    } else {
        assetName = type + "-" + std::to_string(index);
    }

    return assetName;
}

// TODO remove as soon as fty::Asset activation is supported
/// convert fty::Asset to fty::FullAsset
static fty::FullAsset assetToFullAsset(const fty::Asset& asset)
{
    fty::FullAsset::HashMap auxMap; // does not exist in new Asset implementation
    fty::FullAsset::HashMap extMap;
   
    for (const auto& element : asset.getExt())
    {
        // FullAsset hash map has no readOnly parameter
        extMap[element.first] = element.second.first;
    }

    fty::FullAsset fa(
        asset.getInternalName(),
        fty::assetStatusToString(asset.getAssetStatus()),
        asset.getAssetType(),
        asset.getAssetSubtype(),
        asset.getExtEntry(fty::EXT_NAME), // asset name is stored in ext structure
        asset.getParentIname(),
        asset.getPriority(),
        auxMap,
        extMap
    );

    return fa;
}

/// test if asset activation is possible
static bool canAssetBeActivated(const fty::Asset& asset)
{
    log_debug("[asset_manipulation] : checking asset activation");
    mlm::MlmSyncClient client(AGENT_FTY_ASSET, AGENT_ASSET_ACTIVATOR);
    fty::AssetActivator activationAccessor(client);
    
    // TODO remove as soon as fty::Asset activation is supported
    fty::FullAsset fa = assetToFullAsset(asset);

    return activationAccessor.isActivable(fa);
}

/// perform asset activation
static void activateAsset(const fty::Asset& asset)
{
    mlm::MlmSyncClient client(AGENT_FTY_ASSET, AGENT_ASSET_ACTIVATOR);
    fty::AssetActivator activationAccessor(client);
    
    // TODO remove as soon as fty::Asset activation is supported
    fty::FullAsset fa = assetToFullAsset(asset);

    activationAccessor.activate(fa);
    log_debug("[asset_manipulation] : asset activated");
}

/// generate current timestamp string in format yyyy-mm-ddThh:MM:ss+0000
std::string generateCurrentTimestamp()
{
    std::time_t timestamp = std::time(NULL);
    char timeString[TIME_STR_SIZE];
    std::strftime(timeString, TIME_STR_SIZE-1, "%FT%T%z", std::localtime(&timestamp));

    return std::string(timeString);
}

/// generate asset UUID (if manufacture, model and serial are known -> EATON namespace UUID, otherwise random)
std::string generateUUID(const std::string& manufacturer, const std::string& model, const std::string& serial)
{
    std::string uuid;

    if(!manufacturer.empty() && !model.empty() && !serial.empty())
    {
        log_debug("[asset_manufacturer] generate full UUID");

        std::string src = std::string(EATON_NS) + manufacturer + model + serial;
        // hash must be zeroed first
        unsigned char *hash = (unsigned char *)calloc (SHA_DIGEST_LENGTH, sizeof(unsigned char));

        SHA1((unsigned char*)src.c_str(), src.length(), hash);

        hash[6] &= 0x0F;
        hash[6] |= 0x50;
        hash[8] &= 0x3F;
        hash[8] |= 0x80;

        char uuid_char[37];
        uuid_unparse_lower(hash, uuid_char);

        uuid = uuid_char;
    }
    else
    {
        log_debug("[asset_manufacturer] generate random UUID");

        uuid_t u;
        uuid_generate_random(u);

        char uuid_char[37];
        uuid_unparse_lower(u, uuid_char);

        uuid = uuid_char;
    }
    
    return uuid;
}

fty::Asset createAsset(const fty::Asset& asset, bool tryActivate, bool test)
{
    fty::Asset createdAsset;
    createdAsset = asset;

    if(test)
    {
        log_debug("[asset_manipulation] : createAsset test mode");
        test_map_asset_state[asset.getInternalName()] = fty::assetStatusToString(asset.getAssetStatus());
    }
    else
    {
        log_debug("[asset_manipulation] : creating new asset");
        // check if asset can be activated (if needed)
        if((createdAsset.getAssetStatus() == fty::AssetStatus::Active) && !canAssetBeActivated(createdAsset))
        {
            if(tryActivate) // if activation fails and tryActivate is true -> create anyway wih status "non active"
            {
                log_warning("Licensing limitation hit - setting asset status to non active.");
                createdAsset.setAssetStatus(fty::AssetStatus::Nonactive);
            }
            else            // do not create asset in db
            {
                throw std::runtime_error("Licensing limitation hit - maximum amount of active power devices allowed in license reached.");
            }
        }

        // datacenter parent ID must be null
        if(createdAsset.getAssetType() == fty::TYPE_DATACENTER && !createdAsset.getParentIname().empty())
        {
            throw std::runtime_error("Invalid parent ID for asset type DataCenter");
        }

        // first insertion with random unique name, to get the int id later
        // (@ is prohibited in name => name-@@-123 is unique)
        createdAsset.setInternalName(asset.getInternalName() + std::string("-@@-") + std::to_string(rand()));

        // TODO get id of last inserted element (race condition may occur, use a select statement instead)
        long assetIndex = insertAssetToDB(createdAsset);

        // create and update asset internal name (<type/subtype>-<id>)
        std::string assetName = createAssetName(asset.getAssetType(), asset.getAssetSubtype(), assetIndex);
        createdAsset.setInternalName(assetName);

        updateAssetProperty("name", assetName, "id_asset_element", assetIndex);

        // add creation timestamp
        createdAsset.setExtEntry(fty::EXT_CREATE_TS, generateCurrentTimestamp(), true);

        // add UUID
        createdAsset.setExtEntry(fty::EXT_UUID, generateUUID(asset.getManufacturer(), asset.getModel(), asset.getSerialNo()), true);

        // update external properties
        log_debug ("Processing inventory for asset %s", createdAsset.getInternalName().c_str());
        updateAssetExtProperties(createdAsset);

        // activate asset
        if(createdAsset.getAssetStatus() == fty::AssetStatus::Active)
        {
            activateAsset(createdAsset);
        }
    }

    log_debug("[asset_manipulation] : created new asset %s", createdAsset.getInternalName().c_str());
    return createdAsset;
}

fty::Asset updateAsset(const fty::Asset& asset, bool tryActivate, bool test)
{
    fty::Asset updatedAsset;
    updatedAsset = asset;

    if(test)
    {
        log_debug ("[updateAsset]: runs in test mode");
        test_map_asset_state[asset.getInternalName()] = fty::assetStatusToString(asset.getAssetStatus());
    }
    else
    {
        // datacenter parent ID must be null
        if(updatedAsset.getAssetType() == fty::TYPE_DATACENTER && !updatedAsset.getParentIname().empty())
        {
            throw std::runtime_error("Invalid parent ID for asset type DataCenter");
        }

        // get current asset status (as stored in db)
        std::string currentStatus;
        currentStatus = selectAssetProperty<std::string>("status", "name", updatedAsset.getInternalName());

        // activate if needed
        // (old status is non active and requested status is active)
        bool needActivation = ((updatedAsset.getAssetStatus() == fty::AssetStatus::Active) && (currentStatus == fty::assetStatusToString(fty::AssetStatus::Nonactive)));
        if(needActivation)
        {
            // check if asset can be activated
            if(!canAssetBeActivated(updatedAsset))
            {
                if(tryActivate) // if activation fails and tryActivate is true -> update anyway wih status "non active"
                {
                    log_warning("Licensing limitation hit - setting asset status to non active.");
                    updatedAsset.setAssetStatus(fty::AssetStatus::Nonactive);
                }
                else            // do not update asset in db
                {
                    throw std::runtime_error("Licensing limitation hit - maximum amount of active power devices allowed in license reached.");
                }
            }
        }

        // perform update
        updateAssetToDB(updatedAsset);

        if(needActivation)
        {
            activateAsset(updatedAsset);
        }

        log_debug ("Processing inventory for asset %s", updatedAsset.getInternalName().c_str());
        updateAssetExtProperties(updatedAsset);
    }
    
    log_debug("[asset_manipulation] : updated asset %s", asset.getInternalName().c_str());
    return updatedAsset;
}

fty::Asset getAsset(const std::string& assetInternalName, bool test)
{
    fty::Asset a;

    if(test)
    {
        log_debug ("[asset_manipulation]: getAsset test mode");
        a.setInternalName(assetInternalName);
    }
    else
    {
        a = getAssetFromDB(assetInternalName);
    }

    return a;
}

std::vector<fty::Asset> listAssets(bool test)
{
    std::vector<fty::Asset> v;

    if(test)
    {
        log_debug ("[asset_manipulation]: listAssets test mode");

        fty::Asset a;
        
        //TODO improve test
        v.push_back(a);
        v.push_back(a);
        v.push_back(a);
        v.push_back(a);
    }
    else
    {
        v = getAllAssetsFromDB();
    }
    return v;
}

////////////////////////////////////////////////////////////////////////////////

// for fty-proto conversion helpers
static fty::Asset::ExtMap zhashToExtMap(zhash_t *hash, bool readOnly)
{
    fty::Asset::ExtMap map;

    for (auto* item = zhash_first(hash); item; item = zhash_next(hash))
    {
        map.emplace(zhash_cursor(hash), std::make_pair(static_cast<const char *>(item), readOnly));
    }

    return map;
}


static zhash_t* extMapToZhash(const fty::Asset::ExtMap &map)
{
    zhash_t *hash = zhash_new ();
    for (const auto& i :map) {
        zhash_insert (hash, i.first.c_str (), const_cast<void *>(reinterpret_cast<const void *>(i.second.first.c_str())));
    }

    return hash;
}

// fty-proto/Asset conversion
fty_proto_t * assetToFtyProto(const fty::Asset& asset, const std::string& operation, bool test)
{
    fty_proto_t *proto = fty_proto_new(FTY_PROTO_ASSET);

    // no need to free, as fty_proto_set_aux transfers ownership to caller
    zhash_t *aux = zhash_new();

    std::string priority = std::to_string(asset.getPriority());
    zhash_insert(aux, "priority", const_cast<void *>(reinterpret_cast<const void *>(priority.c_str())));
    zhash_insert(aux, "type", const_cast<void *>(reinterpret_cast<const void *>(asset.getAssetType().c_str())));
    zhash_insert(aux, "subtype", const_cast<void *>(reinterpret_cast<const void *>(asset.getAssetSubtype().c_str())));
    if (test)
    {
        zhash_insert(aux, "parent", const_cast<void *>(reinterpret_cast<const void *>("0")));
    }
    else
    {
        std::string parentIname = asset.getInternalName();
        if(!parentIname.empty())
        {
            std::string parentId = std::to_string(selectAssetProperty<int>("id_parent", "name", asset.getInternalName()));
            zhash_insert(aux, "parent", const_cast<void *>(reinterpret_cast<const void *>(parentId.c_str())));
        }
        else
        {
            zhash_insert(aux, "parent", const_cast<void *>(reinterpret_cast<const void *>("0")));
        }
    }
    zhash_insert(aux, "status", const_cast<void *>(reinterpret_cast<const void *>(assetStatusToString(asset.getAssetStatus()).c_str())));

    // no need to free, as fty_proto_set_ext transfers ownership to caller
    zhash_t *ext = extMapToZhash(asset.getExt());

    fty_proto_set_aux(proto, &aux);
    fty_proto_set_name(proto, "%s", asset.getInternalName().c_str());
    fty_proto_set_operation(proto, "%s", operation.c_str());
    fty_proto_set_ext(proto, &ext);

    return proto;
}

fty::Asset ftyProtoToAsset(fty_proto_t * proto, bool extAttributeReadOnly, bool test)
{
    if (fty_proto_id(proto) != FTY_PROTO_ASSET)
    {
        throw std::invalid_argument("Wrong message type");
    }

    fty::Asset asset;
    asset.setInternalName(fty_proto_name(proto));
    asset.setAssetStatus(fty::stringToAssetStatus(fty_proto_aux_string(proto, "status", "active")));
    asset.setAssetType(fty_proto_aux_string(proto, "type", ""));
    asset.setAssetSubtype(fty_proto_aux_string(proto, "subtype", ""));
    if(test)
    {
        asset.setParentIname("");
    }
    else
    {
        int parentId = fty_proto_aux_number(proto, "parent", 0);
        if(parentId != 0)
        {
            asset.setParentIname(selectAssetProperty<std::string>("name", "id_asset_element", parentId));
        }
        else
        {
            asset.setParentIname("");
        }
    }
    asset.setPriority(fty_proto_aux_number(proto, "priority", 5));

    zhash_t *ext = fty_proto_ext(proto);
    asset.setExt(zhashToExtMap(ext, extAttributeReadOnly));

    return asset;
}
