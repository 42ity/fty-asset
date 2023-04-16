/*  =========================================================================
    topology_power_topology_power - class description

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

/*
@header
    topology_power_topology_power -
@discuss
@end
*/

#include "topology_power.h"

////////////////////////////////////////////////////////////////////////
// copied from fty-rest src/web/src/topology_power.ecpp
// C++ transformed
// see also code in src/topology_power/
// implementation of REST /api/v1/topology/power (see RFC11)
////////////////////////////////////////////////////////////////////////

/*!
 * \file topology_power.ecpp
 * \author Karol Hrdina <KarolHrdina@Eaton.com>
 * \author Jim Klimov <EvgenyKlimov@Eaton.com>
 * \author Michal Hrusecky <MichalHrusecky@Eaton.com>
 * \author Alena Chernikava <AlenaChernikava@Eaton.com>
 * \author Michal Vyskocil <MichalVyskocil@Eaton.com>
 * \brief  process power topology requests
 */

#include <string>
#include <sstream>
#include <vector>

#include <czmq.h>
#include <fty_log.h>
#include <fty_common_db_dbpath.h>
#include <fty_common_db.h>
#include <fty_common.h>
#include <fty_common_macros.h>

#include "tntdb.h"
#include "topology2.h"
#include "dbtypes.h"
#include "asset/dbhelpers.h"
#include "data.h"
#include "cleanup.h"
#include "assettopology.h"
#include "location_helpers.h"
#include "utilspp.h"

//
//

int topology_power (std::map<std::string, std::string> & param, std::string & json)
{
    json = "";

    // checked parameters
    int64_t checked_id;
    int request_type = 0;
    std::string asset_id;
    std::string parameter_name;

    // ##################################################
    // BLOCK 1
    // Sanity parameter check
    {
        std::string from = param["from"];
        std::string to = param["to"];
        std::string filter_dc = param["filter_dc"];
        std::string filter_group = param["filter_group"];

        // non-empty count
        int ne_count = (from.empty()?0:1) + (to.empty()?0:1) + (filter_dc.empty()?0:1) + (filter_group.empty()?0:1);
        if (ne_count != 1) {
            //http_die("request-param-required", "from/to/filter_dc/filter_group");
            log_error("request-param-required from/to/filter_dc/filter_group");
            param["error"] = TRANSLATE_ME("'from', 'to', 'filter_dc' or 'filter_group' argument must be set exclusive");
            return -1;
        }

        if (!from.empty()) {
            request_type = ASSET_MSG_GET_POWER_FROM;
            asset_id = from;
            parameter_name = "from";
        }
        else if (!to.empty()) {
            request_type = ASSET_MSG_GET_POWER_TO;
            asset_id = to;
            parameter_name = "to";
        }
        else if (!filter_dc.empty()) {
            request_type = ASSET_MSG_GET_POWER_DATACENTER;
            asset_id = filter_dc;
            parameter_name = "filter_dc";
        }
        else if (!filter_group.empty()) {
            request_type = ASSET_MSG_GET_POWER_GROUP;
            asset_id = filter_group;
            parameter_name = "filter_group";
        }
        else {
            log_error("case not handled");
            param["error"] = TRANSLATE_ME("Internal error");
            return -2;
        }

        if (!persist::is_ok_name (asset_id.c_str ()) ) {
            //std::string expected = TRANSLATE_ME("valid asset name");
            //http_die ("request-param-bad", "id", asset_id.c_str (), expected.c_str ());
            log_error("request-param-bad invalid asset name");
            param["error"] = TRANSLATE_ME("Asset name is not valid (%s)", asset_id.c_str ());
            return -3;
        }

        checked_id = DBAssets::name_to_asset_id (asset_id);

        if (checked_id == -1) {
            //std::string expected = TRANSLATE_ME("existing asset name");
            //http_die ("request-param-bad", "id", asset_id.c_str (), expected.c_str ());
            log_error("request-param-bad");
            param["error"] = TRANSLATE_ME("Asset not found (%s)", asset_id.c_str ());
            return -4;
        }
        if (checked_id == -2) {
            //std::string err =  TRANSLATE_ME("Connecting to database failed.");
            //http_die ("internal-error", err.c_str ());
            log_error("db-connection-failed");
            param["error"] = TRANSLATE_ME("Connection to database failed");
            return -5;
        }
    }
    // Sanity check end

    // ##################################################
    // BLOCK 2
    _scoped_asset_msg_t *input_msg = asset_msg_new (request_type);
    if ( !input_msg ) {
        //std::string err =  TRANSLATE_ME("Cannot allocate memory");
        //http_die ("internal-error", err.c_str ());
        log_error("malloc-failed");
        param["error"] = TRANSLATE_ME("Allocation failed");
        return -10;
    }

   // Call persistence layer
    asset_msg_set_element_id (input_msg,  static_cast<uint32_t>(checked_id));
    _scoped_zmsg_t *return_msg = process_assettopology (DBConn::url.c_str(), &input_msg);
    asset_msg_destroy (&input_msg);
    if (return_msg == NULL) {
        //log_error ("Function process_assettopology() returned a null pointer");
        //http_die("internal-error", "");
        log_error("process_assettopology-failed return null");
        param["error"] = TRANSLATE_ME("Internal error");
        return -11;
    }

    if (is_common_msg (return_msg)) {
        _scoped_common_msg_t *common_msg = common_msg_decode (&return_msg);
        zmsg_destroy (&return_msg);
        if (common_msg == NULL) {
            //log_error ("common_msg_decode() failed");
            //http_die("internal-error", "");
            log_error("common_msg_decode-failed");
            param["error"] = TRANSLATE_ME("Internal error");
            return -20;
        }

        if (common_msg_id (common_msg) == COMMON_MSG_FAIL) {
            log_error ("common_msg is COMMON_MSG_FAIL");
            uint32_t err = common_msg_errorno(common_msg);
            switch(err) {
                case DB_ERROR_BADINPUT:
                    //std::string received = TRANSLATE_ME("id of the asset, that is not a device");
                    //std::string expected = TRANSLATE_ME("id of the asset, that is a device");
                    //http_die("request-param-bad", parameter_name.c_str(), received.c_str (), expected.c_str ());
                    log_error("request-param-bad parameter_name: %s", parameter_name.c_str());
                    param["error"] = TRANSLATE_ME("Asset is not a device (%s)", asset_id.c_str());
                    break;
                case DB_ERROR_NOTFOUND:
                    //http_die("element-not-found", asset_id.c_str());
                    log_error("element-not-found %s", asset_id.c_str());
                    param["error"] = TRANSLATE_ME("Asset not found (%s)", asset_id.c_str());
                    break;
                default:
                    //http_die("internal-error", "");
                    log_error("internal-error err: %" PRIu32, err);
                    param["error"] = TRANSLATE_ME("Internal error");
            }
            common_msg_destroy(&common_msg);
            return -21;
        }
        else {
            log_error ("Unexpected common_msg received. ID = %" PRIu32 , common_msg_id (common_msg));
            //http_die("internal-error", "");
            common_msg_destroy(&common_msg);
            param["error"] = TRANSLATE_ME("Internal error");
            return -22;
        }
    }
    else if (is_asset_msg (return_msg)) {
        _scoped_asset_msg_t *asset_msg = asset_msg_decode (&return_msg);
        zmsg_destroy (&return_msg);

        if (asset_msg == NULL) {
            //log_error ("asset_msg_decode() failed");
            //http_die("internal-error", "");
            log_error("common_msg_decode-failed");
            param["error"] = TRANSLATE_ME("Internal error");
            return -30;
        }

        if (asset_msg_id (asset_msg) == ASSET_MSG_RETURN_POWER) {
            _scoped_zlist_t *powers = asset_msg_get_powers (asset_msg);
            _scoped_zframe_t *devices = asset_msg_get_devices (asset_msg);
            asset_msg_destroy (&asset_msg);

            json = "{";

            if (devices) {
#if CZMQ_VERSION_MAJOR == 3
                byte *buffer = zframe_data (devices);
                assert (buffer);
                _scoped_zmsg_t *zmsg = zmsg_decode ( buffer, zframe_size (devices));
#else
                _scoped_zmsg_t *zmsg = zmsg_decode (devices);
#endif
                if (zmsg == NULL || !zmsg_is (zmsg)) {
                    zframe_destroy (&devices);
                    log_error ("zmsg_decode() failed");
                    //http_die("internal-error", "");
                    param["error"] = TRANSLATE_ME("Internal error");
                    return -31;
                }
                zframe_destroy (&devices);

                json.append ("\"devices\" : [");
                _scoped_zmsg_t *pop = NULL;
                bool first = true;
                while ((pop = zmsg_popmsg (zmsg)) != NULL) { // caller owns zmgs_t
                    if (!is_asset_msg (pop)) {
                        zmsg_destroy (&zmsg);
                        log_error ("malformed internal structure of returned message");
                        //http_die("internal-error", "");
                        param["error"] = TRANSLATE_ME("Internal error");
                        return -32;
                    }
                    _scoped_asset_msg_t *item = asset_msg_decode (&pop); // _scoped_zmsg_t is freed
                    if (item == NULL) {
                        if (pop != NULL) {
                            zmsg_destroy (&pop);
                        }
                        log_error ("asset_smg_decode() failed for internal messages");
                        //http_die("internal-error", "");
                        param["error"] = TRANSLATE_ME("Internal error");
                        return -33;
                    }

                    if (first) first = false;
                    else json.append (", ");

                    std::pair<std::string,std::string> asset_names = DBAssets::id_to_name_ext_name (asset_msg_element_id (item));
                    if (asset_names.first.empty () && asset_names.second.empty ()) {
                        //std::string err =  TRANSLATE_ME("Database failure");
                        //http_die ("internal-error", err.c_str ());
                        log_error ("database-failure");
                        param["error"] = TRANSLATE_ME("Database access failed");
                        return -34;
                    }

                    json.append("{ \"name\" : \"").append(asset_names.second).append("\",");
                    json.append("\"id\" : \"").append(asset_msg_name (item)).append("\",");
                    json.append("\"sub_type\" : \"").append(utils::strip (asset_msg_type_name (item))).append("\"}");

                    asset_msg_destroy (&item);
                }
                zmsg_destroy (&zmsg);
                json.append ("] ");
            }

            if (powers) {
                if (json != "{") {
                    json.append (", ");
                }
                json.append ("\"powerchains\" : [");

                const char *item = static_cast<const char*>(zlist_first (powers));
                bool first = true;
                while (item) {
                    if (first) first = false;
                    else json.append (", ");

                    json.append ("{");
                    std::vector<std::string> tokens;
                    std::istringstream f(item);
                    std::string tmp;
                    // "src_socket:src_id:dst_socket:dst_id"
                    while (getline(f, tmp, ':')) {
                        tokens.push_back(tmp);
                    }
                    std::pair<std::string,std::string> src_names = DBAssets::id_to_name_ext_name (static_cast<uint32_t>(std::stoi (tokens[1])));
                    if (src_names.first.empty () && src_names.second.empty ()) {
                        //std::string err =  TRANSLATE_ME("Database failure");
                        //http_die ("internal-error", err.c_str ());
                        log_error ("database-failure");
                        param["error"] = TRANSLATE_ME("Database access failed");
                        return -35;
                    }
                    json.append("\"src-id\" : \"").append(src_names.first).append("\",");

                    if (!tokens[0].empty() && tokens[0] != "999") {
                        json.append("\"src-socket\" : \"").append(tokens[0]).append("\",");
                    }
                    std::pair<std::string,std::string> dst_names = DBAssets::id_to_name_ext_name (static_cast<uint32_t>(std::stoi (tokens[3])));
                    if (dst_names.first.empty () && dst_names.second.empty ()) {
                        std::string err =  TRANSLATE_ME("Database failure");
                        //http_die ("internal-error", err.c_str ());
                        log_error ("database-failure");
                        param["error"] = TRANSLATE_ME("Database access failed");
                        return -36;
                    }
                    json.append("\"dst-id\" : \"").append(dst_names.first).append("\"");

                    if (!tokens[2].empty() && tokens[2] != "999") {
                        json.append(",\"dst-socket\" : \"").append(tokens[2]).append("\"");
                    }
                    json.append ("}");
                    item = static_cast<const char*>(zlist_next (powers));
                }
                json.append ("] ");
            }
            json.append ("}");
        }
        else {
            log_error ("Unexpected asset_msg received. ID = %" PRIu32 , asset_msg_id (asset_msg));
            //http_die("internal-error", "");
            asset_msg_destroy (&asset_msg);
            param["error"] = TRANSLATE_ME("Internal error");
            return -37;
        }
    }
    else {
        log_error ("Unknown protocol");
        //LOG_END;
        //http_die("internal-error", "");
        zmsg_destroy (&return_msg);
        param["error"] = TRANSLATE_ME("Internal error");
        return -38;
    }

    zmsg_destroy (&return_msg);
    return 0; //ok
}
