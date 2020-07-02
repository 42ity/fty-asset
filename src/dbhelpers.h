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

#ifndef DBHELPERS_H_INCLUDED
#define DBHELPERS_H_INCLUDED


#include <functional>
#include <string>
#include <map>
#include <unordered_map>
#include <tntdb/row.h>
#include <tntdb/connect.h>
#include <czmq.h>
#include <vector>
#include <tuple>
#include <sstream>
#include <type_traits>
#include "fty_asset_classes.h"

// Selects assets in a given container
FTY_ASSET_PRIVATE int
    select_assets_by_container (
        const std::string& container_name,
        const std::set <std::string>& filter,
        std::vector <std::string>& assets,
        bool test
    );

// Selects basic asset info
FTY_ASSET_PRIVATE int
    select_asset_element_basic
        (const std::string &asset_name,
         std::function<void(const tntdb::Row&)> cb,
         bool test);

// Selects ext attributes for given asset
FTY_ASSET_PRIVATE int
    select_ext_attributes
        (uint32_t asset_id,
         std::function<void(const tntdb::Row&)> cb,
         bool test);

// Selects all parents of given asset
FTY_ASSET_PRIVATE int
    select_asset_element_super_parent (
        uint32_t id,
        std::function<void(const tntdb::Row&)>& cb,
        bool test);

// Selects all assets in the DB of given types/subtypes
FTY_ASSET_PRIVATE int
    select_assets_by_filter (
        const std::set <std::string>& filter,
        std::vector <std::string>& assets,
        bool test
    );

// Selects basic asset info for all assets in the DB
FTY_ASSET_PRIVATE int
    select_assets (
            std::function<void(
                const tntdb::Row&
                )>& cb, bool test);

//////////////////////////////////////////////////////////////////////////////////

// Inserts ext attributes from inventory message into DB
FTY_ASSET_PRIVATE int
    process_insert_inventory
    (const std::string& device_name,
    zhash_t *ext_attributes,
    bool read_only,
    bool test);

// Inserts ext attributes from inventory message into DB if not present in the cache
FTY_ASSET_PRIVATE int
    process_insert_inventory
    (const std::string& device_name,
    zhash_t *ext_attributes,
    bool read_only,
    std::unordered_map<std::string,std::string> &map_cache,
    bool test);

// Selects user-friendly name for given asset name
FTY_ASSET_PRIVATE int
    select_ename_from_iname
    (std::string &iname,
     std::string &ename,
     bool test);

// Get amount of active power devices from database
FTY_ASSET_PRIVATE int
    get_active_power_devices
        (bool test = false);

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
    tntdb::Statement statement;

    statement = conn.prepareCached (query.str());

    tntdb::Row row = statement.
        set ("value", keyValue).
        selectRow ();

    TypeRet obj;
    if(!row[0].get(obj))
    {
        throw std::runtime_error("Unable to get data from DB");
    }

    return obj;
}

/// update one field of an asset in the database
/// UPDATE in asset_table <updateColumn> to <updateValue> where <keyColumn> = <keyValue>
template<typename TypeValue, typename TypeKey>
void updateAssetProperty(
    const std::string& updateColumn,
    const TypeValue& updateValue,
    const std::string& keyColumn,
    const TypeKey& keyValue
)
{
    std::stringstream query;
    query << " UPDATE t_bios_asset_element " <<
             " SET " << updateColumn << " = :updateValue" <<
             " WHERE " << keyColumn << " = :keyValue";
    
    tntdb::Connection conn = tntdb::connectCached (DBConn::url);
    tntdb::Statement statement;

    statement = conn.prepareCached (query.str());

    statement
        .set("updateValue", updateValue)
        .set("keyValue", keyValue)
        .execute();
}

// for test purposes
extern std::map<std::string, std::string> test_map_asset_state;

//  @end
#endif
