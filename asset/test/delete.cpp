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

#include "asset/asset-manager.h"
#include "asset/json.h"
#include <test-db/sample-db.h>

#include <catch2/catch.hpp>

#define REQUIRE_EXP(expr) \
    do { \
        if (!expr) { FAIL(expr.error().message()); } \
    } while(0)

TEST_CASE("Delete asset / Last feed")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
              attrs :
                  fast_track: true
              items :
                  - type : Feed
                    name : feed
    )");

    auto ret = fty::asset::AssetManager::deleteAsset(db.idByName("feed"), false);
    REQUIRE(!ret);
    CHECK("Last feed cannot be deleted in device centric mode" == ret.error().toString());
}

TEST_CASE("Delete asset / Last feeds")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
              attrs :
                  fast_track: true
              items :
                  - type : Feed
                    name : feed1
                  - type : Feed
                    name : feed2
    )");

    {
        auto ret = fty::asset::AssetManager::deleteAsset(db.idByName("feed1"), false);
        REQUIRE_EXP(ret);
    }

    {
        auto ret = fty::asset::AssetManager::deleteAsset(db.idByName("feed2"), false);
        REQUIRE(!ret);
        CHECK("Last feed cannot be deleted in device centric mode" == ret.error().toString());
    }
}

TEST_CASE("Delete asset / Last feeds 2")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
              attrs :
                  fast_track: true
              items :
                  - type : Feed
                    name : feed1
                  - type : Feed
                    name : feed2
    )");

    auto ret = fty::asset::AssetManager::deleteAsset(
        {{db.idByName("feed1"), "feed1"}, {db.idByName("feed2"), "feed2"}}, false);
    REQUIRE(ret.size() == 2);
    CHECK(ret.at("feed1").isValid());
    CHECK(!ret.at("feed2").isValid());
    CHECK("Last feed cannot be deleted in device centric mode" == ret.at("feed2").error().toString());
}
