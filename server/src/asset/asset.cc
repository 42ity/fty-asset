/*  =========================================================================
    asset - asset

    Copyright (C) 2014 - 2020 Eaton

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

#include "asset.h"
#include "asset-db-test.h"
#include "asset-db.h"
#include "asset-storage.h"
#include "asset/conversion/full-asset.h"
#include <algorithm>
#include <fty_asset_activator.h>
#include <fty_common_db_dbpath.h>
#include <functional>
#include <memory>
#include <sstream>
#include <time.h>
#include <utility>
#include <uuid/uuid.h>

#define AGENT_ASSET_ACTIVATOR "etn-licensing-credits"

namespace fty {

//============================================================================================================

template <typename T, typename T1>
bool isAnyOf(const T& val, const T1& other)
{
    return val == other;
}

template <typename T, typename T1, typename... Vals>
bool isAnyOf(const T& val, const T1& other, const Vals&... args)
{
    if (val == other) {
        return true;
    } else {
        return isAnyOf(val, args...);
    }
}

//============================================================================================================

/// generate current timestamp string in format yyyy-mm-ddThh:MM:ss+0000
static std::string generateCurrentTimestamp()
{
    static constexpr int tmpSize   = 100;
    std::time_t          timestamp = std::time(NULL);
    char                 timeString[tmpSize];
    std::strftime(timeString, tmpSize - 1, "%FT%T%z", std::localtime(&timestamp));

    return std::string(timeString);
}

/// generate asset UUID (if manufacture, model and serial are known -> EATON namespace UUID, otherwise random)
static std::string generateUUID(
    const std::string& manufacturer, const std::string& model, const std::string& serial)
{
    static std::string ns = "\x93\x3d\x6c\x80\xde\xa9\x8c\x6b\xd1\x11\x8b\x3b\x46\xa1\x81\xf1";
    std::string        uuid;

    if (!manufacturer.empty() && !model.empty() && !serial.empty()) {
        log_debug("generate full UUID");

        std::string src = ns + manufacturer + model + serial;
        // hash must be zeroed first
        unsigned char* hash = static_cast<unsigned char*>(calloc(SHA_DIGEST_LENGTH, sizeof(unsigned char)));

        SHA1(reinterpret_cast<const unsigned char *>(src.c_str()), src.length(), hash);

        hash[6] &= 0x0F;
        hash[6] |= 0x50;
        hash[8] &= 0x3F;
        hash[8] |= 0x80;

        char uuid_char[37];
        uuid_unparse_lower(hash, uuid_char);

        uuid = uuid_char;
    } else {
        uuid_t u;
        uuid_generate_random(u);

        char uuid_char[37];
        uuid_unparse_lower(u, uuid_char);

        uuid = uuid_char;
    }

    return uuid;
}

static AssetStorage& getStorage()
{
    if (g_testMode) {
        return DBTest::getInstance();
    } else {
        return DB::getInstance();
    }
}

/// get children of Asset a
std::vector<std::string> getChildren(const AssetImpl& a)
{
    return getStorage().getChildren(a);
}

//============================================================================================================

// extract asset filters from SerializationInfo structure
void operator>>=(const cxxtools::SerializationInfo& si, AssetFilters& filters)
{
    try {
        if (si.findMember("status") != NULL) {
            const cxxtools::SerializationInfo& status = si.getMember("status");

            std::vector<std::string> v;
            status >>= v;

            for (const std::string& val : v) {
                filters["status"].push_back('"' + val + '"');
            }
        }
    } catch (const std::exception& e) {
        log_error("Invalid filter for status: %s", e.what());
    }

    try {
        if (si.findMember("type") != NULL) {
            const cxxtools::SerializationInfo& type = si.getMember("type");

            std::vector<std::string> v;
            type >>= v;

            for (const std::string& val : v) {
                uint32_t typeId = getStorage().getTypeID(val);

                if (typeId) {
                    filters["id_type"].push_back(std::to_string(typeId));
                } else {
                    log_error("Invalid type filter: %s type does not exist", val.c_str());
                }
            }
        }
    } catch (std::exception& e) {
        log_error("Invalid filter type: %s", e.what());
    }

    try {
        if (si.findMember("sub_type") != NULL) {
            const cxxtools::SerializationInfo& subtype = si.getMember("sub_type");

            std::vector<std::string> v;
            subtype >>= v;

            for (const std::string& val : v) {
                uint32_t subtypeId = getStorage().getSubtypeID(val);
                if (subtypeId) {
                    filters["id_subtype"].push_back(std::to_string(subtypeId));
                } else {
                    log_error("Invalid subtype filter: %s subtype does not exist", val.c_str());
                }
            }
        }
    } catch (std::exception& e) {
        log_error("Invalid filter sub_type: %s", e.what());
    }

    try {
        if (si.findMember("priority") != NULL) {
            const cxxtools::SerializationInfo& priority = si.getMember("priority");

            std::vector<int> v;
            priority >>= v;

            for (int val : v) {
                filters["priority"].push_back(std::to_string(val));
            }
        }
    } catch (std::exception& e) {
        log_error("Invalid filter priority: %s", e.what());
    }

    try {
        if (si.findMember("parent") != NULL) {
            const cxxtools::SerializationInfo& parent = si.getMember("parent");

            std::vector<std::string> v;
            parent >>= v;

            for (const std::string& val : v) {
                uint32_t parentId = getStorage().getID(val);
                if (parentId) {
                    filters["id_parent"].push_back(std::to_string(parentId));
                } else {
                    log_error("Invalid parent filter: %s does not exist", val.c_str());
                }
            }
        }
    } catch (std::exception& e) {
        log_error("Invalid filter parent: %s", e.what());
    }
}

//============================================================================================================

AssetImpl::AssetImpl()
    : m_storage(getStorage())
{
}

AssetImpl::AssetImpl(const std::string& nameId)
    : m_storage(getStorage())
{
    m_storage.loadAsset(nameId, *this);
    m_storage.loadExtMap(*this);
    m_storage.loadLinkedAssets(*this);
}

AssetImpl::~AssetImpl()
{
}

AssetImpl::AssetImpl(const AssetImpl& a)
    : Asset(a)
    , m_storage(getStorage())
{
}

AssetImpl& AssetImpl::operator=(const AssetImpl& a)
{
    if (&a == this) {
        return *this;
    }
    Asset::operator=(a);

    return *this;
}

bool AssetImpl::hasLogicalAsset() const
{
    return getExt().find("logical_asset") != getExt().end();
}

bool AssetImpl::isVirtual() const
{
    return ((getAssetType() == TYPE_CLUSTER) || (getAssetType() == TYPE_HYPERVISOR) ||
            (getAssetType() == TYPE_VIRTUAL_MACHINE) || (getAssetType() == TYPE_STORAGE_SERVICE) ||
            (getAssetType() == TYPE_VAPP) || (getAssetType() == TYPE_CONNECTOR) ||
            (getAssetType() == TYPE_SERVER) || (getAssetType() == TYPE_PLANNER) ||
            (getAssetType() == TYPE_PLAN));
}

bool AssetImpl::hasLinkedAssets() const
{
    return m_storage.hasLinkedAssets(*this);
}

void AssetImpl::remove(bool removeLastDC)
{
    if (RC0 == getInternalName()) {
        throw std::runtime_error("cannot delete RC-0");
    }

    // TODO verify if it is necessary to prevent assets with logical_asset attribute from being deleted
    // if (hasLogicalAsset()) {
    //     throw std::runtime_error("a logical_asset (sensor) refers to it");
    // }

    if (hasLinkedAssets()) {
        throw std::runtime_error("it the source of a link");
    }

    std::vector<std::string> assetChildren = getChildren(*this);

    if (!assetChildren.empty()) {
        throw std::runtime_error("it has at least one child");
    }

    // deactivate asset
    if (getAssetStatus() == AssetStatus::Active) {
        deactivate();
    }

    m_storage.beginTransaction();
    try {
        if (isAnyOf(getAssetType(), TYPE_DATACENTER, TYPE_ROW, TYPE_ROOM, TYPE_RACK)) {
            if (!removeLastDC && m_storage.isLastDataCenter(*this)) {
                throw std::runtime_error("cannot delete last datacenter");
            }
            m_storage.removeExtMap(*this);
            m_storage.removeFromGroups(*this);
            m_storage.removeFromRelations(*this);
            m_storage.removeAsset(*this);
        } else if (isAnyOf(getAssetType(), TYPE_GROUP)) {
            m_storage.clearGroup(*this);
            m_storage.removeExtMap(*this);
            m_storage.removeAsset(*this);
        } else if (isAnyOf(getAssetType(), TYPE_DEVICE)) {
            m_storage.removeFromGroups(*this);
            m_storage.unlinkAll(*this);
            m_storage.removeFromRelations(*this);
            m_storage.removeExtMap(*this);
            m_storage.removeAsset(*this);
        } else {
            m_storage.removeExtMap(*this);
            m_storage.removeAsset(*this);
        }
    } catch (const std::exception& e) {
        m_storage.rollbackTransaction();

        // reactivate is previous status was active
        if (getAssetStatus() == AssetStatus::Active) {
            activate();
        }
        log_debug("Asset could not be removed: %s", e.what());
        throw std::runtime_error("Asset could not be removed: " + std::string(e.what()));
    }
    m_storage.commitTransaction();
}

// generate asset name
static std::string createAssetName(const std::string& type, const std::string& subtype)
{
    std::string assetName;

    timeval t;
    gettimeofday(&t, NULL);
    srand(static_cast<unsigned int>(t.tv_sec * t.tv_usec));
    // generate 8 digit random integer
    unsigned long index = static_cast<unsigned long>(rand()) % static_cast<unsigned long>(100000000);

    std::string indexStr = std::to_string(index);

    // create 8 digit index with leading zeros
    indexStr = std::string(8 - indexStr.length(), '0') + indexStr;

    if (type == fty::TYPE_DEVICE) {
        assetName = subtype + "-" + indexStr;
    } else {
        assetName = type + "-" + indexStr;
    }

    return assetName;
}

void AssetImpl::create()
{
    m_storage.beginTransaction();
    try {
        if (!g_testMode) {
            std::string iname;
            do {
                iname = createAssetName(getAssetType(), getAssetSubtype());
            } while (m_storage.getID(iname));

            setInternalName(iname);
        }

        // set creation timestamp
        setExtEntry(fty::EXT_CREATE_TS, generateCurrentTimestamp(), true);
        // set uuid
        setExtEntry(fty::EXT_UUID, generateUUID(getManufacturer(), getModel(), getSerialNo()), true);

        m_storage.insert(*this);
        m_storage.saveLinkedAssets(*this);
        m_storage.saveExtMap(*this);
    } catch (const std::exception& e) {
        m_storage.rollbackTransaction();
        throw std::runtime_error(std::string(e.what()));
    }
    m_storage.commitTransaction();
}

void AssetImpl::update()
{
    m_storage.beginTransaction();
    try {
        if (!g_testMode && !m_storage.getID(getInternalName())) {
            throw std::runtime_error("Update failed, asset does not exist.");
        }
        // set last update timestamp
        setExtEntry(fty::EXT_UPDATE_TS, generateCurrentTimestamp(), true);

        m_storage.update(*this);
        m_storage.saveLinkedAssets(*this);
        m_storage.saveExtMap(*this);
    } catch (const std::exception& e) {
        m_storage.rollbackTransaction();
        throw std::runtime_error(std::string(e.what()));
    }
    m_storage.commitTransaction();
}

void AssetImpl::restore(bool restoreLinks)
{
    m_storage.beginTransaction();
    // restore only if asset is not already in db
    if (m_storage.getID(getInternalName())) {
        throw std::runtime_error("Asset " + getInternalName() + " already exists, restore is not possible");
    }
    try {
        // set creation timestamp
        setExtEntry(fty::EXT_CREATE_TS, generateCurrentTimestamp(), true);

        m_storage.insert(*this);
        m_storage.saveExtMap(*this);
        if (restoreLinks) {
            m_storage.saveLinkedAssets(*this);
        }

    } catch (const std::exception& e) {
        m_storage.rollbackTransaction();
        log_debug("AssetImpl::save() got EXCEPTION : %s", e.what());
        throw e.what();
    }
    m_storage.commitTransaction();
}

bool AssetImpl::isActivable()
{
    if (g_testMode) {
        return true;
    }

    if (getAssetType() == TYPE_DEVICE) {
        mlm::MlmSyncClient  client(AGENT_FTY_ASSET, AGENT_ASSET_ACTIVATOR);
        fty::AssetActivator activationAccessor(client);

        // TODO remove as soon as fty::Asset activation is supported
        fty::FullAsset fa = fty::conversion::toFullAsset(*this);
        return activationAccessor.isActivable(fa);
    } else {
        return true;
    }
}

void AssetImpl::activate()
{
    if (!g_testMode) {
        if (getAssetType() == TYPE_DEVICE) {
            mlm::MlmSyncClient  client(AGENT_FTY_ASSET, AGENT_ASSET_ACTIVATOR);
            fty::AssetActivator activationAccessor(client);

            // TODO remove as soon as fty::Asset activation is supported
            fty::FullAsset fa = fty::conversion::toFullAsset(*this);

            activationAccessor.activate(fa);

            setAssetStatus(fty::AssetStatus::Active);
            m_storage.update(*this);
        } else {
            setAssetStatus(fty::AssetStatus::Active);
            m_storage.update(*this);
        }
    }
}

void AssetImpl::deactivate()
{
    if (!g_testMode) {
        if (getAssetType() == TYPE_DEVICE) {
            mlm::MlmSyncClient  client(AGENT_FTY_ASSET, AGENT_ASSET_ACTIVATOR);
            fty::AssetActivator activationAccessor(client);

            // TODO remove as soon as fty::Asset activation is supported
            fty::FullAsset fa = fty::conversion::toFullAsset(*this);

            activationAccessor.deactivate(fa);
            log_debug("Asset %s deactivated", getInternalName().c_str());

            setAssetStatus(fty::AssetStatus::Nonactive);
            m_storage.update(*this);
        } else {
            setAssetStatus(fty::AssetStatus::Nonactive);
            m_storage.update(*this);
        }
    }
}

void AssetImpl::unlinkAll()
{
    m_storage.unlinkAll(*this);
}

static std::vector<fty::Asset> buildParentsList(const std::string iname)
{
    // avoid infinite loop
    const unsigned short maxLevels = 255;

    std::vector<fty::Asset> parents;

    fty::AssetImpl a(iname);

    unsigned short level = 0;

    while ((!a.getParentIname().empty()) && (level < maxLevels)) {
        a = fty::AssetImpl(a.getParentIname());
        parents.push_back(a);

        level++;
    }

    return parents;
}

void AssetImpl::updateParentsList()
{
    m_parentsList = buildParentsList(getInternalName());
}

void AssetImpl::assetToSrr(const AssetImpl& asset, cxxtools::SerializationInfo& si)
{
    // basic
    si.addMember("id") <<= asset.getInternalName();
    si.addMember("status") <<= int(asset.getAssetStatus());
    si.addMember("type") <<= asset.getAssetType();
    si.addMember("subtype") <<= asset.getAssetSubtype();
    si.addMember("priority") <<= asset.getPriority();
    si.addMember("parent") <<= asset.getParentIname();

    // linked assets
    cxxtools::SerializationInfo& linked = si.addMember("");

    cxxtools::SerializationInfo tmpSiLinks;
    for (const auto& l : asset.getLinkedAssets()) {
        cxxtools::SerializationInfo& link = tmpSiLinks.addMember("");
        link.addMember("source") <<= l.sourceId();

        if (!l.srcOut().empty()) {
            link.addMember("src_out") <<= l.srcOut();
        }
        if (!l.destIn().empty()) {
            link.addMember("dest_in") <<= l.destIn();
        }

        link.addMember("link_type") <<= l.linkType();

        if (!l.ext().empty()) {
            cxxtools::SerializationInfo& link_ext = link.addMember("");
            cxxtools::SerializationInfo  tmp_link_ext;
            for (const auto& e : l.ext()) {
                cxxtools::SerializationInfo& link_ext_entry = tmp_link_ext.addMember(e.first);
                link_ext_entry.addMember("value") <<= e.second.getValue();
                link_ext_entry.addMember("readOnly") <<= e.second.isReadOnly();
            }
            tmp_link_ext.setCategory(cxxtools::SerializationInfo::Category::Object);
            link_ext = tmp_link_ext;
            link_ext.setName("ext");
        }

        link.setCategory(cxxtools::SerializationInfo::Category::Object);
    }
    tmpSiLinks.setCategory(cxxtools::SerializationInfo::Category::Array);
    linked = tmpSiLinks;
    linked.setName("linked");

    si.addMember("tag") <<= asset.getAssetTag();
    si.addMember("id_secondary") <<= asset.getSecondaryID();

    // ext
    cxxtools::SerializationInfo& ext = si.addMember("");

    cxxtools::SerializationInfo tmpSiExt;
    for (const auto& e : asset.getExt()) {
        cxxtools::SerializationInfo& entry = tmpSiExt.addMember(e.first);
        entry.addMember("value") <<= e.second.getValue();
        entry.addMember("readOnly") <<= e.second.isReadOnly();
    }
    tmpSiExt.setCategory(cxxtools::SerializationInfo::Category::Object);
    ext = tmpSiExt;
    ext.setName("ext");
}

void AssetImpl::srrToAsset(const cxxtools::SerializationInfo& si, AssetImpl& asset)
{
    int         tmpInt = 0;
    std::string tmpString;

    // uuid
    si.getMember("id") >>= tmpString;
    asset.setInternalName(tmpString);

    // status
    si.getMember("status") >>= tmpInt;
    asset.setAssetStatus(AssetStatus(tmpInt));

    // type
    si.getMember("type") >>= tmpString;
    asset.setAssetType(tmpString);

    // subtype
    si.getMember("subtype") >>= tmpString;
    asset.setAssetSubtype(tmpString);

    // priority
    si.getMember("priority") >>= tmpInt;
    asset.setPriority(tmpInt);

    // parend id
    si.getMember("parent") >>= tmpString;
    asset.setParentIname(tmpString);

    // linked assets
    const cxxtools::SerializationInfo linked = si.getMember("linked");
    for (const auto& link_si : linked) {
        std::string key = link_si.name();
        std::string sourceId, srcOut, destIn;
        int linkType = 0;

        link_si.getMember("source") >>= sourceId;
        if(link_si.findMember("src_out") != NULL) {
            link_si.getMember("src_out") >>= srcOut;
        }
        if(link_si.findMember("dest_in") != NULL) {
            link_si.getMember("dest_in") >>= destIn;
        }
        link_si.getMember("link_type") >>= linkType;

        AssetLink::ExtMap linkAttributes;

        if(link_si.findMember("ext") != NULL) {
            // link ext map
            const cxxtools::SerializationInfo link_ext = link_si.getMember("ext");
            for (const auto& link_ext_si : link_ext) {
                std::string ext_key = link_ext_si.name();
                std::string val;
                bool        readOnly = false;
                link_ext_si.getMember("value") >>= val;
                link_ext_si.getMember("readOnly") >>= readOnly;

                ExtMapElement extElement(val, readOnly);

                linkAttributes[ext_key] = extElement;
            }
        }

        asset.addLink(sourceId, srcOut, destIn, linkType, linkAttributes);
    }

    // asset tag
    si.getMember("tag") >>= tmpString;
    asset.setAssetTag(tmpString);

    // id secondary
    si.getMember("id_secondary") >>= tmpString;
    asset.setSecondaryID(tmpString);

    // ext map
    const cxxtools::SerializationInfo ext = si.getMember("ext");

    for (const auto& siExt : ext) {
        std::string key = siExt.name();
        std::string val;
        bool        readOnly = false;
        si.getMember("value") >>= val;
        si.getMember("readOnly") >>= readOnly;

        asset.setExtEntry(key, val, readOnly);
    }
}

std::vector<std::string> AssetImpl::list(const AssetFilters& filters)
{
    return getStorage().listAssets(filters);
}

std::vector<std::string> AssetImpl::listAll()
{
    return getStorage().listAllAssets();
}

void AssetImpl::load()
{
    m_storage.loadAsset(getInternalName(), *this);
    m_storage.loadExtMap(*this);
    m_storage.loadLinkedAssets(*this);
}

static void addSubTree(const std::string& internalName, std::vector<AssetImpl>& toDel)
{
    std::vector<std::pair<AssetImpl, int>> stack;

    AssetImpl a(internalName);

    AssetImpl& ref = a;

    bool         end  = false;
    unsigned int next = 0;

    while (!end) {
        std::vector<std::string> children = getChildren(ref);

        if (next < children.size()) {
            stack.push_back(std::make_pair(ref, next + 1));

            ref  = children[next];
            next = 0;

            auto found = std::find_if(toDel.begin(), toDel.end(), [&](const AssetImpl& assetImpl) {
                return assetImpl.getInternalName() == ref.getInternalName();
            });
            if (found == toDel.end()) {
                toDel.push_back(ref);
            }
        } else {
            if (stack.empty()) {
                end = true;
            } else {
                ref  = stack.back().first;
                next = static_cast<unsigned int>(stack.back().second);
                stack.pop_back();
            }
        }
    }
}

DeleteStatus AssetImpl::deleteList(const std::vector<std::string>& assets, bool recursive, bool deleteVirtualAssets, bool removeLastDC)
{
    std::vector<AssetImpl> toDel;

    DeleteStatus deleted;

    for (const std::string& iname : assets) {
        try {
            AssetImpl a(iname);
            if(a.isVirtual() && !deleteVirtualAssets) {
                continue;
            }
            toDel.push_back(a);

            if (recursive) {
                addSubTree(iname, toDel);
            }
        } catch (std::exception& e) {
            log_warning("Error while loading asset %s. %s", iname.c_str(), e.what());
        }
    }

    auto isAnyParent = [&](const AssetImpl& l, const AssetImpl& r) {
        std::vector<std::pair<std::string, int>> lTree;
        auto                                     ptr = l;
        // path to root of L
        int distL = 0;
        while (ptr.getParentIname() != "") {
            lTree.push_back(std::make_pair(ptr.getInternalName(), distL));
            std::string parentIname = ptr.getParentIname();
            ptr                     = AssetImpl(parentIname);
            distL++;
        }

        int distR = 0;
        ptr       = r;
        while (ptr.getParentIname() != "") {
            // look for LCA, if exists
            auto found = std::find_if(lTree.begin(), lTree.end(), [&](const std::pair<std::string, int>& p) {
                return p.first == ptr.getInternalName();
            });
            if (found != lTree.end()) {
                distL = found->second; // distance of L from LCA
                break;
            }

            ptr = AssetImpl(ptr.getParentIname());
            distR++;
        }
        // farthest node from LCA gets deleted first
        return distL > distR;
    };

    // sort by deletion order
    std::sort(toDel.begin(), toDel.end(), [&](const AssetImpl& l, const AssetImpl& r) {
        return isAnyParent(l, r);
    });

    // remove all links
    for (auto& a : toDel) {
        a.unlinkAll();
    }

    for (auto& d : toDel) {
        // after deleting assets, children list may change -> reload
        d.load();

        try {
            d.remove(removeLastDC);
            deleted.push_back({d, "OK"});
        } catch (std::exception& e) {
            log_error("Asset could not be removed: %s", e.what());
            deleted.push_back({d, "Asset could not be removed: " + std::string(e.what())});
        }
    }

    return deleted;
}

DeleteStatus AssetImpl::deleteAll(bool deleteVirtualAsset)
{
    // get list of all assets (including last datacenter)
    return deleteList(listAll(), false, deleteVirtualAsset, true);
}

/// get internal name from UUID
std::string AssetImpl::getInameFromUuid(const std::string& uuid)
{
    return getStorage().inameByUuid(uuid);
}

} // namespace fty
