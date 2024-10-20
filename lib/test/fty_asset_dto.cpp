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

#include "fty_asset_dto.h"

using namespace fty;

TEST_CASE("fty::Asset Create object")
{
    REQUIRE_NOTHROW([&]()
    {
        Asset asset;
    }());
}

TEST_CASE("fty::Asset Accessors")
{
    Asset asset;
    asset.setInternalName("dc-0");
    asset.setAssetStatus(AssetStatus::Nonactive);
    asset.setAssetType(TYPE_DEVICE);
    asset.setAssetSubtype(SUB_UPS);
    asset.setParentIname("abc123");
    asset.setExtEntry("testKey", "testValue");
    asset.setPriority(4);
    asset.setAssetTag("testTag");

    CHECK(asset.getInternalName() == "dc-0");
    CHECK(asset.getAssetStatus() == AssetStatus::Nonactive);
    CHECK(asset.getAssetType() == TYPE_DEVICE);
    CHECK(asset.getAssetSubtype() == SUB_UPS);
    CHECK(asset.getParentIname() == "abc123");
    CHECK(asset.getExtEntry("testKey") == "testValue");
    CHECK(asset.getPriority() == 4);
    CHECK(asset.getAssetTag() == "testTag");
}

TEST_CASE("fty::Asset Equality check")
{
    Asset asset;
    asset.setInternalName("dc-0");
    asset.setAssetStatus(AssetStatus::Nonactive);
    asset.setAssetType(TYPE_DEVICE);
    asset.setAssetSubtype(SUB_UPS);
    asset.setParentIname("abc123");
    asset.setExtEntry("testKey", "testValue");
    asset.setPriority(4);
    asset.setAssetTag("testTag");

    {
        Asset tmp;
        tmp = asset;
        CHECK(asset == tmp);
    }
    {
        Asset tmp;
        tmp = asset;
        tmp.setInternalName("dc-1");
        CHECK(asset != tmp);
    }
    {
        Asset tmp;
        tmp = asset;
        tmp.setAssetStatus(AssetStatus::Active);
        CHECK(asset != tmp);
    }
    {
        Asset tmp;
        tmp = asset;
        tmp.setAssetType(TYPE_DATACENTER);
        CHECK(asset != tmp);
    }
    {
        Asset tmp;
        tmp = asset;
        tmp.setAssetSubtype(SUB_SENSOR);
        CHECK(asset != tmp);
    }
    {
        Asset tmp;
        tmp = asset;
        tmp.setParentIname("xyz789");
        CHECK(asset != tmp);
    }
    {
        Asset tmp;
        tmp = asset;
        tmp.setExtEntry("testKey", "testValue2");
        CHECK(asset != tmp);
    }
    {
        Asset tmp;
        tmp = asset;
        tmp.setExtEntry("testKey2", "testValue2");
        CHECK(asset != tmp);
    }
    {
        Asset tmp;
        tmp = asset;
        tmp.setPriority(5);
        CHECK(asset != tmp);
    }
    {
        Asset tmp;
        tmp = asset;
        tmp.setAssetTag("testTag2");
        CHECK(asset != tmp);
    }
}

TEST_CASE("fty::Asset Priority set")
{
    FullAsset asset;
    CHECK_NOTHROW(asset.setPriority(1));
    CHECK(asset.getPriority() == 1);
    CHECK_NOTHROW(asset.setPriority("1"));
    CHECK(asset.getPriority() == 1);
    CHECK_NOTHROW(asset.setPriority("P1"));
    CHECK(asset.getPriority() == 1);
}
