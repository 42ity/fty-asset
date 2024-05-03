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
#include "dns.h"
#include <iostream>

TEST_CASE("dns test")
{
    std::cout << " * dns:" << std::endl;

    {
        int found = 0;
        for (auto a : name_to_ip4 ("localhost")) {
            if (a == "127.0.0.1") found++;
        }
        CHECK(found == 1);
    }

    CHECK(! ip_to_name("127.0.0.1").empty ());

    std::cout << "dns: OK" << std::endl;
}
