/*  =========================================================================
    fty_asset_server - Asset server, that takes care about distribution of asset information across the system

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
    fty_asset_server - Asset server, that takes care about distribution of asset information across the system
@discuss
     ASSET PROTOCOL

     ------------------------------------------------------------------------
     ## Topology request

     power topology request:
         subject: "TOPOLOGY"
         message: is a multipart message A/B
                 A = "TOPOLOGY_POWER" - mandatory
                 B = "asset_name" - mandatory

     power topology reply in "OK" case:
         subject: "TOPOLOGY"
         message: is a multipart message A/B/D/C1/.../CN
                 A = "TOPOLOGY_POWER" - mandatory
                 B = "asset_name" - mandatory
                 D = "OK" - mandatory
                 Ci = "asset_name" of power source - not mandatory
                     if there are no power devices
                      -> message is A/B/D

     power topology reply in "ERROR" case:
         subject: "TOPOLOGY"
         message: is a multipart message A/B/D/E
                 A = "TOPOLOGY_POWER" - mandatory
                 B = "asset_name" - mandatory
                 D = "ERROR" - mandatory
                 E = "ASSET_NOT_FOUND"/"INTERNAL_ERROR" - mandatory


    ------------------------------------------------------------------------
    ## Asset manipulation protocol

    REQ:
        subject: "ASSET_MANIPULATION"
        Message is a fty protocol (fty_proto_t) message

        *) read-only/fty_proto ASSET message

        where:
        * 'operation' is one of [ create | create-force | update | delete | retire ].
           Asset messages with different operation value are discarded and not replied to.
        * 'read-only' tells us whether ext attributes should be inserted as read-only or not.
           Allowed values are READONLY and READWRITE.

    REP:
        subject: same as in REQ
        Message is a multipart string message:

        * OK/<asset_id>
        * ERROR/<reason>

        where:
 (OPTIONAL)     <asset_id>  = asset id (in case of create, update operation)
                <reason>    = Error message/code TODO

    Note: in REQ message certain asset information are encoded as follows

      'ext' field
          Power Links - key: "power_link.<device_name>", value: "<first_outlet_num>/<second_outlet_num>", i.e.
1 --> 2 == "1/2" Groups - key: "group", value: "<group_name_1>/.../<group_name_N>"


    ------------------------------------------------------------------------
    ## ASSETS in container

    REQ:
        subject: "ASSETS_IN_CONTAINER"
        Message is a multipart string message

        * GET/<container name>/<type 1>/.../<type n>

        where:
            <container name>        = Name of the container things belongs to that
                                      when empty, no container is used, so all assets are take in account
            <type X>                = Type or subtype to be returned. Possible values are
                                      ups
                                      TODO: add more
                                      when empty, no filtering is done
    REP:
        subject: "ASSETS_IN_CONTAINER"
        Message is a multipart message:

        * OK                         = empty container
        * OK/<asset 1>/.../<asset N> = non-empty
        * ERROR/<reason>

        where:
            <reason>          = ASSET_NOT_FOUND / INTERNAL_ERROR / BAD_COMMAND

    REQ:
        subject: "ASSETS"
        Message is a multipart string message

        * GET/<uuid>/<type 1>/.../<type n>

        where:
            <uuid>                  = zuuid of  message
            <type X>                = Type or subtype to be returned. Possible values are
                                      ups
                                      TODO: add more
                                      when empty, no filtering is done
    REP:
        subject: "ASSETS"
        Message is a multipart message:

        * OK                         = empty container
        * OK/<uuid>/<asset 1>/.../<asset N> = non-empty
        * ERROR/<uuid>/<reason>

        where:
            <reason>          = ASSET_NOT_FOUND / INTERNAL_ERROR / BAD_COMMAND
    ------------------------------------------------------------------------
    ## REPUBLISH

    REQ:
        subject: "REPUBLISH"
        Message is a multipart string message

        /asset1/asset2/asset3       - republish asset information about asset1 asset2 and asset3
        /$all                       - republish information about all assets

     ------------------------------------------------------------------------
     ## ENAME_FROM_INAME

     request user-friendly name for given iname:
         subject: "ENAME_FROM_INAME"
         message: is a string message A
                 B = "asset_iname" - mandatory

     reply in "OK" case:
         subject: "ENAME_FROM_INAME"
         message: is a multipart message A/B
                 A = "OK" - mandatory
                 B = user-friendly name of given asset - mandatory

     reply in "ERROR" case:
         subject: "ENAME_FROM_INAME"
         message: is a multipart message A/B
                 A = "ERROR" - mandatory
                 B = "ASSET_NOT_FOUND"/"MISSING_INAME" - mandatory


     ------------------------------------------------------------------------
     ## ASSET_DETAIL

     request all the available data about given asset:
         subject: "ASSET_DETAIL"
         message: is a multipart message A/B/C
                 A = "GET" - mandatory
                 B = uuid - mandatory
                 C = "asset_name" - mandatory

     power topology reply in "OK" case:
         subject: "ASSET_DETAIL"
         message: is fty-proto asset update message

     power topology reply in "ERROR" case:
         subject: "ASSET_DETAIL"
         message: is a multipart message A/B
                 A = "ERROR" - mandatory
                 B = "BAD_COMMAND"/"INTERNAL_ERROR"/"ASSET_NOT_FOUND" - mandatory


@end
*/

#include "fty_asset_server.h"
#include "fty_asset_autoupdate.h"

#include "test_str.h"
#include "asset-server.h"
#include "asset/dbhelpers.h"

#include "total_power.h"
#include "topology_processor.h"
#include "topology_power.h"

#include <fty_asset_dto.h>
#include <fty_common.h>
#include <fty_common_db_uptime.h>
#include <fty_common_messagebus.h>
#include <fty_proto.h>
#include <fty_common.h>
#include <fty_common_db.h>
#include <fty_common_mlm.h>

#include <malamute.h>
#include <mlm_client.h>
#include <tntdb/connect.h>
#include <functional>
#include <sys/time.h>
#include <uuid/uuid.h>
#include <ctime>
#include <string>
#include <iostream>
#include <cassert>

//mlm_client_t* (fty::AssetServer& server)
#define mailboxClient(server) const_cast<mlm_client_t*>(server.getMailboxClient())

// =============================================================================
// TOPOLOGY/POWER command processing (completed reply)
// bmsg request asset-agent TOPOLOGY REQUEST <uuid> POWER <assetID>
// =============================================================================

static void s_process_TopologyPower(const std::string& client_name, const char* asset_name, bool testMode, zmsg_t* reply)
{
    assert (reply);

    log_debug("%s:\tTOPOLOGY POWER asset_name: %s", client_name.c_str(), asset_name);

    std::string assetName(asset_name ? asset_name : "");

    // select power devices
    // result of power topology - list of power device names
    std::vector<std::string> powerDevices;
    int r = select_devices_total_power(assetName, powerDevices, testMode);

    zmsg_addstr(reply, assetName.c_str());

    // form a message according the contract for the case "OK" and for the case "ERROR"
    if (r == -1) {
        log_error("%s:\tTOPOLOGY POWER: Cannot select power sources (%s)", client_name.c_str(), asset_name);

        zmsg_addstr(reply, "ERROR");
        // zmsg_addstr (reply, "INTERNAL_ERROR");
        zmsg_addstr(reply, TRANSLATE_ME("Internal error").c_str());
    }
    else if (r == -2) {
        log_error("%s:\tTOPOLOGY POWER: Asset not found (%s)", client_name.c_str(), asset_name);

        zmsg_addstr(reply, "ERROR");
        // zmsg_addstr (reply, "ASSET_NOT_FOUND");
        zmsg_addstr(reply, TRANSLATE_ME("Asset not found").c_str());
    }
    else {
        log_debug("%s:\tPower topology for '%s':", client_name.c_str(), asset_name);

        zmsg_addstr(reply, "OK");
        for (const auto& powerDeviceName : powerDevices) {
            log_debug("%s:\t\t%s", client_name.c_str(), powerDeviceName.c_str());
            zmsg_addstr(reply, powerDeviceName.c_str());
        }
    }
}

// =============================================================================
// TOPOLOGY/POWER_TO command processing (completed reply)
// bmsg request asset-agent TOPOLOGY REQUEST <uuid> POWER_TO <assetID>
// =============================================================================

static void s_process_TopologyPowerTo(const std::string& client_name, const char* asset_name, zmsg_t* reply)
{
    assert (reply);

    log_debug("%s:\tTOPOLOGY POWER_TO asset_name: %s", client_name.c_str(), asset_name);

    std::string assetName(asset_name ? asset_name : "");
    std::string result;    // JSON payload
    std::string errReason; // JSON payload (TRANSLATE_ME)
    int r = topology_power_to(assetName, result, errReason);

    zmsg_addstr(reply, assetName.c_str());

    if (r != 0) {
        log_error("%s:\tTOPOLOGY POWER_TO r: %d (asset_name: %s)", client_name.c_str(), r, asset_name);

        if (errReason.empty()) {
            if (!asset_name) {
                errReason = TRANSLATE_ME("Missing argument");
            }
            else {
                errReason = TRANSLATE_ME("Internal error");
            }
        }
        zmsg_addstr(reply, "ERROR");
        zmsg_addstr(reply, errReason.c_str());
    }
    else {
        zmsg_addstr(reply, "OK");
        zmsg_addstr(reply, result.c_str()); // JSON in one frame
    }
}

// =============================================================================
// TOPOLOGY/POWERCHAINS command processing (completed reply)
// bmsg request asset-agent TOPOLOGY REQUEST <uuid> POWERCHAINS <select_cmd> <assetID>
// <select_cmd> in {"to", "from", "filter_dc", "filter_group"}
// =============================================================================

static void s_process_TopologyPowerchains(const std::string& client_name, const char* select_cmd, const char* asset_name, zmsg_t* reply)
{
    assert (reply);

    log_debug("%s:\tTOPOLOGY POWERCHAINS select_cmd: %s, asset_name: %s",
        client_name.c_str(), select_cmd, asset_name);

    std::string command(select_cmd ? select_cmd : "");
    std::string assetName(asset_name ? asset_name : "");
    std::string result;    // JSON payload
    std::string errReason; // JSON payload (TRANSLATE_ME)
    int r = topology_power_process(command, assetName, result, errReason);

    zmsg_addstr(reply, assetName.c_str());

    if (r != 0) {
        log_error("%s:\tTOPOLOGY POWERCHAINS r: %d (cmd: %s, asset_name: %s)",
            client_name.c_str(), r, select_cmd, asset_name);

        if (errReason.empty()) {
            if (!asset_name) {
                errReason = TRANSLATE_ME("Missing argument");
            }
            else {
                errReason = TRANSLATE_ME("Internal error");
            }
        }
        zmsg_addstr(reply, "ERROR");
        zmsg_addstr(reply, errReason.c_str());
    }
    else {
        zmsg_addstr(reply, "OK");
        zmsg_addstr(reply, result.c_str()); // JSON in one frame
    }
}

// =============================================================================
// TOPOLOGY/LOCATION command processing (completed reply)
// bmsg request asset-agent TOPOLOGY REQUEST <uuid> LOCATION <select_cmd> <assetID> <options>
// <select_cmd> in {"to", "from"}
// see topology_location_process() for allowed options
// =============================================================================

static void s_process_TopologyLocation(const std::string& client_name, const char* select_cmd,const char* asset_name, const char* cmd_options, zmsg_t* reply)
{
    assert (reply);

    log_debug("%s:\tTOPOLOGY LOCATION select_cmd: %s, asset_name: %s (options: %s)", client_name.c_str(),
        select_cmd, asset_name, cmd_options);

    std::string command(select_cmd ? select_cmd : "");
    std::string assetName(asset_name ? asset_name : "");
    std::string options(cmd_options ? cmd_options : "");
    std::string result;    // JSON payload
    std::string errReason; // JSON payload (TRANSLATE_ME)
    int r = topology_location_process(command, assetName, options, result, errReason);

    zmsg_addstr(reply, assetName.c_str());

    if (r != 0) {
        log_error("%s:\tTOPOLOGY LOCATION r: %d (cmd: %s, asset_name: %s, options: %s)", client_name.c_str(),
            r, select_cmd, asset_name, cmd_options);

        if (errReason.empty()) {
            if (!asset_name) {
                errReason = TRANSLATE_ME("Missing argument");
            }
            else {
                errReason = TRANSLATE_ME("Internal error");
            }
        }
        zmsg_addstr(reply, "ERROR");
        zmsg_addstr(reply, errReason.c_str());
    }
    else {
        zmsg_addstr(reply, "OK");
        zmsg_addstr(reply, result.c_str()); // JSON in one frame
    }
}

// =============================================================================
// TOPOLOGY/INPUT_POWERCHAIN command processing (completed reply)
// bmsg request asset-agent TOPOLOGY REQUEST <uuid> INPUT_POWERCHAIN <assetID>
// <assetID> shall be a datacenter
// =============================================================================

static void s_process_TopologyInputPowerchain(
    const std::string& client_name, const char* asset_name, zmsg_t* reply)
{
    assert (reply);

    log_debug("%s:\tTOPOLOGY INPUT_POWERCHAIN asset_name: %s", client_name.c_str(), asset_name);

    std::string assetName(asset_name ? asset_name : "");
    std::string result;    // JSON payload
    std::string errReason; // JSON payload (TRANSLATE_ME)
    int r = topology_input_powerchain_process(assetName, result, errReason);

    zmsg_addstr(reply, assetName.c_str());

    if (r != 0) {
        log_error(
            "%s:\tTOPOLOGY INPUT_POWERCHAIN r: %d (asset_name: %s)", client_name.c_str(), r, asset_name);

        if (errReason.empty()) {
            if (!asset_name) {
                errReason = TRANSLATE_ME("Missing argument");
            }
            else {
                errReason = TRANSLATE_ME("Internal error");
            }
        }
        zmsg_addstr(reply, "ERROR");
        zmsg_addstr(reply, errReason.c_str());
    }
    else {
        zmsg_addstr(reply, "OK");
        zmsg_addstr(reply, result.c_str()); // JSON in one frame
    }
}

// =============================================================================
//         Functionality for TOPOLOGY processing
// =============================================================================
// bmsg request asset-agent TOPOLOGY REQUEST <uuid> POWER <assetID>
// bmsg request asset-agent TOPOLOGY REQUEST <uuid> POWER_TO <assetID>
// bmsg request asset-agent TOPOLOGY REQUEST <uuid> POWERCHAINS <select_cmd> <assetID>
// bmsg request asset-agent TOPOLOGY REQUEST <uuid> LOCATION <select_cmd> <assetID> <options>
// bmsg request asset-agent TOPOLOGY REQUEST <uuid> INPUT_POWERCHAIN <assetID>
// =============================================================================

static void s_handle_subject_topology(const fty::AssetServer& server, zmsg_t* msg)
{
    assert (msg);

    char*   message_type = zmsg_popstr(msg);
    char*   uuid         = zmsg_popstr(msg);
    char*   command      = zmsg_popstr(msg);
    zmsg_t* reply        = zmsg_new();

    const std::string& client_name = server.getAgentName();

    log_debug("%s:\tmessage_type: %s, uuid: %s, command: %s", client_name.c_str(), message_type, uuid, command);

    if (!message_type) {
        log_error("%s:\tExpected message_type for subject=TOPOLOGY", client_name.c_str());
    }
    else if (!uuid) {
        log_error("%s:\tExpected uuid for subject=TOPOLOGY", client_name.c_str());
    }
    else if (!command) {
        log_error("%s:\tExpected command for subject=TOPOLOGY", client_name.c_str());
    }
    else if (!reply) {
        log_error("%s:\tTOPOLOGY %s: reply allocation failed", client_name.c_str(), command);
    }
    else {
        // message model always enforce reply
        zmsg_addstr(reply, uuid);
        zmsg_addstr(reply, "REPLY");
        zmsg_addstr(reply, command);

        if (!streq(message_type, "REQUEST")) {
            log_error("%s:\tExpected REQUEST message_type for subject=TOPOLOGY (message_type: %s)",
                client_name.c_str(), message_type);
            zmsg_addstr(reply, "ERROR"); // status
            // reason, JSON payload (TRANSLATE_ME)
            zmsg_addstr(reply, TRANSLATE_ME("REQUEST_MSGTYPE_EXPECTED (msg type: %s)", message_type).c_str());
        }
        else if (streq(command, "POWER")) {
            char* asset_name = zmsg_popstr(msg);
            s_process_TopologyPower(server.getAgentName(), asset_name, server.getTestMode(), reply);
            zstr_free(&asset_name);
        }
        else if (streq(command, "POWER_TO")) {
            char* asset_name = zmsg_popstr(msg);
            s_process_TopologyPowerTo(server.getAgentName(), asset_name, reply);
            zstr_free(&asset_name);
        }
        else if (streq(command, "POWERCHAINS")) {
            char* select_cmd = zmsg_popstr(msg);
            char* asset_name = zmsg_popstr(msg);
            s_process_TopologyPowerchains(server.getAgentName(), select_cmd, asset_name, reply);
            zstr_free(&asset_name);
            zstr_free(&select_cmd);
        }
        else if (streq(command, "LOCATION")) {
            char* select_cmd = zmsg_popstr(msg);
            char* asset_name = zmsg_popstr(msg);
            char* options    = zmsg_popstr(msg); // can be NULL
            s_process_TopologyLocation(server.getAgentName(), select_cmd, asset_name, options, reply);
            zstr_free(&options);
            zstr_free(&asset_name);
            zstr_free(&select_cmd);
        }
        else if (streq(command, "INPUT_POWERCHAIN")) {
            char* asset_name = zmsg_popstr(msg);
            s_process_TopologyInputPowerchain(server.getAgentName(), asset_name, reply);
            zstr_free(&asset_name);
        }
        else {
            log_error("%s:\tUnexpected command for subject=TOPOLOGY (%s)", client_name.c_str(), command);
            zmsg_addstr(reply, "ERROR"); // status
            // reason, JSON payload (TRANSLATE_ME)
            zmsg_addstr(reply, TRANSLATE_ME("UNEXPECTED_COMMAND (command: %s)", command).c_str());
        }

        // send reply
        const char* sender = mlm_client_sender(mailboxClient(server));
        int r = mlm_client_sendto(mailboxClient(server), sender, "TOPOLOGY", NULL, 5000, &reply);
        if (r != 0) {
            log_error("%s:\tTOPOLOGY (command: %s): mlm_client_sendto failed", client_name.c_str(), command);
        }
    }

    zmsg_destroy(&reply);
    zstr_free(&command);
    zstr_free(&uuid);
    zstr_free(&message_type);
}

static void s_handle_subject_assets_in_container(const fty::AssetServer& server, zmsg_t* msg)
{
    assert (msg);

    const std::string& client_name = server.getAgentName();

    if (zmsg_size(msg) < 2) {
        log_error("%s:\tASSETS_IN_CONTAINER: incoming message have less than 2 frames", client_name.c_str());
        return;
    }

    zmsg_t* reply = zmsg_new();

    {
        char* c_command = zmsg_popstr(msg);
        if (!streq(c_command, "GET")) {
            log_error("%s:\tASSETS_IN_CONTAINER: bad command '%s', expected GET", client_name.c_str(), c_command);
            zmsg_addstr(reply, "ERROR");
            zmsg_addstr(reply, "BAD_COMMAND");
        }
        zstr_free(&c_command);
    }

    // if there is no error, call SQL
    if (zmsg_size(reply) == 0) {
        std::string container_name;
        {
            char* c_container_name = zmsg_popstr(msg);
            container_name = c_container_name ? c_container_name : "";
            zstr_free(&c_container_name);
        }

        std::set<std::string> filters;
        while (zmsg_size(msg) > 0) {
            char* filter = zmsg_popstr(msg);
            filters.insert(filter);
            zstr_free(&filter);
        }

        std::vector<std::string> assets;
        int r = select_assets_by_container(container_name, filters, assets, server.getTestMode());
        if (r == 0) {
            zmsg_addstr(reply, "OK");
            for (const auto& asset : assets) {
                zmsg_addstr(reply, asset.c_str());
            }
        }
        else {
            zmsg_addstr(reply, "ERROR");
            zmsg_addstr(reply, ((r == -2) ? "ASSET_NOT_FOUND" : "INTERNAL_ERROR"));
        }
    }

    // send the reply
    const char* sender = mlm_client_sender(mailboxClient(server));
    int r = mlm_client_sendto(mailboxClient(server), sender, "ASSETS_IN_CONTAINER", NULL, 5000, &reply);

    if (r != 0) {
        log_error("%s:\tASSETS_IN_CONTAINER: mlm_client_sendto failed", client_name.c_str());
    }

    zmsg_destroy(&reply);
}

static void s_handle_subject_ename_from_iname(const fty::AssetServer& server, zmsg_t* msg)
{
    assert (msg);

    const std::string& client_name = server.getAgentName();

    zmsg_t* reply = zmsg_new();

    if (zmsg_size(msg) == 0) {
        log_error("%s:\tENAME_FROM_INAME: incoming message have no frame", client_name.c_str());
        zmsg_addstr(reply, "ERROR");
        zmsg_addstr(reply, "MISSING_INAME");
    }
    else {
        char* iname_str = zmsg_popstr(msg);
        std::string iname(iname_str ? iname_str : "");
        zstr_free(&iname_str);

        std::string ename;
        select_ename_from_iname(iname, ename, server.getTestMode());

        if (ename.empty()) {
            zmsg_addstr(reply, "ERROR");
            zmsg_addstr(reply, "ASSET_NOT_FOUND");
        }
        else {
            zmsg_addstr(reply, "OK");
            zmsg_addstr(reply, ename.c_str());
        }
    }

    // send the reply
    const char* sender = mlm_client_sender(mailboxClient(server));
    int r = mlm_client_sendto(mailboxClient(server), sender, "ENAME_FROM_INAME", NULL, 5000, &reply);

    if (r != 0) {
        log_error("%s:\tENAME_FROM_INAME: mlm_client_sendto failed", client_name.c_str());
    }

    zmsg_destroy(&reply);
}

//TODO rework error processing
static void s_handle_subject_assets(const fty::AssetServer& server, zmsg_t* msg)
{
    assert (msg);

    const std::string& client_name = server.getAgentName();

    zmsg_t* reply = zmsg_new();

    if (zmsg_size(msg) == 0) {
        log_error("%s:\tASSETS: incoming message have no frame", client_name.c_str());
        zmsg_addstr(reply, "0");
        zmsg_addstr(reply, "ERROR");
        zmsg_addstr(reply, "MISSING_COMMAND");
        const char* sender = mlm_client_sender(mailboxClient(server));
        mlm_client_sendto(mailboxClient(server), sender, "ASSETS", NULL, 5000, &reply);
        zmsg_destroy(&reply);
        return;
    }

    char* c_command = zmsg_popstr(msg);
    if (!streq(c_command, "GET")) {
        const char *sender = mlm_client_sender(mailboxClient(server));
        const char *subject = mlm_client_subject(mailboxClient(server));

        log_error("%s:\tASSETS command GET expected (command: %s, sender: %s, subject: %s",
            client_name.c_str(), c_command, sender, subject);

        char* uuid = zmsg_popstr(msg);
        if (uuid) {
            zmsg_addstr(reply, uuid);
        }
        zmsg_addstr(reply, "ERROR");
        zmsg_addstr(reply, "BAD_COMMAND");

        mlm_client_sendto(mailboxClient(server), sender, "ASSETS", NULL, 5000, &reply);

        zstr_free(&c_command);
        zstr_free(&uuid);
        zmsg_destroy(&reply);
        return;
    }
    zstr_free(&c_command);

    char* uuid = zmsg_popstr(msg);

    std::set<std::string> filters;
    while (zmsg_size(msg) > 0) {
        char* filter = zmsg_popstr(msg);
        filters.insert(filter);
        zstr_free(&filter);
    }

    // call SQL
    std::vector<std::string> assets;
    int r = select_assets_by_filter(filters, assets, server.getTestMode());

    zmsg_addstr(reply, uuid); // reply, uuid common frame

    if (r == -1) {
        zmsg_addstr(reply, "ERROR");
        zmsg_addstr(reply, "INTERNAL_ERROR");
    }
    else if (r == -2) {
        zmsg_addstr(reply, "ERROR");
        zmsg_addstr(reply, "ASSET_NOT_FOUND");
    }
    else {
        zmsg_addstr(reply, "OK");
        for (const auto& it : assets) {
            zmsg_addstr(reply, it.c_str());
        }
    }

    // send the reply
    const char* sender = mlm_client_sender(mailboxClient(server));
    r = mlm_client_sendto(mailboxClient(server), sender, "ASSETS", NULL, 5000, &reply);

    if (r != 0) {
        log_error("%s:\tASSETS: mlm_client_sendto failed", client_name.c_str());
    }

    zstr_free(&uuid);
    zmsg_destroy(&reply);
}

// CAUTION: very similar code in asset/src/asset-configure-infrom.cpp::sendConfigure()
// subject changed
static zmsg_t* s_publish_create_or_update_asset_msg(const std::string& client_name,
    const std::string& asset_name, const char* operation, std::string& subject, bool test_mode)
{
    #define CLEANUP do { zhash_destroy(&ext); zhash_destroy(&aux); } while(0)

    zhash_t* aux = zhash_new();
    zhash_t* ext = zhash_new();

    if (!(aux && ext)) {
        log_error("%s:\tMemory allocation failed", client_name.c_str());
        CLEANUP;
        return NULL;
    }
    zhash_autofree(aux);
    zhash_autofree(ext);

    uint32_t asset_id = 0;

    std::function<void(const tntdb::Row&)> cb1 = [aux, &asset_id, asset_name](const tntdb::Row& row) {
        int foo_i = 0;
        row["priority"].get(foo_i);
        zhash_insert(aux, "priority", static_cast<void*>(const_cast<char*>(std::to_string(foo_i).c_str())));

        foo_i = 0;
        row["id_type"].get(foo_i);
        zhash_insert(aux, "type", static_cast<void*>(const_cast<char*>(persist::typeid_to_type(static_cast<uint16_t>(foo_i)).c_str())));

        // additional aux items (requiered by uptime)
        if (streq(persist::typeid_to_type(static_cast<uint16_t>(foo_i)).c_str(), "datacenter")) {
#if 0 // original 2.4.0 (memleaks, get_dc_upses() insert strdup'ed items)
            if (!DBUptime::get_dc_upses(asset_name.c_str(), aux)) {
                log_error("Cannot read upses for dc with id = %s", asset_name.c_str());
            }
#else // no memleaks (use a non autofree interm. zhash on get_dc_upses() call)
            zhash_t* upses = zhash_new();
            if (!DBUptime::get_dc_upses(asset_name.c_str(), upses)) {
                log_error("Cannot read upses for dc with id = %s", asset_name.c_str());
            }
            else {
                zlist_t *keys = zhash_keys(upses);
                for (void* it = zlist_first(keys); it; it = zlist_next(keys)) {
                    const char* key = static_cast<const char*>(it);
                    zhash_freefn(upses, key, free); // to free strdup'ed item
                    zhash_insert(aux, key, zhash_lookup(upses, key)); // aux insert (strdup)
                    //const char* value = static_cast<const char*>(zhash_lookup(upses, key));
                    //log_trace("after get_dc_upses(), aux insert (%s, %s)", key, value);
                }
                zlist_destroy(&keys);
            }
            zhash_destroy(&upses);
#endif
        }

        foo_i = 0;
        row["subtype_id"].get(foo_i);
        zhash_insert(aux, "subtype", static_cast<void*>( const_cast<char*>(persist::subtypeid_to_subtype(static_cast<uint16_t>(foo_i)).c_str())));

        foo_i = 0;
        row["id_parent"].get(foo_i);
        zhash_insert(aux, "parent", static_cast<void*>( const_cast<char*>(std::to_string(foo_i).c_str())));

        std::string foo_s;
        row["status"].get(foo_s);
        zhash_insert(aux, "status", static_cast<void*>( const_cast<char*>(foo_s.c_str())));

        row["id"].get(asset_id);
    };

    // select basic info
    int r = select_asset_element_basic(asset_name, cb1, test_mode);
    if (r != 0) {
        log_warning("%s:\tCannot select info about '%s'", client_name.c_str(), asset_name.c_str());
        CLEANUP;
        return NULL;
    }

    std::function<void(const tntdb::Row&)> cb2 = [ext](const tntdb::Row& row) {
        std::string keytag;
        row["keytag"].get(keytag);
        std::string value;
        row["value"].get(value);
        zhash_insert(ext, keytag.c_str(), static_cast<void*>( const_cast<char*>(value.c_str())));
    };

    // select ext attributes
    r = select_ext_attributes(asset_id, cb2, test_mode);
    if (r != 0) {
        log_warning("%s:\tCannot select ext attributes for '%s'", client_name.c_str(), asset_name.c_str());
        CLEANUP;
        return NULL;
    }

    // handle required but missing ext. attributes (inventory)
    {
        zhash_t* inventory = zhash_new();
        zhash_autofree(inventory);

        // create uuid ext attribute if missing
        if (!zhash_lookup(ext, "uuid")) {
            const char* serial = static_cast<const char*>(zhash_lookup(ext, "serial_no"));
            const char* model  = static_cast<const char*>(zhash_lookup(ext, "model"));
            const char* mfr    = static_cast<const char*>(zhash_lookup(ext, "manufacturer"));

            fty_uuid_t* uuid = fty_uuid_new();
            const char* uuid_new = nullptr;
            if (serial && model && mfr) {
                // we have all information => create uuid
                uuid_new = fty_uuid_calculate(uuid, mfr, model, serial);
            }
            else {
                // generate random uuid
                uuid_new = fty_uuid_generate(uuid);
            }
            zhash_insert(inventory, "uuid", static_cast<void*>( const_cast<char*>(uuid_new)));
            fty_uuid_destroy(&uuid);
        }

        // create timestamp ext attribute if missing
        if (!zhash_lookup(ext, "create_ts")) {
            std::time_t timestamp = std::time(NULL);
            char mbstr[128] = "";
            std::strftime(mbstr, sizeof(mbstr), "%FT%T%z", std::localtime(&timestamp));
            zhash_insert(inventory, "create_ts", static_cast<void*>( const_cast<char*>(mbstr)));
        }

        if (zhash_size(inventory) != 0) {
            // update ext
            for (void* it = zhash_first(inventory); it; it = zhash_next(inventory)) {
                auto keytag = zhash_cursor(inventory);
                auto value = it;
                zhash_insert(ext, keytag, value);
            }

            // update db inventory
            process_insert_inventory(asset_name.c_str(), inventory, true /*readonly*/, test_mode);
        }

        zhash_destroy(&inventory);
    }

    std::function<void(const tntdb::Row&)> cb3 = [aux](const tntdb::Row& row) {
        static const std::vector<std::string> names({
            "parent_name1", "parent_name2", "parent_name3", "parent_name4", "parent_name5",
            "parent_name6", "parent_name7", "parent_name8", "parent_name9", "parent_name10"
        });

        for (const auto& name : names) {
            std::string pname_value;
            row[name].get(pname_value);
            if (pname_value.empty()) {
                continue;
            }
            std::string pname_key = name;
            // 11 == strlen ("parent_name")
            pname_key.insert(11, 1, '.'); // parent_nameX -> parent_name.X
            zhash_insert(aux, pname_key.c_str(), static_cast<void*>(const_cast<char*>(pname_value.c_str())));
        }
    };

    // select "physical topology"
    r = select_asset_element_super_parent(asset_id, cb3, test_mode);
    if (r != 0) {
        log_error("%s:\tselect_asset_element_super_parent ('%s') failed.",
            client_name.c_str(), asset_name.c_str());
        CLEANUP;
        return NULL;
    }

    // set subject
    const char* type = static_cast<const char*>(zhash_lookup(aux, "type"));
    const char* subtype = static_cast<const char*>(zhash_lookup(aux, "subtype"));
    if (!type) { type = "unknown"; }
    if (!subtype) { subtype = "unknown"; }

    subject = std::string{type} + "." + std::string{subtype} + "@" + asset_name;

    // encode the asset
    // other information like groups, powerchain are not included in the message
    zmsg_t* msg = fty_proto_encode_asset(aux, asset_name.c_str(), operation, ext);

    log_debug("notifying ASSETS %s %s", operation, subject.c_str());

    CLEANUP;
    return msg;

    #undef CLEANUP
}

//extern
void send_create_or_update_asset(const fty::AssetServer& server, const std::string& asset_name, const char* operation)
{
    std::string subject{"unknown"}; // changed

    zmsg_t* msg = s_publish_create_or_update_asset_msg(
        server.getAgentName(), asset_name, operation, subject, server.getTestMode());

    if (!msg) {
        log_error("%s:\tpublish failed (asset='%s', op='%s')",
            server.getAgentName().c_str(), asset_name.c_str(), operation);
        return;
    }

    int r = mlm_client_send(const_cast<mlm_client_t*>(server.getStreamClient()), subject.c_str(), &msg);
    if (r != 0) {
        log_error("%s:\tmlm_client_send '%s' failed for asset '%s'",
            server.getAgentName().c_str(), operation, asset_name.c_str());
    }
    zmsg_destroy(&msg);
}

static void s_sendto_create_or_update_asset(const fty::AssetServer& server, const std::string& asset_name,
    const char* operation, const char* address, const char* uuid)
{
    std::string subject{"unknown"}; // changed

    zmsg_t* msg = s_publish_create_or_update_asset_msg(
        server.getAgentName(), asset_name, operation, subject, server.getTestMode());

    if (!msg) {
        log_error("%s:\tASSET_DETAIL: asset not found (%s)",
            server.getAgentName().c_str(), asset_name.c_str());
        msg = zmsg_new();
        zmsg_addstr(msg, "ERROR");
        zmsg_addstr(msg, "ASSET_NOT_FOUND");
    }
    zmsg_pushstr(msg, uuid);

    int r = mlm_client_sendto(mailboxClient(server), address, subject.c_str(), NULL, 5000, &msg);
    if (r != 0) {
        log_error("%s:\tmlm_client_sendto '%s'/'%s' failed for asset '%s'",
            server.getAgentName().c_str(), address, subject.c_str(), asset_name.c_str());
    }
    zmsg_destroy(&msg);
}

static void s_handle_subject_asset_detail(const fty::AssetServer& server, zmsg_t** zmessage_p)
{
    if (!zmessage_p || !*zmessage_p) {
        return;
    }

    zmsg_t* zmessage = *zmessage_p;

    char* c_command = zmsg_popstr(zmessage);
    if (!streq(c_command, "GET")) {
        log_error("%s:\tASSET_DETAIL: bad command '%s', expected GET", server.getAgentName().c_str(), c_command);

        char* uuid  = zmsg_popstr(zmessage);
        zmsg_t* reply = zmsg_new();
        if (uuid) { zmsg_addstr(reply, uuid); }
        zmsg_addstr(reply, "ERROR");
        zmsg_addstr(reply, "BAD_COMMAND");

        const char* sender = mlm_client_sender(mailboxClient(server));
        mlm_client_sendto(mailboxClient(server), sender, "ASSET_DETAIL", NULL, 5000, &reply);

        zstr_free(&uuid);
        zstr_free(&c_command);
        zmsg_destroy(&reply);
        return;
    }
    zstr_free(&c_command);

    // select an asset and publish it through mailbox
    char* uuid = zmsg_popstr(zmessage);
    char* asset_name = zmsg_popstr(zmessage);

    const char* sender = mlm_client_sender(mailboxClient(server));
    s_sendto_create_or_update_asset(server, asset_name, FTY_PROTO_ASSET_OP_UPDATE, sender, uuid);

    zstr_free(&asset_name);
    zstr_free(&uuid);
}

static void s_handle_subject_asset_manipulation(const fty::AssetServer& server, zmsg_t** zmessage_p)
{
    const std::string& client_name = server.getAgentName();

    // Check request format
    if (!zmessage_p || !*zmessage_p) {
        return;
    }
    zmsg_t* zmessage = *zmessage_p;

    bool read_only{false};
    {
        char* read_only_s = zmsg_popstr(zmessage);

        if (read_only_s && streq(read_only_s, "READONLY")) {
            read_only = true;
        }
        else if (read_only_s && streq(read_only_s, "READWRITE")) {
            read_only = false;
        }
        else {
            zmsg_t* reply = zmsg_new();
            zmsg_addstr(reply, "ERROR");
            zmsg_addstr(reply, "BAD_COMMAND");
            const char* sender = mlm_client_sender(mailboxClient(server));
            mlm_client_sendto(mailboxClient(server), sender, "ASSET_MANIPULATION", NULL, 5000, &reply);
            zmsg_destroy(&reply);
            zstr_free(&read_only_s);
            return;
        }
        zstr_free(&read_only_s);
    }

    if (!fty_proto_is(zmessage)) {
        log_error("%s:\tASSET_MANIPULATION: receiver message is not fty_proto", client_name.c_str());
        return;
    }

    fty_proto_t* proto = fty_proto_decode(zmessage_p);
    if (!proto) {
        log_error("%s:\tASSET_MANIPULATION: failed to decode message", client_name.c_str());
        return;
    }

    if (ftylog_getInstance()->isLogDebug()) {
        fty_proto_print(proto);
    }

    const char* operation = fty_proto_operation(proto);

    // process operation from message
    zmsg_t* reply = zmsg_new();
    try {
        // asset manipulation is disabled
        if (server.getGlobalConfigurability() == 0) {
            throw std::runtime_error("Licensing limitation hit - asset manipulation is prohibited.");
        }

        // get asset from fty-proto
        fty::AssetImpl asset;
        fty::Asset::fromFtyProto(proto, asset, read_only, server.getTestMode());
        log_debug(
            "s_handle_subject_asset_manipulation(): Processing operation '%s' "
            "for asset named '%s' with type '%s' and subtype '%s'",
            operation, asset.getInternalName().c_str(), asset.getAssetType().c_str(),
            asset.getAssetSubtype().c_str());

        if (streq(operation, "create") || streq(operation, "create-force")) {
            bool requestActivation = (asset.getAssetStatus() == fty::AssetStatus::Active);
            // create-force -> tryActivate = true
            if (!asset.isActivable()) {
                if (streq(operation, "create-force")) {
                    asset.setAssetStatus(fty::AssetStatus::Nonactive);
                }
                else {
                    throw std::runtime_error(
                        "Licensing limitation hit - maximum amount of active power devices allowed in "
                        "license reached.");
                }
            }

            // store asset to db
            asset.create();

            // activate asset
            if (requestActivation) {
                try {
                    asset.activate();
                }
                catch (const std::exception& e) {
                    // if activation fails, delete asset
                    fty::AssetImpl::deleteList({asset.getInternalName()}, false);
                    throw std::runtime_error(e.what());
                }
            }

            zmsg_addstr(reply, "OK");
            zmsg_addstr(reply, asset.getInternalName().c_str());

            // full notification
            server.sendNotification(FTY_ASSET_SUBJECT_CREATED, fty::Asset::toJson(asset));
        }
        else if (streq(operation, "update")) {
            fty::AssetImpl currentAsset(asset.getInternalName());
            // on update, add link info from current asset:
            //    as fty-proto does not carry link info, they would be deleted on update
            for (const auto& l : currentAsset.getLinkedAssets()) {
                asset.addLink(l.sourceId(), l.srcOut(), l.destIn(), l.linkType(), l.ext());
            }

            // force ID of asset to update
            log_debug("s_handle_subject_asset_manipulation(): Updating asset with internal name %s",
                asset.getInternalName().c_str());

            bool requestActivation = (currentAsset.getAssetStatus() == fty::AssetStatus::Nonactive &&
                                      asset.getAssetStatus() == fty::AssetStatus::Active);
            bool requestDeactivation = (currentAsset.getAssetStatus() == fty::AssetStatus::Active &&
                                  asset.getAssetStatus() == fty::AssetStatus::Nonactive);

            // tryUpdate is not supported in old interface
            if (!asset.isActivable()) {
                throw std::runtime_error(
                    "Licensing limitation hit - maximum amount of active power devices allowed in "
                    "license reached.");
            }

            // store asset to db
            asset.update();

            // activate asset
            if (requestActivation) {
                try {
                    asset.activate();
                }
                catch (const std::exception& e) {
                    // if activation fails, set status to nonactive
                    asset.setAssetStatus(fty::AssetStatus::Nonactive);
                    asset.update();
                    throw std::runtime_error(e.what());
                }
            }
            else if(requestDeactivation) {
                asset.deactivate();
            }

            asset.load();

            zmsg_addstr(reply, "OK");
            zmsg_addstr(reply, asset.getInternalName().c_str());

            // full notification
            cxxtools::SerializationInfo si;
            si.addMember("before") <<= currentAsset;
            si.addMember("after") <<= asset;
            server.sendNotification(FTY_ASSET_SUBJECT_UPDATED, JSON::writeToString(si, false));
        }
        else {
            // unknown op
            log_error("%s:\tASSET_MANIPULATION: operation '%s' unknown", client_name.c_str(), operation);
            zmsg_addstr(reply, "ERROR");
            zmsg_addstr(reply, "OPERATION_NOT_IMPLEMENTED");
        }
    }
    catch (const std::exception& e) {
        log_error("exception reached: %s", e.what());
        fty_proto_print(proto);
        zmsg_destroy(&reply); // reinit for error
        reply = zmsg_new();
        zmsg_addstr(reply, "ERROR");
        zmsg_addstr(reply, e.what());
    }

    const char* sender = mlm_client_sender(mailboxClient(server));
    int r = mlm_client_sendto(mailboxClient(server), sender, "ASSET_MANIPULATION", NULL, 5000, &reply);
    if (r != 0) {
        log_error("ASSET_MANIPULATION/%s: failed to reply to %s", operation, sender);
    }

    zmsg_destroy(&reply);
    fty_proto_destroy(&proto);
}

static void s_update_topology(const fty::AssetServer& server, fty_proto_t* asset)
{
    if (!asset || (fty_proto_id(asset) != FTY_PROTO_ASSET)) {
        return;
    }

    const char* operation = fty_proto_operation(asset);
    const char* name = fty_proto_name(asset); // iname

    if (!streq(operation, FTY_PROTO_ASSET_OP_UPDATE)) {
        log_debug("%s:\tIgnore: '%s' on '%s'", server.getAgentName().c_str(), operation, name);
        return;
    }

    // select assets, that were affected by the change
    std::set<std::string> filters; //empty
    std::vector<std::string> asset_names;
    int r = select_assets_by_container(name, filters, asset_names, server.getTestMode());
    if (r != 0) {
        log_warning("%s:\tCannot select assets in container '%s'", server.getAgentName().c_str(), name);
        return;
    }

    // For every asset we need to form new message!
    for (const auto& asset_name : asset_names) {
        send_create_or_update_asset(server, asset_name, FTY_PROTO_ASSET_OP_UPDATE);
    }
}

static void s_repeat_all(const fty::AssetServer& server, const std::set<std::string>& assets_to_publish)
{
    std::vector<std::string> asset_names;

    std::function<void(const tntdb::Row&)> cb = [&asset_names, &assets_to_publish](const tntdb::Row& row) {
        std::string foo;
        row["name"].get(foo);
        if (assets_to_publish.size() == 0)
            asset_names.push_back(foo);
        else if (assets_to_publish.count(foo) == 1)
            asset_names.push_back(foo);
    };

    // select all assets
    int r = select_assets(cb, server.getTestMode());
    if (r != 0) {
        log_warning("%s:\tCannot list all assets", server.getAgentName().c_str());
        return;
    }

    // send a new message for each asset
    for (const auto& asset_name : asset_names) {
        send_create_or_update_asset(server, asset_name, FTY_PROTO_ASSET_OP_UPDATE);
    }
}

static void s_repeat_all(const fty::AssetServer& server)
{
    return s_repeat_all(server, {});
}

void handle_incoming_limitations(fty::AssetServer& server, fty_proto_t* metric)
{
    // subject matches type.name, so checking those should be sufficient
    if (metric && (fty_proto_id(metric) == FTY_PROTO_METRIC)
        && streq(fty_proto_name(metric), "rackcontroller-0")
        && streq(fty_proto_type(metric), "configurability.global")
    ) {
        log_debug("Setting configurability/global to %s.", fty_proto_value(metric));
        server.setGlobalConfigurability(atoi(fty_proto_value(metric)));
    }
}

void fty_asset_server(zsock_t* pipe, void* args)
{
    if (!args) {
        log_error("args is NULL");
        return;
    }

    // new messagebus interfaces (-ng suffix)
    fty::AssetServer server;
    server.setAgentName(static_cast<char*>(args));
    server.setAgentNameNg(server.getAgentName() + "-ng"); // new-generation interface
    server.setSrrAgentName(server.getAgentName() + "-srr"); // SRR

    zpoller_t* poller = zpoller_new(
        pipe,
        mlm_client_msgpipe(mailboxClient(server)),
        mlm_client_msgpipe(const_cast<mlm_client_t*>(server.getStreamClient())),
        NULL);
    if (!poller) {
        log_error("poller new failed");
        return;
    }

    log_info("%s:\tStarted", server.getAgentName().c_str());

    zsock_signal(pipe, 0);

    while (!zsys_interrupted) {

        const int POLL_TIMEOUT_MS = 30000;
        void* which = zpoller_wait(poller, POLL_TIMEOUT_MS);

        if (which == NULL) {
            if (zpoller_terminated(poller) || zsys_interrupted) {
                break;
            }
        }
        else if (which == pipe) {
            //log_debug("which = PIPE"); }
            zmsg_t* msg = zmsg_recv(pipe);
            char* cmd = zmsg_popstr(msg);
            bool term{false};

            log_debug("%s:\tActor command=%s", server.getAgentName().c_str(), cmd);

            if (streq(cmd, "$TERM")) {
                log_info("%s:\tGot $TERM", server.getAgentName().c_str());
                term = true;
            }
            else if (streq(cmd, "CONNECTSTREAM")) {
                char* endpoint = zmsg_popstr(msg);
                server.setStreamEndpoint(endpoint);

                char* stream_name = zsys_sprintf("%s-stream", server.getAgentName().c_str());
                int r = mlm_client_connect(const_cast<mlm_client_t*>(server.getStreamClient()),
                    server.getStreamEndpoint().c_str(), 1000, stream_name);
                if (r != 0) {
                    log_error("%s:\tCan't connect to malamute endpoint '%s'",
                        stream_name, server.getStreamEndpoint().c_str());
                }

                // new interface
                server.createPublisherClientNg(); // notifications
                server.connectPublisherClientNg();

                zstr_free(&endpoint);
                zstr_free(&stream_name);
                zsock_signal(pipe, 0);
            }
            else if (streq(cmd, "PRODUCER")) {
                char* stream = zmsg_popstr(msg);
                server.setTestMode(streq(stream, "ASSETS-TEST"));
                int r = mlm_client_set_producer(const_cast<mlm_client_t*>(server.getStreamClient()), stream);
                if (r != 0) {
                    log_error("%s:\tCan't set producer on stream '%s'",
                        server.getAgentName().c_str(), stream);
                }
                zstr_free(&stream);
                zsock_signal(pipe, 0);
            }
            else if (streq(cmd, "CONSUMER")) {
                char* stream  = zmsg_popstr(msg);
                char* pattern = zmsg_popstr(msg);
                int r = mlm_client_set_consumer(const_cast<mlm_client_t*>(server.getStreamClient()), stream, pattern);
                if (r != 0) {
                    log_error("%s:\tCan't set consumer on stream '%s', '%s'",
                        server.getAgentName().c_str(), stream, pattern);
                }
                zstr_free(&pattern);
                zstr_free(&stream);
                zsock_signal(pipe, 0);
            }
            else if (streq(cmd, "CONNECTMAILBOX")) {
                char* endpoint = zmsg_popstr(msg);
                server.setMailboxEndpoint(endpoint);
                server.setSrrEndpoint(endpoint);

                int r = mlm_client_connect(mailboxClient(server),
                    server.getMailboxEndpoint().c_str(), 1000, server.getAgentName().c_str());
                if (r != 0) {
                    log_error("%s:\tCan't connect to malamute endpoint '%s'",
                        server.getAgentName().c_str(), server.getMailboxEndpoint().c_str());
                }

                // new interface
                server.createMailboxClientNg(); // queue
                server.connectMailboxClientNg();
                server.receiveMailboxClientNg(FTY_ASSET_MAILBOX);
                server.initSrr(FTY_ASSET_SRR_QUEUE);

                zstr_free(&endpoint);
                zsock_signal(pipe, 0);
            }
            else if (streq(cmd, "REPEAT_ALL")) {
                s_repeat_all(server);
                log_debug("%s:\tREPEAT_ALL end", server.getAgentName().c_str());
            }
            else {
                log_info("%s:\tUnhandled command %s", server.getAgentName().c_str(), cmd);
            }
            zstr_free(&cmd);
            zmsg_destroy(&msg);
            if (term) {
                break;
            }
        }
        // handle MAILBOX DELIVER messages
        else if (which == mlm_client_msgpipe(mailboxClient(server))) {
            zmsg_t* zmessage = mlm_client_recv(mailboxClient(server));
            const char* sender = mlm_client_sender(mailboxClient(server));
            const char* subject = mlm_client_subject(mailboxClient(server));

            log_info("%s:\tMAILBOX DELIVER (sender: %s, subject: %s)", server.getAgentName().c_str(), sender, subject);

            if (!zmessage) {
                //nop
            }
            else if (streq(subject,"TOPOLOGY")) {
                s_handle_subject_topology(server, zmessage);
            }
            else if (streq(subject,"ASSETS_IN_CONTAINER")) {
                s_handle_subject_assets_in_container(server, zmessage);
            }
            else if (streq(subject,"ASSETS")) {
                s_handle_subject_assets(server, zmessage);
            }
            else if (streq(subject,"ENAME_FROM_INAME")) {
                s_handle_subject_ename_from_iname(server, zmessage);
            }
            else if (streq(subject,"REPUBLISH")) {

                char* asset = zmsg_popstr(zmessage);
                if (!asset || streq(asset, "$all")) {
                    s_repeat_all(server);
                }
                else {
                    std::set<std::string> assets_to_publish;
                    while (asset) {
                        assets_to_publish.insert(asset);
                        zstr_free(&asset);
                        asset = zmsg_popstr(zmessage);
                    }
                    s_repeat_all(server, assets_to_publish);
                }
                zstr_free(&asset);
                // do not reply anything (requesters don't expect an answer)

            }
            else if (streq(subject,"ASSET_MANIPULATION")) {
                s_handle_subject_asset_manipulation(server, &zmessage);
            }
            else if (streq(subject,"ASSET_DETAIL")) {
                s_handle_subject_asset_detail(server, &zmessage);
            }
            else {
                log_info("%s:\tUnexpected subject '%s'", server.getAgentName().c_str(), subject);
            }
            zmsg_destroy(&zmessage);
        }
        // handle STREAM_DELIVER messages
        else if (which == mlm_client_msgpipe(const_cast<mlm_client_t*>(server.getStreamClient()))) {
            zmsg_t* zmessage = mlm_client_recv(const_cast<mlm_client_t*>(server.getStreamClient()));

            if (zmessage && fty_proto_is(zmessage)) {
                fty_proto_t* proto = fty_proto_decode(&zmessage);

                if (fty_proto_id(proto) == FTY_PROTO_ASSET) {
                    log_debug("%s:\tSTREAM DELIVER (PROTO_ASSET)", server.getAgentName().c_str());
                    s_update_topology(server, proto);
                }
                else if (fty_proto_id(proto) == FTY_PROTO_METRIC) {
                    log_debug("%s:\tSTREAM DELIVER (PROTO_METRIC)", server.getAgentName().c_str());
                    handle_incoming_limitations(server, proto);
                }
                fty_proto_destroy(&proto);
            }
            zmsg_destroy(&zmessage);
        }
    }

    log_info("%s:\tended", server.getAgentName().c_str());

    zpoller_destroy(&poller);
}
