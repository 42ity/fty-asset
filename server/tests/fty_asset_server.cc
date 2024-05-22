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

#include <catch2/catch.hpp>
#include "fty_asset_server.h"
#include "fty_asset_autoupdate.h"
#include "../src/asset-server.h"
#include "../src/test_str.h"
#include <fty_common_messagebus.h>
#include <fty_proto.h>
#include <malamute.h>
#include <czmq.h>
#include <iostream>
#include <sstream>

// stores correlationID : asset JSON for each message received
static std::map<std::string, std::string> assetTestMap;

// IMPORTANT NOTE:
// here, we need privileges to write in DB (runtime)
// we only logs success or error... no catch2 macros here
static void s_test_asset_mailbox_handler(const messagebus::Message& msg)
{
    std::function<std::string(const messagebus::Message&)> msg2str = [] (const messagebus::Message& m)
    {
        std::ostringstream oss;
        oss << "metaData (" << m.metaData().size() << "):" << std::endl;
        for (const auto& i : m.metaData()) {
            oss << i.first << ": " << i.second << std::endl;
        }
        oss << "userData (" << m.userData().size() << "):" << std::endl;
        for (const auto& s : m.userData()) {
            oss << s << std::endl;
        }
        return oss.str();
    };

    std::cout << "s_test_asset_mailbox_handler" << std::endl;
    std::cout << "msg: " << msg2str(msg) << std::endl;

    try {
        std::string msgSubject = msg.metaData().find(messagebus::Message::SUBJECT)->second;
        if (msgSubject == FTY_ASSET_SUBJECT_CREATE) {
            fty::Asset msgAsset;
            fty::Asset::fromJson(msg.userData().back(), msgAsset);

            fty::Asset mapAsset;
            fty::Asset::fromJson(
                assetTestMap.find(msg.metaData().find(messagebus::Message::CORRELATION_ID)->second)->second,
                mapAsset);

            std::cout << "msgAsset: " << std::endl << fty::Asset::toJson(msgAsset) << std::endl;
            std::cout << "mapAsset: " << std::endl << fty::Asset::toJson(mapAsset) << std::endl;

            if (msgAsset.getInternalName() == mapAsset.getInternalName()) {
                log_info("fty-asset-server-test:Test #15.1: OK");
            } else {
                log_error("fty-asset-server-test:Test #15.1: FAILED");
            }
        } else if (msgSubject == FTY_ASSET_SUBJECT_UPDATE) {
            fty::Asset msgAsset;
            fty::Asset::fromJson(msg.userData().back(), msgAsset);

            fty::Asset mapAsset;
            fty::Asset::fromJson(
                assetTestMap.find(msg.metaData().find(messagebus::Message::CORRELATION_ID)->second)->second,
                mapAsset);

            std::cout << "msgAsset: " << std::endl << fty::Asset::toJson(msgAsset) << std::endl;
            std::cout << "mapAsset: " << std::endl << fty::Asset::toJson(mapAsset) << std::endl;

            if (msgAsset.getInternalName() == mapAsset.getInternalName()) {
                log_info("fty-asset-server-test:Test #15.2: OK");
            } else {
                log_error("fty-asset-server-test:Test #15.2: FAILED");
            }
        } else if (msgSubject == FTY_ASSET_SUBJECT_GET) {
            std::string assetJson = msg.userData().back();
            fty::Asset  msgAsset;
            fty::Asset::fromJson(assetJson, msgAsset);

            std::cout << "msgAsset: " << std::endl << fty::Asset::toJson(msgAsset) << std::endl;

            std::string assetName = msgAsset.getInternalName();

            if (assetTestMap.find(msg.metaData().find(messagebus::Message::CORRELATION_ID)->second)->second ==
                assetName) {
                log_info("fty-asset-server-test:Test #15.3: OK");
            } else {
                log_error("fty-asset-server-test:Test #15.3: FAILED");
            }
        } else {
            log_error("fty-asset-server-test:Invalid subject %s", msgSubject.c_str());
        }
    } catch (const std::exception& e) {
        log_error("s_test_asset_mailbox_handler (e: %s)", e.what());
    }
}

TEST_CASE("fty_asset_server test")
{
    log_debug("Setting test mode to true");
    g_testMode = true;

    std::cout << " * fty_asset_server:" << std::endl;

    //  @selftest
    // Test #1:  Simple create/destroy test
    std::cout << "fty-asset-server-test:Test #1" << std::endl;
    {
        fty::AssetServer server;
    }
    std::cout << "fty-asset-server-test:Test #1: OK" << std::endl;

    std::string rnd_name = "1234";
    {
        timeval t;
        gettimeofday(&t, NULL);
        srand(static_cast<unsigned int>(t.tv_sec * t.tv_usec));
        std::cerr << "################### " << t.tv_sec * t.tv_usec << std::endl;
        rnd_name = std::to_string(rand());
    }

    const std::string endpoint = "inproc://fty_asset_server-test";
    const std::string client_name = "fty-asset-" + rnd_name;
    const std::string asset_server_test_name = "asset_agent_test-" + rnd_name;
    std::cout << "endpoint: " << endpoint << std::endl;
    std::cout << "client_name: " << client_name << std::endl;
    std::cout << "asset_server_test_name: " << asset_server_test_name << std::endl;

    std::cout << "create server..." << std::endl;
    zactor_t* server = zactor_new(mlm_server, static_cast<void*>( const_cast<char*>("Malamute")));
    CHECK(server != NULL);
    zstr_sendx(server, "BIND", endpoint.c_str(), NULL);

    std::cout << "create ui..." << std::endl;
    mlm_client_t* ui = mlm_client_new();
    CHECK(ui != NULL);
    mlm_client_connect(ui, endpoint.c_str(), 5000, client_name.c_str());
    mlm_client_set_producer(ui, "ASSETS-TEST");
    mlm_client_set_consumer(ui, "ASSETS-TEST", ".*");

    std::cout << "create asset_server..." << std::endl;
    zactor_t* asset_server = zactor_new(fty_asset_server, static_cast<void*>( const_cast<char*>(asset_server_test_name.c_str())));
    CHECK(asset_server != NULL);
    zstr_sendx(asset_server, "CONNECTSTREAM", endpoint.c_str(), NULL);
    zsock_wait(asset_server);
    zstr_sendx(asset_server, "PRODUCER", "ASSETS-TEST", NULL);
    zsock_wait(asset_server);
    zstr_sendx(asset_server, "CONSUMER", "ASSETS-TEST", ".*", NULL);
    zsock_wait(asset_server);
    zstr_sendx(asset_server, "CONSUMER", "LICENSING-ANNOUNCEMENTS-TEST", ".*", NULL);
    zsock_wait(asset_server);
    zstr_sendx(asset_server, "CONNECTMAILBOX", endpoint.c_str(), NULL);
    zsock_wait(asset_server);

    static const char* asset_name = TEST_INAME;

    std::cout << "Test #2: subject ASSET_MANIPULATION, message fty_proto_t *asset" << std::endl;
    {
        log_debug("fty-asset-server-test:Test #2");
        const char* subject = "ASSET_MANIPULATION";
        zhash_t*    aux     = zhash_new();
        zhash_insert(aux, "type", static_cast<void*>( const_cast<char*>("datacenter")));
        zhash_insert(aux, "subtype", static_cast<void*>( const_cast<char*>("N_A")));
        zmsg_t* msg = fty_proto_encode_asset(aux, asset_name, FTY_PROTO_ASSET_OP_CREATE, NULL);
        zmsg_pushstrf(msg, "%s", "READWRITE");
        int rv = mlm_client_sendto(ui, asset_server_test_name.c_str(), subject, NULL, 5000, &msg);
        zhash_destroy(&aux);
        CHECK(rv == 0);
        zmsg_t* reply = mlm_client_recv(ui);
        if (!fty_proto_is(reply)) {
            CHECK(streq(mlm_client_subject(ui), subject));
            CHECK(zmsg_size(reply) == 2);
            char* str = zmsg_popstr(reply);
            CHECK(streq(str, "OK"));
            zstr_free(&str);
            str = zmsg_popstr(reply);
            // CHECK(streq(str, asset_name));
            zstr_free(&str);
            zmsg_destroy(&reply);
        } else {
            CHECK(fty_proto_is(reply));
            fty_proto_t* fmsg             = fty_proto_decode(&reply);
            std::string  expected_subject = "unknown.unknown@";
            expected_subject.append(asset_name);
            // CHECK(streq(mlm_client_subject(ui), expected_subject.c_str()));
            CHECK(streq(fty_proto_operation(fmsg), FTY_PROTO_ASSET_OP_CREATE));
            fty_proto_destroy(&fmsg);
            zmsg_destroy(&reply);
        }

        reply = mlm_client_recv(ui);
        if (!fty_proto_is(reply)) {
            CHECK(streq(mlm_client_subject(ui), subject));
            CHECK(zmsg_size(reply) == 2);
            char* str = zmsg_popstr(reply);
            CHECK(streq(str, "OK"));
            zstr_free(&str);
            str = zmsg_popstr(reply);
            // CHECK(streq(str, asset_name));
            zstr_free(&str);
            zmsg_destroy(&reply);
        } else {
            CHECK(fty_proto_is(reply));
            fty_proto_t* fmsg             = fty_proto_decode(&reply);
            std::string  expected_subject = "unknown.unknown@";
            expected_subject.append(asset_name);
            // CHECK(streq(mlm_client_subject(ui), expected_subject.c_str()));
            CHECK(streq(fty_proto_operation(fmsg), FTY_PROTO_ASSET_OP_CREATE));
            fty_proto_destroy(&fmsg);
            zmsg_destroy(&reply);
        }
        log_info("fty-asset-server-test:Test #2: OK");
    }

    std::cout << "Test #3: message fty_proto_t *asset" << std::endl;
    {
        log_debug("fty-asset-server-test:Test #3");
        zmsg_t* msg = fty_proto_encode_asset(NULL, asset_name, FTY_PROTO_ASSET_OP_UPDATE, NULL);
        int rv  = mlm_client_send(ui, "update-test", &msg);
        CHECK(rv == 0);
        zclock_sleep(200);
        log_info("fty-asset-server-test:Test #3: OK");
    }

    std::cout << "Test #4: subject TOPOLOGY, message POWER" << std::endl;
    {
        log_debug("fty-asset-server-test:Test #4");
        const char* subject = "TOPOLOGY";
        const char* command = "POWER";
        const char* uuid    = "123456";
        zmsg_t*     msg     = zmsg_new();
        zmsg_addstr(msg, "REQUEST");
        zmsg_addstr(msg, uuid);
        zmsg_addstr(msg, command);
        zmsg_addstr(msg, asset_name);
        int rv = mlm_client_sendto(ui, asset_server_test_name.c_str(), subject, NULL, 5000, &msg);
        CHECK(rv == 0);
        zmsg_t* reply = mlm_client_recv(ui);
        CHECK(streq(mlm_client_subject(ui), subject));
        CHECK(zmsg_size(reply) == 5);
        char* str = zmsg_popstr(reply);
        CHECK(streq(str, uuid));
        zstr_free(&str);
        str = zmsg_popstr(reply);
        CHECK(streq(str, "REPLY"));
        zstr_free(&str);
        str = zmsg_popstr(reply);
        CHECK(streq(str, command));
        zstr_free(&str);
        str = zmsg_popstr(reply);
        // CHECK(streq(str, asset_name));
        zstr_free(&str);
        str = zmsg_popstr(reply);
        CHECK(streq(str, "OK"));
        zstr_free(&str);
        zmsg_destroy(&reply);
        log_info("fty-asset-server-test:Test #4: OK");
    }
    std::cout << "Test #5: subject ASSETS_IN_CONTAINER, message GET" << std::endl;
    {
        log_debug("fty-asset-server-test:Test #5");
        const char* subject = "ASSETS_IN_CONTAINER";
        const char* command = "GET";
        zmsg_t*     msg     = zmsg_new();
        zmsg_addstr(msg, command);
        zmsg_addstr(msg, asset_name);
        int rv = mlm_client_sendto(ui, asset_server_test_name.c_str(), subject, NULL, 5000, &msg);
        CHECK(rv == 0);
        zmsg_t* reply = mlm_client_recv(ui);
        CHECK(streq(mlm_client_subject(ui), subject));
        CHECK(zmsg_size(reply) == 1);
        char* str = zmsg_popstr(reply);
        CHECK(streq(str, "OK"));
        zstr_free(&str);
        zmsg_destroy(&reply);
        log_info("fty-asset-server-test:Test #5: OK");
    }
    std::cout << "Test #6: subject ASSETS, message GET" << std::endl;
    {
        log_debug("fty-asset-server-test:Test #6");
        const char* subject = "ASSETS";
        const char* command = "GET";
        zmsg_t*     msg     = zmsg_new();
        zmsg_addstr(msg, command);
        zmsg_addstr(msg, "UUID");
        zmsg_addstr(msg, asset_name);
        int rv = mlm_client_sendto(ui, asset_server_test_name.c_str(), subject, NULL, 5000, &msg);
        CHECK(rv == 0);
        zmsg_t* reply = mlm_client_recv(ui);
        CHECK(streq(mlm_client_subject(ui), subject));
        CHECK(zmsg_size(reply) == 2);
        char* uuid = zmsg_popstr(reply);
        CHECK(streq(uuid, "UUID"));
        char* str = zmsg_popstr(reply);
        CHECK(streq(str, "OK"));
        zstr_free(&str);
        zstr_free(&uuid);
        zmsg_destroy(&reply);
        log_info("fty-asset-server-test:Test #6: OK");
    }
    std::cout << "Test #7: message REPEAT_ALL" << std::endl;
    {
        log_debug("fty-asset-server-test:Test #7");
        const char* command = "REPEAT_ALL";
        int rv      = zstr_sendx(asset_server, command, NULL);
        CHECK(rv == 0);
        zclock_sleep(200);
        log_info("fty-asset-server-test:Test #7: OK");
    }
    std::cout << "Test #8: subject REPUBLISH, message $all" << std::endl;
    {
        log_debug("fty-asset-server-test:Test #8");
        const char* subject = "REPUBLISH";
        zmsg_t*     msg     = zmsg_new();
        zmsg_addstr(msg, "$all");
        int rv = mlm_client_sendto(ui, asset_server_test_name.c_str(), subject, NULL, 5000, &msg);
        CHECK(rv == 0);
        zclock_sleep(200);
        log_info("fty-asset-server-test:Test #8: OK");
    }
    std::cout << "Test #9: subject ASSET_DETAIL, message GET/<iname>" << std::endl;
    {
        log_debug("fty-asset-server-test:Test #9");
        const char* subject = "ASSET_DETAIL";
        const char* command = "GET";
        const char* uuid    = "UUID-0000-TEST";
        zmsg_t*     msg     = zmsg_new();
        zmsg_addstr(msg, command);
        zmsg_addstr(msg, uuid);
        zmsg_addstr(msg, asset_name);
        int rv = mlm_client_sendto(ui, asset_server_test_name.c_str(), subject, NULL, 5000, &msg);
        CHECK(rv == 0);
        zmsg_t* reply    = mlm_client_recv(ui);
        char*   rcv_uuid = zmsg_popstr(reply);
        CHECK(0 == strcmp(rcv_uuid, uuid));
        CHECK(fty_proto_is(reply));
        fty_proto_t* freply = fty_proto_decode(&reply);
        const char*  str    = fty_proto_name(freply);
        // CHECK(streq(str, asset_name));
        str = fty_proto_operation(freply);
        CHECK(streq(str, FTY_PROTO_ASSET_OP_UPDATE));
        fty_proto_destroy(&freply);
        zstr_free(&rcv_uuid);
        log_info("fty-asset-server-test:Test #9: OK");
    }

    std::cout << "Test #10: subject ENAME_FROM_INAME, message <iname>" << std::endl;
    {
        log_debug("fty-asset-server-test:Test #10");
        const char* subject     = "ENAME_FROM_INAME";
        zmsg_t*     msg         = zmsg_new();
        zmsg_addstr(msg, asset_name);
        int rv = mlm_client_sendto(ui, asset_server_test_name.c_str(), subject, NULL, 5000, &msg);
        CHECK(rv == 0);
        zmsg_t* reply = mlm_client_recv(ui);
        CHECK(zmsg_size(reply) == 2);
        char* str = zmsg_popstr(reply);
        CHECK(streq(str, "OK"));
        zstr_free(&str);
        str = zmsg_popstr(reply);
        CHECK(streq(str, TEST_ENAME));
        zstr_free(&str);
        zmsg_destroy(&reply);
        log_info("fty-asset-server-test:Test #10: OK");
    }

    zactor_t* autoupdate_server = zactor_new(fty_asset_autoupdate_server, static_cast<void*>( const_cast<char*>("asset-autoupdate-test")));
    zstr_sendx(autoupdate_server, "CONNECT", endpoint.c_str(), NULL);
    zsock_wait(autoupdate_server);
    zstr_sendx(autoupdate_server, "PRODUCER", "ASSETS-TEST", NULL);
    zsock_wait(autoupdate_server);
    zstr_sendx(autoupdate_server, "ASSET_AGENT_NAME", asset_server_test_name.c_str(), NULL);

    std::cout << "Test #11: message WAKEUP" << std::endl;
    {
        log_debug("fty-asset-server-test:Test #11");
        const char* command = "WAKEUP";
        int rv      = zstr_sendx(autoupdate_server, command, NULL);
        CHECK(rv == 0);
        zclock_sleep(200);
        log_info("fty-asset-server-test:Test #11: OK");
    }

    std::cout << "Test #12: test licensing limitations" << std::endl;
    {
        log_debug("fty-asset-server-test:Test #12");
        // try to create asset when configurability is enabled
        const char* subject = "ASSET_MANIPULATION";
        zhash_t*    aux     = zhash_new();
        zhash_insert(aux, "type", static_cast<void*>( const_cast<char*>("datacenter")));
        zhash_insert(aux, "subtype", static_cast<void*>( const_cast<char*>("N_A")));
        zmsg_t* msg = fty_proto_encode_asset(aux, asset_name, FTY_PROTO_ASSET_OP_CREATE, NULL);
        zmsg_pushstrf(msg, "%s", "READWRITE");
        int rv = mlm_client_sendto(ui, asset_server_test_name.c_str(), subject, NULL, 5000, &msg);
        zclock_sleep(200);
        zhash_destroy(&aux);
        CHECK(rv == 0);
        char*   str   = NULL;
        zmsg_t* reply = mlm_client_recv(ui);
        if (!fty_proto_is(reply)) {
            CHECK(streq(mlm_client_subject(ui), subject));
            CHECK(zmsg_size(reply) == 2);
            str = zmsg_popstr(reply);
            CHECK(streq(str, "OK"));
            zstr_free(&str);
        }
        zmsg_destroy(&reply); // throw away stream message

        reply = mlm_client_recv(ui);
        if (!fty_proto_is(reply)) {
            CHECK(streq(mlm_client_subject(ui), subject));
            CHECK(zmsg_size(reply) == 2);
            str = zmsg_popstr(reply);
            CHECK(streq(str, "OK"));
            zstr_free(&str);
        }
        zmsg_destroy(&reply); // throw away stream message

        // disable configurability
        mlm_client_set_producer(ui, "LICENSING-ANNOUNCEMENTS-TEST");
        zmsg_t* smsg = fty_proto_encode_metric(
            NULL, static_cast<uint64_t>(time(NULL)), 24 * 60 * 60, "configurability.global", "rackcontroller-0", "0", "");
        mlm_client_send(ui, "configurability.global@rackcontroller-0", &smsg);
        zclock_sleep(200);
        // try to create asset when configurability is disabled
        aux = zhash_new();
        zhash_insert(aux, "type", static_cast<void*>( const_cast<char*>("datacenter")));
        zhash_insert(aux, "subtype", static_cast<void*>( const_cast<char*>("N_A")));
        msg = fty_proto_encode_asset(aux, asset_name, FTY_PROTO_ASSET_OP_CREATE, NULL);
        zmsg_pushstrf(msg, "%s", "READWRITE");
        rv = mlm_client_sendto(ui, asset_server_test_name.c_str(), subject, NULL, 5000, &msg);
        zclock_sleep(200);
        zhash_destroy(&aux);
        CHECK(rv == 0);
        reply = mlm_client_recv(ui);
        CHECK(streq(mlm_client_subject(ui), subject));
        CHECK(zmsg_size(reply) == 2);
        str = zmsg_popstr(reply);
        CHECK(streq(str, "ERROR"));
        zstr_free(&str);
        str = zmsg_popstr(reply);
        CHECK(streq(str, "Licensing limitation hit - asset manipulation is prohibited."));
        zstr_free(&str);
        zmsg_destroy(&reply);
        // enable configurability again, but set limit to power devices
        smsg = fty_proto_encode_metric(
            NULL, static_cast<uint64_t>(time(NULL)), 24 * 60 * 60, "configurability.global", "rackcontroller-0", "1", "");
        mlm_client_send(ui, "configurability.global@rackcontroller-0", &smsg);
        smsg = fty_proto_encode_metric(
            NULL, static_cast<uint64_t>(time(NULL)), 24 * 60 * 60, "power_nodes.max_active", "rackcontroller-0", "3", "");
        mlm_client_send(ui, "power_nodes.max_active@rackcontroller-0", &smsg);
        zclock_sleep(300);
        // send power devices
        aux = zhash_new();
        zhash_insert(aux, "type", static_cast<void*>( const_cast<char*>("device")));
        zhash_insert(aux, "subtype", static_cast<void*>( const_cast<char*>("epdu")));
        zhash_insert(aux, "status", static_cast<void*>( const_cast<char*>("active")));
        msg = fty_proto_encode_asset(aux, "test1", FTY_PROTO_ASSET_OP_UPDATE, NULL);
        zmsg_pushstrf(msg, "%s", "READWRITE");
        rv = mlm_client_sendto(ui, asset_server_test_name.c_str(), subject, NULL, 5000, &msg);
        if (aux)
            zhash_destroy(&aux);
        zclock_sleep(200);
        CHECK(rv == 0);
        reply = mlm_client_recv(ui);
        if (!fty_proto_is(reply)) {
            CHECK(streq(mlm_client_subject(ui), subject));
            CHECK(zmsg_size(reply) == 2);
            str = zmsg_popstr(reply);
            CHECK(streq(str, "OK"));
            zstr_free(&str);
        }
        zmsg_destroy(&reply); // throw away stream message

        reply = mlm_client_recv(ui);
        if (!fty_proto_is(reply)) {
            CHECK(streq(mlm_client_subject(ui), subject));
            CHECK(zmsg_size(reply) == 2);
            str = zmsg_popstr(reply);
            CHECK(streq(str, "OK"));
            zstr_free(&str);
        }
        zmsg_destroy(&reply); // throw away stream message

        aux = zhash_new();
        zhash_autofree(aux);
        zhash_insert(aux, "type", static_cast<void*>( const_cast<char*>("device")));
        zhash_insert(aux, "subtype", static_cast<void*>( const_cast<char*>("epdu")));
        zhash_insert(aux, "status", static_cast<void*>( const_cast<char*>("active")));
        msg = fty_proto_encode_asset(aux, "test2", FTY_PROTO_ASSET_OP_UPDATE, NULL);
        zmsg_pushstrf(msg, "%s", "READWRITE");
        rv = mlm_client_sendto(ui, asset_server_test_name.c_str(), subject, NULL, 5000, &msg);
        if (aux)
            zhash_destroy(&aux);
        zclock_sleep(1000);
        CHECK(rv == 0);
        reply = mlm_client_recv(ui);
        if (!fty_proto_is(reply)) {
            CHECK(streq(mlm_client_subject(ui), subject));
            CHECK(zmsg_size(reply) == 2);
            str = zmsg_popstr(reply);
            CHECK(streq(str, "OK"));
            zstr_free(&str);
        }
        zmsg_destroy(&reply); // throw away stream message

        reply = mlm_client_recv(ui);
        if (!fty_proto_is(reply)) {
            CHECK(streq(mlm_client_subject(ui), subject));
            CHECK(zmsg_size(reply) == 2);
            str = zmsg_popstr(reply);
            CHECK(streq(str, "OK"));
            zstr_free(&str);
        }
        zmsg_destroy(&reply); // throw away stream message

        aux = zhash_new();
        zhash_autofree(aux);
        zhash_insert(aux, "type", static_cast<void*>( const_cast<char*>("device")));
        zhash_insert(aux, "subtype", static_cast<void*>( const_cast<char*>("epdu")));
        zhash_insert(aux, "status", static_cast<void*>( const_cast<char*>("active")));
        msg = fty_proto_encode_asset(aux, "test3", FTY_PROTO_ASSET_OP_UPDATE, NULL);
        zmsg_pushstrf(msg, "%s", "READWRITE");
        rv = mlm_client_sendto(ui, asset_server_test_name.c_str(), subject, NULL, 5000, &msg);
        if (aux)
            zhash_destroy(&aux);
        zclock_sleep(200);
        CHECK(rv == 0);

        reply = mlm_client_recv(ui);
        if (!fty_proto_is(reply)) {
            CHECK(streq(mlm_client_subject(ui), subject));
            CHECK(zmsg_size(reply) == 2);
            str = zmsg_popstr(reply);
            CHECK(streq(str, "OK"));
            zstr_free(&str);
        }
        zmsg_destroy(&reply); // throw away stream message

        reply = mlm_client_recv(ui);
        if (!fty_proto_is(reply)) {
            CHECK(streq(mlm_client_subject(ui), subject));
            CHECK(zmsg_size(reply) == 2);
            str = zmsg_popstr(reply);
            CHECK(streq(str, "OK"));
            zstr_free(&str);
        }
        zmsg_destroy(&reply); // throw away stream message
    }

    std::cout << "Test #13: asset conversion to json" << std::endl;
    {
        log_debug("fty-asset-server-test:Test #13");

        fty::Asset asset;
        asset.setInternalName("dc-0");
        asset.setAssetStatus(fty::AssetStatus::Nonactive);
        asset.setAssetType(fty::TYPE_DEVICE);
        asset.setAssetSubtype(fty::SUB_UPS);
        asset.setParentIname("abc123");
        asset.setExtEntry("testKey", "testValue");
        asset.setPriority(4);

        std::string jsonStr = fty::Asset::toJson(asset);

        fty::Asset asset2;
        fty::Asset::fromJson(jsonStr, asset2);

        CHECK(asset == asset2);

        log_debug("fty-asset-server-test:Test #13 OK");
    }

    std::cout << "Test #14: asset conversion to fty-proto" << std::endl;
    {
        log_debug("fty-asset-server-test:Test #14");

        fty::Asset asset;
        asset.setInternalName("dc-0");
        asset.setAssetStatus(fty::AssetStatus::Nonactive);
        asset.setAssetType(fty::TYPE_DEVICE);
        asset.setAssetSubtype(fty::SUB_UPS);
        asset.setParentIname("test-parent");
        asset.setExtEntry("testKey", "testValue");
        asset.setPriority(4);

        asset.dump(std::cout);

        fty_proto_t* p = fty::Asset::toFtyProto(asset, "UPDATE", true);

        fty::Asset asset2;

        fty::Asset::fromFtyProto(p, asset2, false, true);
        fty_proto_destroy(&p);

        asset2.dump(std::cout);

        CHECK(asset == asset2);

        log_debug("fty-asset-server-test:Test #14 OK");
    }

    std::cout << "Test #15: new generation asset interface" << std::endl;
    {
        static const char* FTY_ASSET_TEST_Q   = "FTY.Q.ASSET.TEST";
        static const char* FTY_ASSET_TEST_PUB = "test-publisher";
        static const char* FTY_ASSET_TEST_REC = "test-receiver";

        const std::string FTY_ASSET_TEST_MAIL_NAME = asset_server_test_name + "-ng";

        log_debug("fty-asset-server-test:Test #15");

        std::unique_ptr<messagebus::MessageBus> publisher(
            messagebus::MlmMessageBus(endpoint, FTY_ASSET_TEST_PUB));
        std::unique_ptr<messagebus::MessageBus> receiver(
            messagebus::MlmMessageBus(endpoint, FTY_ASSET_TEST_REC));

        publisher->connect();

        receiver->connect();
        receiver->receive(FTY_ASSET_TEST_Q, s_test_asset_mailbox_handler);

        // test asset
        fty::Asset asset;
        asset.setInternalName("test-asset");
        asset.setAssetStatus(fty::AssetStatus::Active);
        asset.setAssetType("device");
        asset.setAssetSubtype("ups");
        asset.setParentIname("");
        asset.setPriority(4);
        asset.setExtEntry("name", "Test asset");

        std::cout << "asset: " << std::endl << fty::Asset::toJson(asset) << std::endl;

        messagebus::Message msg;

        // test create
        msg.metaData().clear();
        msg.metaData().emplace(messagebus::Message::CORRELATION_ID, messagebus::generateUuid());
        msg.metaData().emplace(messagebus::Message::SUBJECT, FTY_ASSET_SUBJECT_CREATE);
        msg.metaData().emplace(messagebus::Message::FROM, FTY_ASSET_TEST_REC);
        msg.metaData().emplace(messagebus::Message::TO, FTY_ASSET_TEST_MAIL_NAME);
        msg.metaData().emplace(messagebus::Message::REPLY_TO, FTY_ASSET_TEST_Q);
        msg.metaData().emplace(METADATA_TRY_ACTIVATE, "true");
        msg.metaData().emplace(METADATA_NO_ERROR_IF_EXIST, "true");

        msg.userData().clear();
        msg.userData().push_back(fty::Asset::toJson(asset));

        assetTestMap.emplace(
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, fty::Asset::toJson(asset));

        log_info("fty-asset-server-test:Test #15.1: send CREATE message");
        publisher->sendRequest(FTY_ASSET_MAILBOX, msg);
        zclock_sleep(200);

        // test update
        msg.metaData().clear();
        msg.metaData().emplace(messagebus::Message::CORRELATION_ID, messagebus::generateUuid());
        msg.metaData().emplace(messagebus::Message::SUBJECT, FTY_ASSET_SUBJECT_UPDATE);
        msg.metaData().emplace(messagebus::Message::FROM, FTY_ASSET_TEST_REC);
        msg.metaData().emplace(messagebus::Message::TO, FTY_ASSET_TEST_MAIL_NAME);
        msg.metaData().emplace(messagebus::Message::REPLY_TO, FTY_ASSET_TEST_Q);
        msg.metaData().emplace(METADATA_TRY_ACTIVATE, "true");

        msg.userData().clear();

        assetTestMap.emplace(
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, fty::Asset::toJson(asset));

        log_info("fty-asset-server-test:Test #15.2: send UPDATE message");
        publisher->sendRequest(FTY_ASSET_MAILBOX, msg);
        zclock_sleep(200);
        // test get
        msg.metaData().clear();
        msg.metaData().emplace(messagebus::Message::CORRELATION_ID, messagebus::generateUuid());
        msg.metaData().emplace(messagebus::Message::SUBJECT, FTY_ASSET_SUBJECT_GET);
        msg.metaData().emplace(messagebus::Message::FROM, FTY_ASSET_TEST_REC);
        msg.metaData().emplace(messagebus::Message::TO, FTY_ASSET_TEST_MAIL_NAME);
        msg.metaData().emplace(messagebus::Message::REPLY_TO, FTY_ASSET_TEST_Q);
        msg.metaData().emplace(METADATA_TRY_ACTIVATE, "true");

        msg.userData().clear();
        msg.userData().push_back("test-asset");

        assetTestMap.emplace(msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, "test-asset");

        log_info("fty-asset-server-test:Test #15.3: send GET message");
        publisher->sendRequest(FTY_ASSET_MAILBOX, msg);
        zclock_sleep(200);
    }

    zactor_destroy(&autoupdate_server);
    zactor_destroy(&asset_server);
    mlm_client_destroy(&ui);
    zactor_destroy(&server);

    //  @end
    std::cout << "fty_asset_server: OK" << std::endl;
}
