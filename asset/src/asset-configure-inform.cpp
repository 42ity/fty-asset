/*  ========================================================================
    Copyright (C) 2020 Eaton
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
    ========================================================================
*/

#include "asset/asset-configure-inform.h"
#include <fty_common_db.h>
#include <fty_common_db_connection.h>
#include <fty_common.h>
#include <fty_common_mlm_utils.h>
#include <fty_proto.h>
#include <fty_log.h>
#include <malamute.h>
#include <thread>

namespace fty::asset {

static void* voidify(const std::string& str)
{
    return reinterpret_cast<void*>(const_cast<char*>(str.c_str()));
}

static zhash_t* s_map2zhash(const std::map<std::string, std::string>& mss)
{
    zhash_t* ret = zhash_new();
    if (!ret) { return nullptr; }
    zhash_autofree(ret);
    for (const auto& it : mss) {
        zhash_insert(ret, it.first.c_str(), voidify(it.second));
    }
    return ret;
}

// IMPORTANT NOTE: upses *must* be autofree
static bool s_getDcUPSes(fty::db::Connection& conn, const std::string& assetName, zhash_t* upses)
{
    if (!upses) {
        return false;
    }

    auto dcId = db::nameToAssetId(assetName);
    if (!dcId) {
        return false;
    }

    std::vector<std::string> listUps;

    auto addUps = [&listUps](const fty::db::Row& row) {
        listUps.push_back(row.get("name"));
    };

    int r = db::selectAssetsByContainer(conn, *dcId, {persist::asset_type::DEVICE}, {persist::asset_subtype::UPS}, "", "active", addUps);
    if (r != 0) {
        return false;
    }

    int i = 0;
    for (const auto& ups : listUps) {
        char key[16];
        snprintf(key, sizeof(key), "ups%d", i);
        zhash_insert(upses, key, voidify(ups));
        i++;
    }
    return true;
}

// CAUTION: very similar code in server/src/fty_asset_server.cc::s_publish_create_or_update_asset_msg()
Expected<void> sendConfigure(const std::vector<std::pair<db::AssetElement, persist::asset_operation>>& rows, const std::string& agentName)
{
    mlm_client_t* client = mlm_client_new();
    if (!client) {
        return unexpected("mlm_client_new () failed.");
    }

    int r = mlm_client_connect(client, MLM_ENDPOINT, 1000, agentName.c_str());
    if (r != 0) {
        mlm_client_destroy(&client);
        return unexpected("mlm_client_connect () failed.");
    }

    r = mlm_client_set_producer(client, FTY_PROTO_STREAM_ASSETS);
    if (r != 0) {
        mlm_client_destroy(&client);
        return unexpected(" mlm_client_set_producer () failed.");
    }

    fty::db::Connection conn;
    for (const auto& oneRow : rows) {
        // row.first defines the asset, .second the operation/action
        const std::string s_priority{std::to_string(oneRow.first.priority)};
        const std::string s_parent{std::to_string(oneRow.first.parentId)};
        const std::string s_asset_name{oneRow.first.name};
        const std::string s_asset_type{persist::typeid_to_type(oneRow.first.typeId)};
        const std::string s_asset_subtype{persist::subtypeid_to_subtype(oneRow.first.subtypeId)};

        zhash_t* aux = zhash_new();
        zhash_autofree(aux);
        zhash_insert(aux, "priority", voidify(s_priority));
        zhash_insert(aux, "type", voidify(s_asset_type));
        zhash_insert(aux, "subtype", voidify(s_asset_subtype));
        zhash_insert(aux, "parent", voidify(s_parent));
        zhash_insert(aux, "status", voidify(oneRow.first.status));

        // this is a bit hack, but we know that our topology ends with datacenter (hopefully)
        std::string dc_name;

        auto cb = [aux, &dc_name](const fty::db::Row& row) {
            static const std::vector<std::string> names({
                "parent_name1", "parent_name2", "parent_name3", "parent_name4", "parent_name5",
                "parent_name6", "parent_name7", "parent_name8", "parent_name9", "parent_name10"
            });

            for (const auto& name : names) {
                const std::string pname_value{row.get(name)};
                if (pname_value.empty()) {
                    continue;
                }
                std::string pname_key{name};
                // 11 == strlen ("parent_name")
                pname_key.insert(11, 1, '.'); // "parent_nameX" -> "parent_name.X"
                zhash_insert(aux, pname_key.c_str(), voidify(pname_value));
                dc_name = pname_value;
            }
        };

        auto res = db::selectAssetElementSuperParent(oneRow.first.id, cb);
        if (!res) {
            logError("selectAssetElementSuperParent error: {}", res.error());
            zhash_destroy(&aux);
            mlm_client_destroy(&client);
            return unexpected("persist::select_asset_element_super_parent () failed.");
        }

        zhash_t* ext = s_map2zhash(oneRow.first.ext);

        const std::string subject{s_asset_type + "." + s_asset_subtype + "@" + s_asset_name};
        const std::string operation{operation2str(oneRow.second)};

        zmsg_t* msg = fty_proto_encode_asset(aux, s_asset_name.c_str(), operation.c_str(), ext);
        zhash_destroy(&ext);
        zhash_destroy(&aux);
        r = mlm_client_send(client, subject.c_str(), &msg);
        zmsg_destroy(&msg);
        if (r != 0) {
            log_error("send failed: subject='%s', operation='%s'", subject.c_str(), operation.c_str());
            mlm_client_destroy(&client);
            return unexpected("mlm_client_send () failed.");
        }

        // ask fty-asset to republish so we would get UUID
        if (streq(operation.c_str(), FTY_PROTO_ASSET_OP_CREATE)
            || streq(operation.c_str(), FTY_PROTO_ASSET_OP_UPDATE))
        {
            msg = zmsg_new();
            zmsg_addstr(msg, s_asset_name.c_str());
            r = mlm_client_sendto(client, AGENT_FTY_ASSET, "REPUBLISH", nullptr, 5000, &msg);
            zmsg_destroy(&msg);
            if (r != 0) {
                log_error("sendto %s REPUBLISH %s failed.", AGENT_FTY_ASSET, s_asset_name.c_str());
            }
            //no response expected
        }

        // data for uptime
        if (oneRow.first.subtypeId == persist::asset_subtype::UPS) {
            log_debug("uptime inventory dc='%s'", dc_name.c_str());

            aux = zhash_new();
            zhash_autofree(aux);
            if (!s_getDcUPSes(conn, dc_name, aux)) {
                log_error("Cannot read upses for dc='%s'", dc_name.c_str());
            }

            const std::string type{"datacenter"};
            zhash_update(aux, "type", voidify(type));

            msg = fty_proto_encode_asset(aux, dc_name.c_str(), FTY_PROTO_ASSET_OP_INVENTORY, nullptr);
            zhash_destroy(&aux);
            r = mlm_client_send(client, std::string{type + ".unknown@" + dc_name}.c_str(), &msg);
            zmsg_destroy(&msg);
            if (r != 0) {
                log_error("send failed: uptime inventory dc='%s'", dc_name.c_str());
                mlm_client_destroy(&client);
                return unexpected("mlm_client_send () failed.");
            }
        }
    }

    zclock_sleep(500); // ensure all was sent before client destroy
    mlm_client_destroy(&client);

    return {};
}

Expected<void> sendConfigure(const db::AssetElement& row, persist::asset_operation actionType, const std::string& agentName)
{
    return sendConfigure({std::make_pair(row, actionType)}, agentName);
}

std::string generateMlmClientId(const std::string& client_name)
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    std::string pid = ss.str();
    if (pid.empty()) { pid = std::to_string(random()); }
    return client_name + "." + pid;
}

} // namespace fty::asset
