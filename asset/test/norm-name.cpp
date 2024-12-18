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
#include <test-db/sample-db.h>
#include <iostream>

#include <catch2/catch.hpp>

TEST_CASE("Long names / simple")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
        )");

    {
        auto res = fty::asset::normName("Long Long Long name Long name Long name device name", 50);
        REQUIRE(res);
        CHECK(*res == "Long Long Long name Long name Long name device nam");
    }
}

TEST_CASE("Long names / already exists")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
              items :
                  - type     : Feed
                    name     : feed
                  - type     : Server
                    name     : dev1
                    ext-name : Long Long Long name Long name Long name device nam
        )");

    {
        auto res = fty::asset::normName("Long Long Long name Long name Long name device name", 50);
        REQUIRE(res);
        CHECK(*res == "Long Long Long name Long name Long name device n~1");
    }
}

TEST_CASE("Long names / already exists 2")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
              items :
                  - type     : Feed
                    name     : feed
                  - type     : Server
                    name     : dev1
                    ext-name : Long Long Long name Long name Long name device nam
                  - type     : Server
                    name     : dev2
                    ext-name : Long Long Long name Long name Long name device n~1
        )");

    {
        auto res = fty::asset::normName("Long Long Long name Long name Long name device name", 50);
        REQUIRE(res);
        CHECK(*res == "Long Long Long name Long name Long name device n~2");
    }
}

TEST_CASE("Long names / already exists 3")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
              items :
                  - type     : Feed
                    name     : feed
                  - type     : Server
                    name     : dev1
                    ext-name : Long Long Long name Long name Long name device nam
                  - type     : Server
                    name     : dev2
                    ext-name : Long Long Long name Long name Long name device ~10
        )");

    {
        auto res = fty::asset::normName("Long Long Long name Long name Long name device name", 50);
        REQUIRE(res);
        CHECK(*res == "Long Long Long name Long name Long name device ~11");
    }
}
