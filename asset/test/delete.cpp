#include "asset/asset-manager.h"
#include "asset/json.h"
#include <catch2/catch.hpp>
#include <test-db/sample-db.h>

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
