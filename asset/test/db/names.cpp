/*  ====================================================================================================================
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
    ====================================================================================================================
*/

#include "asset/asset-db.h"
#include <fty_common_asset_types.h>
#include <fty_common_db_connection.h>
#include <test-db/sample-db.h>

#include <catch2/catch.hpp>

TEST_CASE("Asset names")
{
    fty::SampleDb db(R"(
        items:
          - type     : Ups
            name     : ups
            ext-name : Ups
    )");

    fty::db::Connection conn;

    // Tests
    // nameToAssetId
    {
        auto id = fty::asset::db::nameToAssetId("ups");
        if (!id) {
            FAIL(id.error());
        }
        REQUIRE(*id == db.idByName("ups"));
    }

    // nameToAssetId/not found
    {
        auto id = fty::asset::db::nameToAssetId("_device");
        CHECK(!id);
        REQUIRE(id.error() == "Element '_device' not found.");
    }

    // idToNameExtName
    {
        auto id = fty::asset::db::idToNameExtName(db.idByName("ups"));
        if (!id) {
            FAIL(id.error());
        }
        REQUIRE(id->first == "ups");
        REQUIRE(id->second == "Ups");
    }

    // idToNameExtName/not found
    {
        auto id = fty::asset::db::idToNameExtName(uint32_t(-1));
        CHECK(!id);
        REQUIRE(id.error() == fmt::format("Element '{}' not found.", uint32_t(-1)));
    }

    // extNameToAssetName
    {
        auto id = fty::asset::db::extNameToAssetName("Ups");
        if (!id) {
            FAIL(id.error());
        }
        REQUIRE(*id == "ups");
    }

    // extNameToAssetName/not found
    {
        auto id = fty::asset::db::extNameToAssetName("Some shit");
        CHECK(!id);
        REQUIRE(id.error() == "Element 'Some shit' not found.");
    }

    // extNameToAssetId
    {
        auto id = fty::asset::db::extNameToAssetId("Ups");
        if (!id) {
            FAIL(id.error());
        }
        REQUIRE(*id == db.idByName("ups"));
    }

    // extNameToAssetId/not found
    {
        auto id = fty::asset::db::extNameToAssetId("Some shit");
        CHECK(!id);
        REQUIRE(id.error() == "Element 'Some shit' not found.");
    }
}
