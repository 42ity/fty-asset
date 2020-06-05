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
    AssetImpl(const std::string& nameId, bool loadLinks = true);
    ~AssetImpl() override;

    AssetImpl(const AssetImpl& a);

    AssetImpl& operator=(const AssetImpl& a);

    // asset operations
    void remove(bool recursive = false, bool removeLastDC = false);
    bool hasLinkedAssets() const;
    bool hasLogicalAsset() const;
    void load(bool loadLinks = true);
    void save(bool saveLinks = true);
    bool isActivable();
    void activate();
    void deactivate();
    void linkTo(const std::string& src);
    void unlinkFrom(const std::string& src);

    // serialize/deserialize data
    static cxxtools::SerializationInfo getSerializedData();
    static std::vector<AssetImpl>      getDataFromSi(cxxtools::SerializationInfo& si);

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
