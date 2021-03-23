#include "asset/asset-db.h"
#include "asset/db.h"
#include <catch2/catch.hpp>
#include <fty_common_asset_types.h>
#include <test-db/sample-db.h>

TEST_CASE("Select asset")
{
    fty::SampleDb db("");

    tnt::Connection conn;

    fty::asset::db::AssetElement el;
    el.name      = "device";
    el.status    = "active";
    el.priority  = 1;
    el.subtypeId = persist::subtype_to_subtypeid("ups");
    el.typeId    = persist::type_to_typeid("device");

    {
        auto ret = fty::asset::db::insertIntoAssetElement(conn, el, true);
        if (!ret) {
            FAIL(ret.error());
        }
        REQUIRE(*ret > 0);
        el.id = *ret;
    }

    {
        auto ret = fty::asset::db::insertIntoAssetExtAttributes(conn, el.id, {{"name", "Device name"}}, true);
        if (!ret) {
            FAIL(ret.error());
        }
        REQUIRE(*ret > 0);
    }

    fty::asset::db::AssetElement gr;
    gr.name      = "MyGroup";
    gr.status    = "active";
    gr.priority  = 1;
    gr.typeId    = persist::type_to_typeid("group");

    {
        auto ret = fty::asset::db::insertIntoAssetElement(conn, gr, true);
        if (!ret) {
            FAIL(ret.error());
        }
        REQUIRE(*ret > 0);
        gr.id = *ret;
    }

    {
        auto ret = fty::asset::db::insertElementIntoGroups(conn, {gr.id}, el.id);
        if (!ret) {
            FAIL(ret.error());
        }
        REQUIRE(*ret > 0);
    }


    SECTION("selectAssetElementByName")
    {
        auto res = fty::asset::db::selectAssetElementByName("device");
        if (!res) {
            FAIL(res.error());
        }
        REQUIRE(res);
        CHECK(res->id == el.id);
        CHECK(res->name == "device");
        CHECK(res->status == "active");
        CHECK(res->priority == 1);
        CHECK(res->subtypeId == persist::subtype_to_subtypeid("ups"));
        CHECK(res->typeId == persist::type_to_typeid("device"));
    }

    SECTION("selectAssetElementByName/wrong")
    {
        auto res = fty::asset::db::selectAssetElementByName("mydevice");
        CHECK(!res);
        CHECK(res.error() == "Element 'mydevice' not found.");
    }

    SECTION("selectAssetElementWebById")
    {
        auto res = fty::asset::db::selectAssetElementWebById(el.id);
        if (!res) {
            FAIL(res.error());
        }
        REQUIRE(res);
        CHECK(res->id == el.id);
        CHECK(res->name == "device");
        CHECK(res->status == "active");
        CHECK(res->priority == 1);
        CHECK(res->subtypeId == persist::subtype_to_subtypeid("ups"));
        CHECK(res->typeId == persist::type_to_typeid("device"));
    }

    SECTION("selectAssetElementWebById/wrong")
    {
        auto res = fty::asset::db::selectAssetElementWebById(uint32_t(-1));
        CHECK(!res);
        CHECK(res.error() == fmt::format("Element '{}' not found.", uint32_t(-1)));
    }

    SECTION("selectExtAttributes")
    {
        auto res = fty::asset::db::selectExtAttributes(el.id);
        if (!res) {
            FAIL(res.error());
        }
        REQUIRE(res);
        CHECK(res->size() == 1);
        CHECK((*res)["name"].value == "Device name");
        CHECK((*res)["name"].readOnly == true);
    }

    SECTION("selectAssetElementGroups")
    {
        auto res = fty::asset::db::selectAssetElementGroups(el.id);
        if (!res) {
            FAIL(res.error());
        }
        REQUIRE(res);
        CHECK(res->size() == 1);
        CHECK((*res)[gr.id] == "MyGroup");
    }

    SECTION("selectAssetElementGroups/wrong")
    {
        auto res = fty::asset::db::selectAssetElementGroups(uint32_t(-1));
        if (!res) {
            FAIL(res.error());
        }
        REQUIRE(res);
        CHECK(res->size() == 0);
    }

    SECTION("selectAssetElementSuperParent")
    {
        auto res = fty::asset::db::selectAssetElementSuperParent(el.id, [](const auto& /*row*/){});
        if (!res) {
            FAIL(res.error());
        }
        REQUIRE(res);
    }

    SECTION("countKeytag")
    {
        auto res = fty::asset::db::countKeytag("name", "Device name");
        if (!res) {
            FAIL(res.error());
        }
        REQUIRE(res);
        CHECK(*res == 1);
    }

    SECTION("selectShortElements")
    {
        auto res = fty::asset::db::selectShortElements(persist::type_to_typeid("device"), persist::subtype_to_subtypeid("ups"));
        if (!res) {
            FAIL(res.error());
        }
        REQUIRE(res);
        CHECK(res->size() == 1);
    }

    // Clean up
    {
        auto res = fty::asset::db::deleteAssetGroupLinks(conn, gr.id);
        if (!res) {
            FAIL(res.error());
        }
        CHECK(res);
        CHECK(res > 0);
    }

    {
        auto res = fty::asset::db::deleteAssetExtAttributesWithRo(conn, el.id, true);
        if (!res) {
            FAIL(res.error());
        }
        CHECK(res);
        CHECK(res > 0);
    }

    {
        auto res = fty::asset::db::deleteAssetElement(conn, el.id);
        if (!res) {
            FAIL(res.error());
        }
        REQUIRE(*res > 0);
    }

    {
        auto res = fty::asset::db::deleteAssetElement(conn, gr.id);
        if (!res) {
            FAIL(res.error());
        }
        REQUIRE(*res > 0);
    }
}
