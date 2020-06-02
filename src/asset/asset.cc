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
#include "conversion/json.h"
#include <algorithm>
#include <fty_asset_activator.h>
#include <fty_common_db_dbpath.h>
#include <memory>
#include <sstream>
#include <time.h>
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
    : m_db(g_testMode ? new DBTest : new DB)
{
    m_db->init();
}

AssetImpl::AssetImpl(const std::string& nameId)
    : m_db(g_testMode ? new DBTest : new DB)
{
    m_db->init();

    m_db->loadAsset(nameId, *this);
    m_db->loadExtMap(*this);
    m_db->loadChildren(*this);
    m_db->loadLinkedAssets(*this);
}

AssetImpl::~AssetImpl()
{
}

AssetImpl::AssetImpl(const AssetImpl& a)
    : Asset(a)
    , m_db(g_testMode ? new DBTest : new DB)
{
    m_db->init();
}

AssetImpl& AssetImpl::operator=(const AssetImpl& a)
{
    return *this;
}

bool AssetImpl::hasLogicalAsset() const
{
    return getExt().find("logical_asset") != getExt().end();
}

void AssetImpl::remove(bool recursive)
{
    if (RC0 == getInternalName()) {
        throw std::runtime_error("Prevented deleting RC-0");
    }

    if (hasLogicalAsset()) {
        throw std::runtime_error(TRANSLATE_ME("a logical_asset (sensor) refers to it"));
    }

    if (!getLinkedAssets().empty()) {
        throw std::runtime_error(TRANSLATE_ME("can't delete the asset because it is linked to others"));
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

    m_db->beginTransaction();
    try {
        if (isAnyOf(getAssetType(), TYPE_DATACENTER, TYPE_ROW, TYPE_ROOM, TYPE_RACK)) {
            if (m_db->isLastDataCenter(*this)) {
                throw std::runtime_error(TRANSLATE_ME("will not allow last datacenter to be deleted"));
            }
            m_db->removeExtMap(*this);
            m_db->removeFromGroups(*this);
            m_db->removeFromRelations(*this);
            m_db->removeAsset(*this);
        } else if (isAnyOf(getAssetType(), TYPE_GROUP)) {
            m_db->clearGroup(*this);
            m_db->removeExtMap(*this);
            m_db->removeAsset(*this);
        } else if (isAnyOf(getAssetType(), TYPE_DEVICE)) {
            m_db->removeFromGroups(*this);
            m_db->unlinkFrom(*this);
            m_db->removeFromRelations(*this);
            m_db->removeExtMap(*this);
            m_db->removeAsset(*this);
        } else {
            m_db->removeExtMap(*this);
            m_db->removeAsset(*this);
        }
    } catch (const std::exception& e) {
        m_db->rollbackTransaction();
        throw std::runtime_error(e.what());
    }
    m_db->commitTransaction();
}

void AssetImpl::save()
{
    m_db->beginTransaction();
    try {
        if (getId()) {
            m_db->update(*this);
        } else { // create
            // set creation timestamp
            setExtEntry(fty::EXT_CREATE_TS, generateCurrentTimestamp(), true);
            // set uuid
            setExtEntry(fty::EXT_UUID, generateUUID(getManufacturer(), getModel(), getSerialNo()), true);

            m_db->insert(*this);
        }
        m_db->saveLinkedAssets(*this);
        m_db->saveExtMap(*this);
    } catch (const std::exception& e) {
        m_db->rollbackTransaction();
        throw e.what();
    }
    m_db->commitTransaction();
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
        m_db->update(*this);
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

        setAssetStatus(fty::AssetStatus::Nonactive);
        m_db->update(*this);
    }
}

cxxtools::SerializationInfo AssetImpl::getSerializedData()
{
    using namespace fty::conversion;

    std::unique_ptr<AssetImpl::DB> db;
    if (g_testMode) {
        db = std::unique_ptr<AssetImpl::DB>(new DBTest);
    } else {
        db = std::unique_ptr<AssetImpl::DB>(new DB);
        db->init();
    }

    cxxtools::SerializationInfo si;

    cxxtools::SerializationInfo& assets = si.addMember("assets");

    for (const std::string assetName : db->listAllAssets()) {
        AssetImpl a(assetName);

        log_debug("Backing up asset %s...", a.getInternalName().c_str());

        cxxtools::SerializationInfo& siAsset = assets.addMember("");
        siAsset <<= a;
    }

    assets.setCategory(cxxtools::SerializationInfo::Array);

    return si;
}
void AssetImpl::restoreDataFromSi(cxxtools::SerializationInfo& si)
{
    using namespace fty::conversion;

    const cxxtools::SerializationInfo& assets = si.getMember("assets");

    for (auto it = assets.begin(); it != assets.end(); ++it) {
        AssetImpl a;

        *it >>= a;

        log_debug("Restoring asset %s...", a.getInternalName().c_str());

        std::stringstream s;
        a.dump(s);

        std::cout << s.str() << std::endl;

        // a.save();
    }
}

std::vector<std::string> AssetImpl::list()
{
    std::unique_ptr<AssetImpl::DB> db;
    if (g_testMode) {
        db = std::unique_ptr<AssetImpl::DB>(new DBTest);
    } else {
        db = std::unique_ptr<AssetImpl::DB>(new DB);
        db->init();
    }
    return db->listAllAssets();
}

void AssetImpl::reload()
{
    m_db->loadAsset(getInternalName(), *this);
    m_db->loadExtMap(*this);
    m_db->loadChildren(*this);
    m_db->loadLinkedAssets(*this);
}

static void printAssetTreeRec(const std::string& iname, int level)
{
    std::cout << "print on asset " << iname << std::endl;
    AssetImpl a(iname);
    for (const auto& child : a.getChildren()) {
        for (int i = 0; i < level; i++) {
            std::cout << "\t";
        }
        std::cout << child << std::endl;

        printAssetTreeRec(child, level + 1);
    }
}

static void getChildrenTree(const std::string& iname, std::vector<AssetImpl>& tree)
{
    AssetImpl a(iname);
    for (const auto& child : a.getChildren()) {
        tree.push_back(a);
        getChildrenTree(child, tree);
    }
}

void AssetImpl::deleteList(const std::vector<std::string>& assets)
{
    std::vector<AssetImpl> toDel;

    std::cerr << "list of asset to delete:\n" << std::endl;
    for (const auto& a : assets) {
        std::cerr << a << " ";
    }
    std::cerr << std::endl;

    std::vector<std::string> errors;

    for (const std::string& iname : assets) {
        std::vector<AssetImpl> childrenTree;

        AssetImpl a(iname);
        toDel.push_back(a);

        // get tree of children recursively
        getChildrenTree(iname, childrenTree);

        std::cerr << "list of children of " << iname << ":\n" << std::endl;
        for (const auto& a : assets) {
            std::cerr << a << " ";
        }
        std::cerr << std::endl;

        // remove assets already in the list
        for (const std::string& iname : assets) {
            childrenTree.erase(std::remove_if(childrenTree.begin(), childrenTree.end(),
                                   [&](const Asset& child) {
                                       return iname == child.getInternalName();
                                   }),
                childrenTree.end());
        }

        // get linked assets
        std::vector<std::string> linkedAssets = a.getLinkedAssets();
        // remove assets already in the list
        for (const std::string& iname : assets) {
            linkedAssets.erase(
                std::remove(linkedAssets.begin(), linkedAssets.end(), iname), linkedAssets.end());
        }

        if (!childrenTree.empty() || !linkedAssets.empty()) {
            errors.push_back(iname);
        }
    }

    auto isLinked = [&](const Asset& l, const Asset& r) {
        // check if l is linked to r (l < r)
        auto linksL    = l.getLinkedAssets();
        auto isLinkedL = std::find(linksL.begin(), linksL.end(), r.getInternalName());
        if (isLinkedL != linksL.end()) {
            return false;
        }
        // check if r is linked to l (l > r)
        auto linksR    = r.getLinkedAssets();
        auto isLinkedR = std::find(linksR.begin(), linksR.end(), l.getInternalName());
        if (isLinkedR != linksR.end()) {
            return true;
        }
        return false;
    };

    auto isAnyParent = [&](const Asset& l, const Asset& r) {
        auto rUp = r;
        while (true) {
            // if l is child of r, l < r
            if (l.getParentIname() == rUp.getInternalName()) {
                return true;
            }
            // if r is not parent of l, look in deletion list for ancestor of r
            auto it = std::find_if(toDel.begin(), toDel.end(), [&](const Asset& it) {
                return it.getInternalName() == rUp.getParentIname();
            });
            // if parent of r is not in the list, they are on separate branches
            if (it == toDel.end()) {
                return false;
            }
            // go one level up on r side
            rUp = *it;
        }
    };

    // sort by deletion order
    std::sort(toDel.begin(), toDel.end(), [&](const Asset& l, const Asset& r) {
        return isAnyParent(l, r) || isLinked(l, r);
    });

    // TODO remove
    printAssetTreeRec(toDel.begin()->getInternalName(), 0);

    for (auto& d : toDel) {
        // check if asset is in error list
        auto it = std::find_if(errors.begin(), errors.end(), [&](const std::string& iname) {
            return d.getInternalName() == iname;
        });

        if (it != errors.end()) {
            std::cerr << "Asset " << *it << " cannot be deleted" << std::endl;
        } else {
            d.deactivate();
            // d.remove(false);
        }
    }
}


void AssetImpl::deleteAll()
{
    deleteList(list());
}

} // namespace fty
