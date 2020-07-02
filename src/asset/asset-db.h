/*  =========================================================================
    asset_db - asset_db

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

#pragma once
#include "asset-storage.h"
#include <memory>
#include <mutex>
#include <string>
#include <tntdb.h>
#include <vector>

namespace fty {

class DB : public AssetStorage
{
public:
    static DB& getInstance()
    {
        static DB m_instance;
        return m_instance;
    }

    void loadAsset(const std::string& nameId, Asset& asset);

    void                     loadExtMap(Asset& asset);
    void                     loadLinkedAssets(Asset& asset);
    std::vector<std::string> getChildren(const Asset& asset);

    bool hasLinkedAssets(const Asset& asset);
    void link(Asset& src, const std::string& srcOut, Asset& dest, const std::string& destIn, int linkType);
    void unlink(Asset& src, const std::string& srcOut, Asset& dest, const std::string& destIn, int linkType);
    void unlinkAll(Asset& dest);
    void clearGroup(Asset& asset);
    void removeAsset(Asset& asset);
    void removeFromRelations(Asset& asset);
    void removeFromGroups(Asset& asset);
    void removeExtMap(Asset& asset);
    bool isLastDataCenter(Asset& asset);

    void beginTransaction();
    void rollbackTransaction();
    void commitTransaction();

    void update(Asset& asset);
    void insert(Asset& asset);

    void        saveLinkedAssets(Asset& asset);
    void        saveExtMap(Asset& asset);
    std::string inameById(uint32_t id);
    std::string inameByUuid(const std::string& uuid);

    std::vector<std::string> listAllAssets();

private:
    DB(bool test = false);
    std::mutex                m_conn_lock;
    mutable tntdb::Connection m_conn;
};

} // namespace fty
