#include "asset/asset-db.h"
#include "asset/db.h"
#include <catch2/catch.hpp>
#include <fty_common_asset_types.h>
#include <test-db/sample-db.h>

TEST_CASE("Asset names")
{
    fty::SampleDb db("");

    tnt::Connection conn;

    fty::asset::db::AssetElement el;
    el.name      = "device";
    el.status    = "active";
    el.priority  = 1;
    el.subtypeId = persist::subtype_to_subtypeid("ups");
    el.typeId    = persist::type_to_typeid("device");

    auto ret = fty::asset::db::insertIntoAssetElement(conn, el, true);
    if (!ret) {
        FAIL(ret.error());
    }
    REQUIRE(*ret > 0);

    auto ret1 = fty::asset::db::insertIntoAssetExtAttributes(conn, *ret, {{"name", "Device name"}}, true);
    if (!ret1) {
        FAIL(ret1.error());
    }
    REQUIRE(*ret1 > 0);

    // Tests
    SECTION("nameToAssetId")
    {
        auto id = fty::asset::db::nameToAssetId("device");
        if (!id) {
            FAIL(id.error());
        }
        REQUIRE(*id == *ret);
    }

    SECTION("nameToAssetId/not found")
    {
        auto id = fty::asset::db::nameToAssetId("_device");
        CHECK(!id);
        REQUIRE(id.error() == "Element '_device' not found.");
    }

    SECTION("idToNameExtName")
    {
        auto id = fty::asset::db::idToNameExtName(*ret);
        if (!id) {
            FAIL(id.error());
        }
        REQUIRE(id->first == "device");
        REQUIRE(id->second == "Device name");
    }

    SECTION("idToNameExtName/not found")
    {
        auto id = fty::asset::db::idToNameExtName(uint32_t(-1));
        CHECK(!id);
        REQUIRE(id.error() == fmt::format("Element '{}' not found.", uint32_t(-1)));
    }

    SECTION("extNameToAssetName")
    {
        auto id = fty::asset::db::extNameToAssetName("Device name");
        if (!id) {
            FAIL(id.error());
        }
        REQUIRE(*id == "device");
    }

    SECTION("extNameToAssetName/not found")
    {
        auto id = fty::asset::db::extNameToAssetName("Some shit");
        CHECK(!id);
        REQUIRE(id.error() == "Element 'Some shit' not found.");
    }

    SECTION("extNameToAssetId")
    {
        auto id = fty::asset::db::extNameToAssetId("Device name");
        if (!id) {
            FAIL(id.error());
        }
        REQUIRE(*id == *ret);
    }

    SECTION("extNameToAssetId/not found")
    {
        auto id = fty::asset::db::extNameToAssetId("Some shit");
        CHECK(!id);
        REQUIRE(id.error() == "Element 'Some shit' not found.");
    }

    // Clean up
    {
        auto res = fty::asset::db::deleteAssetExtAttributesWithRo(conn, *ret, true);
        if (!res) {
            FAIL(res.error());
        }
        CHECK(res);
        CHECK(res > 0);
    }

    {
        auto res = fty::asset::db::deleteAssetElement(conn, *ret);
        if (!res) {
            FAIL(res.error());
        }
        REQUIRE(*res > 0);
    }
}
