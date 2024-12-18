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

#pragma once

#include "fty_asset_dto.h"
#include <map>
#include <string>
#include <vector>

extern bool g_testMode;

namespace fty {

static constexpr const char* RC0 = "rackcontroller-0";

class AssetStorage;

using AssetFilters = std::map<std::string, std::vector<std::string>>;
void operator>>=(const cxxtools::SerializationInfo& si, AssetFilters& filters);

using DeleteStatus = std::vector<std::pair<Asset, std::string>>;

class AssetImpl : public Asset
{
public:
    AssetImpl();
    AssetImpl(const std::string& nameId);
    ~AssetImpl() override;

    AssetImpl(const AssetImpl& a);

    AssetImpl& operator=(const AssetImpl& a);

    // asset operations
    bool hasLinkedAssets() const;
    bool hasLogicalAsset() const;
    bool isVirtual() const;
    void load();
    void create();
    void update();
    void restore(bool restoreLinks = false);
    bool isActivable() const;
    void activate();
    void deactivate();
    void unlinkAll();

    void updateParentsList();

    static void assetToSrr(const AssetImpl& asset, cxxtools::SerializationInfo& si);
    static void srrToAsset(const cxxtools::SerializationInfo& si, AssetImpl& asset);

    static std::vector<std::string> list(const AssetFilters& filters);
    static std::vector<std::string> listAll();

    static DeleteStatus deleteList(const std::vector<std::string>& assets, bool recursive, bool deleteVirtualAssets = true, bool removeLastDC = false);
    static DeleteStatus deleteAll(bool deleteVirtualAsset = false);

    static std::string getInameFromUuid(const std::string& uuid);
    static uint32_t    getIDFromIname(const std::string& iname);
    static std::string getInameFromID(const uint32_t id);

    using Asset::operator==;

    friend std::vector<std::string> getChildren(const AssetImpl& a);

private:
    AssetStorage& m_storage;

    void remove(bool removeLastDC = false);
};

} // namespace fty
