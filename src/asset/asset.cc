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
#include "conversion/full-asset.h"
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

//============================================================================================================

AssetImpl::AssetImpl()
    : m_db(DB::getInstance())
{
}

AssetImpl::AssetImpl(const std::string& nameId, bool loadLinks)
    : m_db(DB::getInstance())
{
    m_db.loadAsset(nameId, *this);
    m_db.loadExtMap(*this);
    m_db.loadChildren(*this);
    if (loadLinks) {
        m_db.loadLinkedAssets(*this);
    }
}

AssetImpl::~AssetImpl()
{
}

AssetImpl::AssetImpl(const AssetImpl& a)
    : Asset(a)
    , m_db(DB::getInstance())
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
    return m_db.hasLinkedAssets(*this);
}

void AssetImpl::remove(bool recursive, bool removeLastDC)
{
    if (RC0 == getInternalName()) {
        throw std::runtime_error("Prevented deleting RC-0");
    }

    if (hasLogicalAsset()) {
        throw std::runtime_error(TRANSLATE_ME("a logical_asset (sensor) refers to it"));
    }

    if (hasLinkedAssets()) {
        throw std::runtime_error(TRANSLATE_ME("can't delete asset because it has other assets connected"));
    }

    if (!recursive && !getChildren().empty()) {
        throw std::runtime_error(TRANSLATE_ME("can't delete the asset because it has at least one child"));
    }

    if (recursive) {
        for (const std::string& id : getChildren()) {
            AssetImpl asset(id);
            asset.remove(recursive);
        }
    }

    m_db.beginTransaction();
    try {
        if (isAnyOf(getAssetType(), TYPE_DATACENTER, TYPE_ROW, TYPE_ROOM, TYPE_RACK)) {
            if (!removeLastDC && m_db.isLastDataCenter(*this)) {
                throw std::runtime_error(TRANSLATE_ME("will not allow last datacenter to be deleted"));
            }
            m_db.removeExtMap(*this);
            m_db.removeFromGroups(*this);
            m_db.removeFromRelations(*this);
            m_db.removeAsset(*this);
        } else if (isAnyOf(getAssetType(), TYPE_GROUP)) {
            m_db.clearGroup(*this);
            m_db.removeExtMap(*this);
            m_db.removeAsset(*this);
        } else if (isAnyOf(getAssetType(), TYPE_DEVICE)) {
            m_db.removeFromGroups(*this);
            m_db.unlinkAll(*this);
            m_db.removeFromRelations(*this);
            m_db.removeExtMap(*this);
            m_db.removeAsset(*this);
        } else {
            m_db.removeExtMap(*this);
            m_db.removeAsset(*this);
        }
    } catch (const std::exception& e) {
        m_db.rollbackTransaction();
        throw std::runtime_error(e.what());
    }
    m_db.commitTransaction();
}

void AssetImpl::save(bool saveLinks)
{
    m_db.beginTransaction();
    try {
        if (getId()) {
            m_db.update(*this);
        } else { // create
            // set creation timestamp
            setExtEntry(fty::EXT_CREATE_TS, generateCurrentTimestamp(), true);
            // set uuid
            setExtEntry(fty::EXT_UUID, generateUUID(getManufacturer(), getModel(), getSerialNo()), true);

            m_db.insert(*this);
        }
        if (saveLinks) {
            m_db.saveLinkedAssets(*this);
        }
        m_db.saveExtMap(*this);
    } catch (const std::exception& e) {
        m_db.rollbackTransaction();
        throw e.what();
    }
    m_db.commitTransaction();
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
        m_db.update(*this);
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
        m_db.update(*this);
    }
}

void AssetImpl::linkTo(const std::string& src)
{
    try {
        AssetImpl s(src);
        m_db.link(s, *this);
    } catch (std::exception& ex) {
        log_error("%s", ex.what());
    }
    m_db.loadLinkedAssets(*this);
}

void AssetImpl::unlinkFrom(const std::string& src)
{
    AssetImpl s(src);
    m_db.unlink(s, *this);

    m_db.loadLinkedAssets(*this);
}

void AssetImpl::unlinkAll()
{
    m_db.unlinkAll(*this);
}

std::vector<std::string> AssetImpl::list()
{
    return DB::getInstance().listAllAssets();
}

void AssetImpl::load(bool loadLinks)
{
    m_db.loadAsset(getInternalName(), *this);
    m_db.loadExtMap(*this);
    m_db.loadChildren(*this);
    if (loadLinks) {
        m_db.loadLinkedAssets(*this);
    }
}

// static void getChildrenList(const std::string& iname, std::vector<std::string>& tree, int level)
// {
//     std::cerr << "########################### " << level << std::endl;
//     AssetImpl a(iname);
//     for (const auto& child : a.getChildren()) {
//         tree.push_back(child);
//         getChildrenList(child, tree, level + 1);
//     }
// }

void AssetImpl::deleteList(const std::vector<std::string>& assets)
{
    std::vector<AssetImpl>   toDel;
    std::vector<std::string> errors;

    for (const std::string& iname : assets) {
        std::vector<std::pair<AssetImpl, int>> stack;
        std::vector<std::string>               childrenList;

        AssetImpl a(iname);
        toDel.push_back(a);

        // get tree of children recursively
        // getChildrenList(iname, childrenList, 0);

        AssetImpl& ref = a;
        childrenList.push_back(ref.getInternalName());

        bool         end  = false;
        unsigned int next = 0;

        while (!end) {
            if (next < ref.getChildren().size()) {
                stack.push_back(std::make_pair(ref, next + 1));

                ref  = ref.getChildren()[next];
                next = 0;

                childrenList.push_back(ref.getInternalName());
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

        // remove assets already in the list
        for (const std::string& iname : assets) {
            childrenList.erase(
                std::remove(childrenList.begin(), childrenList.end(), iname), childrenList.end());
        }

        // get linked assets
        std::vector<std::string> linkedAssets = a.getLinkedAssets();

        // remove assets already in the list
        for (const std::string& iname : assets) {
            linkedAssets.erase(
                std::remove(linkedAssets.begin(), linkedAssets.end(), iname), linkedAssets.end());
        }

        if (!childrenList.empty() || !linkedAssets.empty()) {
            errors.push_back(iname);
        }
    }

    // auto isLinked = [&](const Asset& l, const Asset& r) {
    //     // check if l is linked to r (l < r)
    //     auto linksL    = l.getLinkedAssets();
    //     auto isLinkedL = std::find(linksL.begin(), linksL.end(), r.getInternalName());
    //     if (isLinkedL != linksL.end()) {
    //         return false;
    //     }
    //     // check if r is linked to l (l > r)
    //     auto linksR    = r.getLinkedAssets();
    //     auto isLinkedR = std::find(linksR.begin(), linksR.end(), l.getInternalName());
    //     if (isLinkedR != linksR.end()) {
    //         return true;
    //     }
    //     return false;
    // };

    for (auto& a : toDel) {
        a.unlinkAll();
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
        return isAnyParent(l, r) /*  || isLinked(l, r) */;
    });

    for (auto& d : toDel) {
        // after deleting assets, children list may change -> reload
        d.load();
        auto it = std::find_if(errors.begin(), errors.end(), [&](const std::string& iname) {
            return d.getInternalName() == iname;
        });

        if (it != errors.end()) {
            log_error("Asset %s cannot be deleted", it->c_str());
        } else {
            d.deactivate();
            // non recursive, force removal of last DC
            log_debug("Deleting asset %s", d.getInternalName().c_str());
            d.remove(false, true);
        }
    }
} // namespace fty


void AssetImpl::deleteAll()
{
    // get list of all assets
    deleteList(list());
}

} // namespace fty
