#include "asset/asset-helpers.h"
#include "asset/json.h"
#include <asset/asset-manager.h>
#include <catch2/catch.hpp>
#include <test-db/sample-db.h>
#include <yaml-cpp/yaml.h>

inline YAML::Node reorder(const YAML::Node& node)
{
    YAML::Node ret;
    if (node.IsMap()) {
        ret = YAML::Node(YAML::NodeType::Map);
        std::map<std::string, YAML::Node> map;
        for (const auto& it : node) {
            map[it.first.as<std::string>()] = it.second.IsMap() ? reorder(it.second) : it.second;
        }
        for (const auto& [key, val] : map) {
            ret[key] = val;
        }
    } else if (node.IsSequence()) {
        ret = YAML::Node(YAML::NodeType::Sequence);
        for (const auto& it : node) {
            ret.push_back(it.second.IsMap() ? reorder(it.second) : it.second);
        }
    } else {
        ret = node;
    }
    return ret;
}

inline std::string normalizeJson(const std::string& json)
{
    try {
        YAML::Node yaml = YAML::Load(json);

        if (yaml.IsMap()) {
            yaml = reorder(yaml);
        }

        YAML::Emitter out;
        out << YAML::DoubleQuoted << YAML::Flow << yaml;
        return std::string(out.c_str());
    } catch (const std::exception&) {
        return "";
    }
}

TEST_CASE("Read asset / Check id")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
              items :
                  - type     : Server
                    name     : srv
                    ext-name : Server
        )");

    std::string strId = "srv";
    uint32_t    id    = 0;
    if (auto res = fty::asset::checkElementIdentifier("srv", strId)) {
        id = *res;
    } else if (auto res2 = fty::asset::checkElementIdentifier("srv", "device" + strId)) {
        id = *res2;
    } else {
        FAIL("Not found " + strId);
    }
    CHECK(id > 0);
}

TEST_CASE("Read asset / Check id/wrong")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
              items :
                  - type     : Server
                    name     : srv
                    ext-name : Server
        )");

    std::string strId = "srv1";
    uint32_t    id    = 0;
    if (auto res = fty::asset::checkElementIdentifier("srv", strId)) {
        id = *res;
    } else if (auto res2 = fty::asset::checkElementIdentifier("srv", "srv" + strId)) {
        id = *res2;
    }
    CHECK(id == 0);
}

TEST_CASE("Read")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
              items :
                  - type     : Server
                    name     : srv
                    ext-name : Server
    )");

    static std::string check = R"({
        "id" :                  "srv",
        "power_devices_in_uri": "/api/v1/assets?in=srv&sub_type=epdu,pdu,feed,genset,ups,sts,rackcontroller",
        "name" :                "Server",
        "status" :              "active",
        "priority" :            "P1",
        "type" :                "device",
        "location_uri" :        "/api/v1/asset/datacenter",
        "location_id" :         "datacenter",
        "location" :            "Data Center",
        "location_type":        "datacenter",
        "groups":               [],
        "powers":               [],
        "sub_type" :            "server",
        "ext" :                 [],
        "computed" :            {},
        "parents" : [{
            "id" :          "datacenter",
            "name" :        "Data Center",
            "type" :        "datacenter",
            "sub_type" :    "N_A"
        }]
    })";

    uint32_t id = 0;
    if (auto res = fty::asset::checkElementIdentifier("srv", "srv")) {
        id = *res;
    }

    REQUIRE(id > 0);
    std::string jsonAsset = fty::asset::getJsonAsset(id);

    CHECK(!jsonAsset.empty());
    CHECK(normalizeJson(jsonAsset) == normalizeJson(check));
}
