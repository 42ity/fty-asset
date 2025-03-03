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

#include "asset/asset-helpers.h"
#include <catch2/catch.hpp>

TEST_CASE("generateUUID")
{
    using namespace fty::asset;

    // random UUID (manufacturer and/or serial are empty)
    CHECK(generateUUID(AssetFilter("","","","")).uuid != generateUUID(AssetFilter("","","","")).uuid);
    CHECK(generateUUID(AssetFilter("","","x","")).uuid != generateUUID(AssetFilter("","","x","")).uuid);
    CHECK(generateUUID(AssetFilter("","","","y")).uuid != generateUUID(AssetFilter("","","","y")).uuid);
    CHECK(generateUUID(AssetFilter("a","","","")).uuid != generateUUID(AssetFilter("a","","","")).uuid);
    CHECK(generateUUID(AssetFilter("","b","","")).uuid != generateUUID(AssetFilter("","b","","")).uuid);

    // sha1 UUID (manufacturer and serial are non empty, madAddr handled, ipAddr ignored)
    CHECK(generateUUID(AssetFilter("a","b","","")).uuid == generateUUID(AssetFilter("a","b","","")).uuid);
    CHECK(generateUUID(AssetFilter("a","b","x","")).uuid == generateUUID(AssetFilter("a","b","x","")).uuid);
    CHECK(generateUUID(AssetFilter("a","b","x0","")).uuid != generateUUID(AssetFilter("a","b","x1","")).uuid);
    CHECK(generateUUID(AssetFilter("a","b","x","y0")).uuid == generateUUID(AssetFilter("a","b","x","y1")).uuid);

    CHECK(generateUUID(AssetFilter("a","b","x","")).uuid != generateUUID(AssetFilter("1","b","x","")).uuid);
    CHECK(generateUUID(AssetFilter("a","b","x","")).uuid != generateUUID(AssetFilter("a","1","x","")).uuid);
    CHECK(generateUUID(AssetFilter("a","b","x","")).uuid != generateUUID(AssetFilter("a","b","1","")).uuid);

    // sha1 UUID case insensitive
    CHECK(generateUUID(AssetFilter("a","b","x","")).uuid == generateUUID(AssetFilter("A","B","X","")).uuid);

    // sha1 UUID macAddress sanitized
    CHECK(generateUUID(AssetFilter("a","b","x y z","")).uuid == generateUUID(AssetFilter("a","b","xyz","")).uuid);
    CHECK(generateUUID(AssetFilter("a","b","x:y:z","")).uuid == generateUUID(AssetFilter("a","b","xyz","")).uuid);
    CHECK(generateUUID(AssetFilter("a","b",":: x::: y  z:: ","")).uuid == generateUUID(AssetFilter("a","b","xyz","")).uuid);
}
