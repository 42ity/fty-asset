/*  =========================================================================
    dbhelpers - Helpers functions for database

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

// clang-format off

#pragma once

#include <functional>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <sstream>
#include <type_traits>

#include <fty_common_db.h>
#include <tntdb/row.h>
#include <tntdb/connect.h>
#include <tntdb.h>
#include <czmq.h>


// Selects assets in a given container
int select_assets_by_container(
    const std::string& container_name,
    const std::set <std::string>& filter,
    std::vector <std::string>& assets,
    bool test
);

// Selects basic asset info
int select_asset_element_basic(
    const std::string &asset_name,
    std::function<void(const tntdb::Row&)> cb,
    bool test
);

// Selects ext attributes for given asset
int select_ext_attributes(
    uint32_t asset_id,
    std::function<void(const tntdb::Row&)> cb,
    bool test
);

// Selects all parents of given asset
int select_asset_element_super_parent(
    uint32_t id,
    std::function<void(const tntdb::Row&)>& cb,
    bool test
);

// Selects all assets in the DB of given types/subtypes
int select_assets_by_filter(
    const std::set <std::string>& filter,
    std::vector <std::string>& assets,
    bool test
);

// Selects basic asset info for all assets in the DB
int select_assets(
    std::function<void(const tntdb::Row&)>& cb,
    bool test
);

//////////////////////////////////////////////////////////////////////////////////

// Inserts ext attributes from inventory message into DB
int process_insert_inventory(
    const std::string& device_name,
    zhash_t *ext_attributes,
    bool read_only,
    bool test
);

// Inserts ext attributes from inventory message into DB if not present in the cache
int process_insert_inventory(
    const std::string& device_name,
    zhash_t *ext_attributes,
    bool read_only,
    std::unordered_map<std::string,std::string> &map_cache,
    bool test
);

// Selects user-friendly name for given asset name
int select_ename_from_iname(
    const std::string &iname,
    std::string &ename,
    bool test
);

////////////////////////////////////////////////////////////////////////////////

/// select one field of an asset from the database
/// SELECT <column> from asset_table where <keyColumn> = <keyValue>
template<typename TypeRet, typename TypeValue>
TypeRet selectAssetProperty(
    const std::string& column,     // column to select
    const std::string& keyColumn,  // key
    const TypeValue& keyValue      // value of key param
)
{
    std::stringstream query;
    query << " SELECT " << column <<
             " FROM t_bios_asset_element " <<
             " WHERE " << keyColumn << " = :value";

    tntdb::Connection conn = tntdb::connectCached (DBConn::url);

    auto q = conn.prepareCached (query.str());
    q.set ("value", keyValue);

    auto row = q.selectRow();

    TypeRet obj;
    row[0].get(obj);

    return obj;
}
