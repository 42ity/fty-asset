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
#include "fty_asset_autoupdate.h"
#include <fty_log.h>
#include <iostream>

TEST_CASE("fty_asset_autoupdate test")
{
    std::cout << " * fty_asset_autoupdate: " << std::endl;

    //  Test #1: Simple create/destroy test
    {
        log_debug ("fty_asset_autoupdate:Test #1");
        zactor_t *self = zactor_new (fty_asset_autoupdate_server, static_cast<void*>( const_cast<char*>( "asset-autoupdate-test")));
        REQUIRE(self);
        zclock_sleep (200);
        zactor_destroy (&self);
        CHECK(!self);
        log_info ("fty-asset-server-test:Test #1: OK");
    }

    std::cout << "fty_asset_autoupdate: OK"  << std::endl;
}
