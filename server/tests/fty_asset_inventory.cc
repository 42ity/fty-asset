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
#include "fty_asset_inventory.h"
#include <fty_log.h>
#include <fty_proto.h>
#include <malamute.h>
#include <czmq.h>
#include <iostream>

TEST_CASE("fty_asset_inventory test")
{
    std::cout << " * fty_asset_inventory:" << std::endl;

    //  Test #1: Simple create/destroy test
    {
        log_debug ("fty-asset-server-test:Test #1");
        zactor_t *self = zactor_new (fty_asset_inventory_server, static_cast<void*>( const_cast<char*>( "asset-inventory-test")));
        zclock_sleep (200);
        zactor_destroy (&self);
        log_info ("fty-asset-server-test:Test #1: OK");
    }

    static const char* endpoint = "inproc://fty_asset_inventory_test";

    zactor_t *server = zactor_new (mlm_server, static_cast<void*>( const_cast<char*>( "Malamute")));
    assert ( server != NULL );
    zstr_sendx (server, "BIND", endpoint, NULL);

    mlm_client_t *ui = mlm_client_new ();
    mlm_client_connect (ui, endpoint, 5000, "fty-asset-inventory-ui");
    mlm_client_set_producer (ui, "ASSETS-TEST");

    zactor_t *inventory_server = zactor_new (fty_asset_inventory_server, static_cast<void*>( const_cast<char*>("asset-inventory-test")));
    zstr_sendx (inventory_server, "CONNECT", endpoint, NULL);
    zsock_wait (inventory_server);
    zstr_sendx (inventory_server, "CONSUMER", "ASSETS-TEST", "inventory@.*", NULL);
    zsock_wait (inventory_server);

    // Test #2: create inventory message and process it
    {
        log_debug ("fty-asset-server-test:Test #2");
        zmsg_t *msg = fty_proto_encode_asset (
                NULL,
                "MyDC",
                "inventory",
                NULL);
        int rv = mlm_client_send (ui, "inventory@dc-1", &msg);
        assert (rv == 0);
        zclock_sleep (200);
        log_info ("fty-asset-server-test:Test #2: OK");
    }

    zactor_destroy (&inventory_server);
    mlm_client_destroy (&ui);
    zactor_destroy (&server);

    std::cout << "fty_asset_inventory: OK" << std::endl;
}
