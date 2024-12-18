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
#include "asset/asset-manager.h"
#include "test-utils.h"
#include <test-db/sample-db.h>

#include <catch2/catch.hpp>

template <typename T>
struct Holder
{
    Holder():
        m_delete(false)
    {
    }

    explicit Holder(const T& el)
        : m_el(el)
    {
    }

    ~Holder()
    {
        if (m_delete) {
            std::cerr << "delete " << m_el.name << std::endl;
            deleteAsset(m_el);
        }
    }

    bool m_delete = true;
    T m_el;
};

static Holder<fty::asset::db::AssetElement> placeAsset(fty::asset::db::AssetElement& parent, int size, int location, bool expectOk)
{
    using namespace fmt::literals;
    std::string json;
    try {
        json = fmt::format(R"({{
            "name" :                "{name}",
            "priority" :            "P2",
            "status" :              "active",
            "sub_type" :            "server",
            "type" :                "device",
            "location":             "{parent}",
            "ext": [
                {{"u_size"         : "{size}",      "read_only": true}},
                {{"location_u_pos" : "{loc}",       "read_only": true}},
                {{"location_w_pos" : "horizonatal", "read_only": true}}
            ]
        }})",
            "name"_a = "el" + std::to_string(random()), "size"_a = size, "loc"_a = location, "parent"_a = parent.name);
    } catch (const fmt::format_error& e) {
        std::cerr << e.what() << std::endl;
    }

    auto ret = fty::asset::AssetManager::createAsset(json, "dummy", false);
    if (expectOk) {
        if (!ret) {
            FAIL(ret.error());
        }
        CHECK(*ret);
        return Holder<fty::asset::db::AssetElement>(*fty::asset::db::selectAssetElementWebById(*ret));
    } else {
        CHECK(!ret);
        return Holder<fty::asset::db::AssetElement>();
    }
}

TEST_CASE("USize")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
              items :
                  - type     : Row
                    name     : row
                    items    :
                    - type : Rack
                      name : rack
                      attrs :
                          u_size: 10

        )");

    auto rack = fty::asset::db::selectAssetElementWebByName("rack");

    SECTION("Fit it")
    {
        auto el  = placeAsset(*rack, 1, 1, true);
        auto el1 = placeAsset(*rack, 1, 2, true);
        auto el2 = placeAsset(*rack, 1, 3, true);
        auto el3 = placeAsset(*rack, 1, 4, true);
    }

    SECTION("Wrong pos")
    {
        auto el  = placeAsset(*rack, 2, 1, true);
        auto el1 = placeAsset(*rack, 1, 2, false);
    }

    SECTION("Fit it 2")
    {
        auto el  = placeAsset(*rack, 2, 1, true);
        auto el1 = placeAsset(*rack, 1, 4, true);
        auto el2 = placeAsset(*rack, 1, 5, true);
        auto el3 = placeAsset(*rack, 1, 6, true);
    }

    SECTION("Huge")
    {
        auto el  = placeAsset(*rack, 10, 1, true);
    }

    SECTION("Huge fail")
    {
        auto el  = placeAsset(*rack, 11, 1, false);
    }

    SECTION("Border pos")
    {
        auto el  = placeAsset(*rack, 1, 9, true);
        auto el1 = placeAsset(*rack, 1, 10, true);
    }

    SECTION("Border pos wrong")
    {
        auto el  = placeAsset(*rack, 1, 9, true);
        auto el1 = placeAsset(*rack, 2, 10, false);
    }
}
