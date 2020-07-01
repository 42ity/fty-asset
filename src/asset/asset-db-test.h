/*  =========================================================================
    asset_asset_db_test - asset/asset-db-test

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

#pragma once
#include "asset-storage.h"
#include <memory>
#include <string>
#include <vector>

namespace fty {

class DBTest : public AssetStorage
{
public:
    static DBTest& getInstance()
    {
        static DBTest m_instance;
        return m_instance;
    }

    void loadAsset(const std::string& nameId, Asset& asset) override;

    void                     loadExtMap(Asset& asset) override;
    void                     loadLinkedAssets(Asset& asset) override;
    std::vector<std::string> getChildren(const Asset& asset) override;

    bool hasLinkedAssets(const Asset& asset) override;
    void link(Asset& src, const std::string& srcOut, Asset& dest, const std::string& destIn,
        int linkType = 1) override;
    void unlink(Asset& src, const std::string& srcOut, Asset& dest, const std::string& destIn,
        int linkType = 1) override;
    void unlinkAll(Asset& dest) override;
    void clearGroup(Asset& asset) override;
    void removeAsset(Asset& asset) override;
    void removeFromRelations(Asset& asset) override;
    void removeFromGroups(Asset& asset) override;
    void removeExtMap(Asset& asset) override;
    bool isLastDataCenter(Asset& asset) override;

    void beginTransaction() override;
    void rollbackTransaction() override;
    void commitTransaction() override;

    void update(Asset& asset) override;
    void insert(Asset& asset) override;

    void        saveLinkedAssets(Asset& asset) override;
    void        saveExtMap(Asset& asset) override;
    std::string inameById(uint32_t id) override;
    std::string inameByUuid(const std::string& uuid) override;

    std::vector<std::string> listAllAssets() override;

private:
    DBTest();
};

} // namespace fty
