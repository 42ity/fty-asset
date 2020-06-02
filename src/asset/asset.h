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

#include "include/fty_asset_dto.h"
#include <cxxtools/serializationinfo.h>

extern bool g_testMode;

namespace fty {

static constexpr const char* RC0 = "rackcontroller-0";

class AssetImpl : public Asset
{
public:
    AssetImpl();
    AssetImpl(const std::string& nameId);
    ~AssetImpl() override;

    AssetImpl(const AssetImpl& a);

    AssetImpl& operator=(const AssetImpl& a);

    // asset operations
    void remove(bool recursive = false);
    bool hasLogicalAsset() const;
    void save();
    void reload();
    bool isActivable();
    void activate();
    void deactivate();

    // serialize/deserialize data
    static cxxtools::SerializationInfo getSerializedData();
    static void                        restoreDataFromSi(cxxtools::SerializationInfo& si);

    static std::vector<std::string> list();
    static void                     deleteList(const std::vector<std::string>& assets);
    static void                     deleteAll();

    using Asset::operator==;

private:
    class Interface;
    class DB;
    class DBTest;
    std::unique_ptr<DB> m_db;
};

} // namespace fty
