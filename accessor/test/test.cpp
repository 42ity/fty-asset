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

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "fty_asset_dto.h"
#include "fty_asset_accessor.h"

using namespace fty;

TEST_CASE("Create test")
{
    REQUIRE_NOTHROW([&]()
    {
        AssetAccessor accessor;
        (void)accessor;
    }());
}

TEST_CASE("Request ID - Success")
{
    auto id = AssetAccessor::assetInameToID("rackcontroller-0");

    REQUIRE(id);
    CHECK(*id == 1);
}

TEST_CASE("Request ID - Failure")
{
    auto id = AssetAccessor::assetInameToID("datacenter-0");

    REQUIRE(!id);
    CHECK(id.error().find("failed") != std::string::npos);
}
