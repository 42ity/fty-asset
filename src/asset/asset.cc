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
#include "include/asset/conversion/full-asset.h"
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
        unsigned char* hash = (unsigned char*)calloc(SHA_DIGEST_LENGTH, sizeof(unsigned char));

        SHA1((unsigned char*)src.c_str(), src.length(), hash);

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

AssetImpl::AssetImpl()
    : m_storage(getStorage())
{
}

AssetImpl::AssetImpl(const std::string& nameId, bool loadLinks)
    : m_storage(getStorage())
{
    m_storage.loadAsset(nameId, *this);
    m_storage.loadExtMap(*this);
    if (loadLinks) {
        m_storage.loadLinkedAssets(*this);
    }
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

bool AssetImpl::hasLinkedAssets() const
{
    return m_storage.hasLinkedAssets(*this);
}

void AssetImpl::remove(bool removeLastDC)
{
    if (RC0 == getInternalName()) {
        throw std::runtime_error("cannot delete RC-0");
    }

    if (hasLogicalAsset()) {
        throw std::runtime_error("a logical_asset (sensor) refers to it");
    }

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

void AssetImpl::create()
{
    m_storage.beginTransaction();
    try {
        if (!g_testMode && m_storage.getID(getInternalName())) {
            throw(std::runtime_error("Create failed, asset name already exists"));
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

    mlm::MlmSyncClient  client(AGENT_FTY_ASSET, AGENT_ASSET_ACTIVATOR);
    fty::AssetActivator activationAccessor(client);

    // TODO remove as soon as fty::Asset activation is supported
    fty::FullAsset fa = fty::conversion::toFullAsset(*this);

    return activationAccessor.isActivable(fa);
}

void AssetImpl::activate()
{
    if (!g_testMode) {
        mlm::MlmSyncClient  client(AGENT_FTY_ASSET, AGENT_ASSET_ACTIVATOR);
        fty::AssetActivator activationAccessor(client);

        // TODO remove as soon as fty::Asset activation is supported
        fty::FullAsset fa = fty::conversion::toFullAsset(*this);

        activationAccessor.activate(fa);

        setAssetStatus(fty::AssetStatus::Active);
        m_storage.update(*this);
    }
}

void AssetImpl::deactivate()
{
    if (!g_testMode) {
        mlm::MlmSyncClient  client(AGENT_FTY_ASSET, AGENT_ASSET_ACTIVATOR);
        fty::AssetActivator activationAccessor(client);

        // TODO remove as soon as fty::Asset activation is supported
        fty::FullAsset fa = fty::conversion::toFullAsset(*this);

        activationAccessor.deactivate(fa);
        log_debug("Asset %s deactivated", getInternalName().c_str());

        setAssetStatus(fty::AssetStatus::Nonactive);
        m_storage.update(*this);
    }
}

void AssetImpl::linkTo(
    const std::string& src, const std::string& srcOut, const std::string& destIn, int linkType)
{
    try {
        AssetImpl s(src);
        m_storage.link(s, srcOut, *this, destIn, linkType);
    } catch (std::exception& ex) {
        log_error("%s", ex.what());
    }
    m_storage.loadLinkedAssets(*this);
}

void AssetImpl::unlinkFrom(
    const std::string& src, const std::string& srcOut, const std::string& destIn, int linkType)
{
    AssetImpl s(src);
    m_storage.unlink(s, srcOut, *this, destIn, linkType);

    m_storage.loadLinkedAssets(*this);
}

void AssetImpl::unlinkAll()
{
    m_storage.unlinkAll(*this);
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
    si.addMember("linked") <<= asset.getLinkedAssets();

    // ext
    cxxtools::SerializationInfo& ext = si.addMember("");

    cxxtools::SerializationInfo data;
    for (const auto& e : asset.getExt()) {
        cxxtools::SerializationInfo& entry = data.addMember(e.first);
        entry <<= e.second.first;
    }
    data.setCategory(cxxtools::SerializationInfo::Category::Object);
    ext = data;
    ext.setName("ext");
}

void AssetImpl::srrToAsset(const cxxtools::SerializationInfo& si, AssetImpl& asset)
{
    int         tmpInt;
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
    std::vector<AssetLink> tmpVector;
    si.getMember("linked") >>= tmpVector;
    asset.setLinkedAssets(tmpVector);

    // ext map
    const cxxtools::SerializationInfo ext = si.getMember("ext");
    for (const auto& si : ext) {
        std::string key = si.name();
        std::string val;
        si >>= val;

        asset.setExtEntry(key, val);
    }
}

std::vector<std::string> AssetImpl::list()
{
    return getStorage().listAllAssets();
}

void AssetImpl::load()
{
    m_storage.loadAsset(getInternalName(), *this);
    m_storage.loadExtMap(*this);
    m_storage.loadLinkedAssets(*this);
}

AssetImpl::DeleteStatus AssetImpl::deleteAsset(const std::string& internalName, bool recursive)
{
    std::vector<std::string> toDel;

    if (getStorage().getID(internalName) == 0) {
        DeleteStatus deleted;
        deleted.push_back({internalName, "Asset does not exist"});
        return deleted;
    }

    toDel.push_back(internalName);

    // delete whole sub-tree
    if (recursive) {
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

                toDel.push_back(ref.getInternalName());
            } else {
                if (stack.empty()) {
                    end = true;
                } else {
                    ref  = stack.back().first;
                    next = stack.back().second;
                    stack.pop_back();
                }
            }
        }
    }

    return deleteList(toDel);
}

AssetImpl::DeleteStatus AssetImpl::deleteList(const std::vector<std::string>& assets, bool removeLastDC)
{
    std::vector<AssetImpl> toDel;

    DeleteStatus deleted;

    for (const std::string& iname : assets) {
        try {
            AssetImpl a(iname);
            toDel.push_back(a);
        } catch (std::exception& e) {
            log_error("Error while loading asset %s. %s", iname.c_str(), e.what());
            deleted.push_back({iname, "Error while loading asset: " + std::string(e.what())});
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
            deleted.push_back({d.getInternalName(), "OK"});
        } catch (std::exception& e) {
            log_error("Asset could not be removed: %s", e.what());
            deleted.push_back({d.getInternalName(), "Asset could not be removed: " + std::string(e.what())});
        }
    }

    return deleted;
}

AssetImpl::DeleteStatus AssetImpl::deleteAll()
{
    // get list of all assets (including last datacenter)
    return deleteList(list(), true);
}

/// get internal name from UUID
std::string AssetImpl::getInameFromUuid(const std::string& uuid)
{
    return getStorage().inameByUuid(uuid);
}

} // namespace fty
