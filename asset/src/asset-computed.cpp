/*
 *
 * Copyright (C) 2015 - 2018 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "asset/asset-computed.h"
#include <fty_log.h>
#include <fty/convert.h>
#include <fty_common.h>
#include <fty_common_db_asset.h>
#include <fty_common_db_dbpath.h>
#include <functional>
#include <tntdb/connection.h>
#include <tntdb/row.h>

static int s_get_devices_usize(tntdb::Connection& conn, std::set<uint32_t>& elements)
{
    int size = 0;

    std::function<void(const tntdb::Row&)> sumarize = [&size](const tntdb::Row& row) {
        uint32_t tmp = fty::convert<uint32_t>(row.getString("value"));
        if (tmp != UINT32_MAX) {
            size += int(tmp);
        }
    };

    int rv = DBAssets::select_asset_ext_attribute_by_keytag(conn, "u_size", elements, sumarize);
    if (rv != 0)
        return rv;
    return size;
}

/* TODO: function reports only success or -1 indicating some error, which will be expressed
 *       as a null value in JSON output. For more fine grained error reporting, the
 *       item.ecpp must be reworked substantially.*/
int free_u_size(uint32_t elementId)
{
    try {
        tntdb::Connection conn;
        conn = tntdb::connect(DBConn::url);

        // get the rack u_size
        std::set<uint32_t> rack_id{elementId};
        int                freeusize = s_get_devices_usize(conn, rack_id);
        if (!freeusize) {
            return -1;
        }
        log_debug("rack size is %i", freeusize);

        // get id of the devices inside the rack
        std::set<uint32_t>                     element_ids{};
        std::function<void(const tntdb::Row&)> func = [&element_ids](const tntdb::Row& row) {
            uint32_t asset_id = 0;
            row["asset_id"].get(asset_id);
            element_ids.insert(asset_id);
        };

        auto rv = DBAssets::select_assets_by_container(conn, elementId, func);
        if (rv == -1) {
            log_debug("free_u_size() rv is null");
            return -1;
        }
        // substract sum( device size ) if there are some
        int freeusize2 = s_get_devices_usize(conn, element_ids);
        if (!freeusize2) {
            return -1;
        }
        freeusize -= element_ids.empty() ? 0 : freeusize2;
        log_debug("freeusize %d", freeusize);
        return freeusize;
    } catch (const std::exception& ex) {
        log_error("free_u_size fails %s", ex.what());
        return -1;
    }
}

static uint32_t s_select_outlet_count(tntdb::Connection& conn, uint32_t id)
{
    static const char* KEY = "outlet.count";

    std::map<std::string, std::pair<std::string, bool>> res;
    int                                                 ret = DBAssets::select_ext_attributes(conn, id, res);

    if (ret != 0 || res.count(KEY) == 0)
        return UINT32_MAX;

    std::string foo   = res.at(KEY).first.c_str();
    auto        dot_i = foo.find('.');
    if (dot_i != std::string::npos) {
        foo.erase(dot_i, foo.size() - dot_i);
    }

    // validation: too big values in DB are weird
    // we're not going to have epdu with more 10K+ outlets
    // in the near future - if so, then fix this code
    if (foo.size() > 5)
        return UINT32_MAX;

    return fty::convert<uint32_t>(foo);
}

int rack_outlets_available(uint32_t elementId, std::map<std::string, int>& res)
{
    int               sum     = -1;
    bool              tainted = false;
    tntdb::Connection conn;
    res["sum"] = sum;

    std::function<void(const tntdb::Row& row)> cb = [&conn, &sum, &tainted, &res](const tntdb::Row& row) {
        uint32_t device_subtype = 0;
        row["subtype_id"].get(device_subtype);
        if (!persist::is_epdu(int(device_subtype)) && !persist::is_pdu(int(device_subtype)))
            return;

        uint32_t device_asset_id = 0;
        row["asset_id"].get(device_asset_id);

        uint32_t foo          = s_select_outlet_count(conn, device_asset_id);
        int      outlet_count = foo != UINT32_MAX ? int(foo) : -1;

        int outlet_used = DBAssets::count_of_link_src(conn, device_asset_id);
        if (outlet_used == -1)
            outlet_count = -1;
        else
            outlet_count -= outlet_used;

        if (outlet_count >= 0)
            sum += outlet_count;
        else
            tainted = true;
        res[std::to_string(device_asset_id)] = outlet_count;
    };

    int rv;
    try {
        conn = tntdb::connect(DBConn::url);
        rv   = DBAssets::select_assets_by_container(conn, elementId, cb);
    } catch (std::exception& e) {
        log_error("%s", e.what());
        return -1;
    }

    if (!tainted)
        res["sum"] = sum + 1; // sum is initialized to -1
    return rv;
}
