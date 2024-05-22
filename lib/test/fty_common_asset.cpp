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
#include "fty_common_asset.h"
#include "conversion/json.h"

#include <cxxtools/serializationinfo.h>
#include <fty_common_json.h>
#include <fty_common_asset_types.h>

TEST_CASE("conversion/json test")
{
    try {
        fty::Asset asset1, asset2;
        std::string json = fty::conversion::toJson(asset1);
        fty::conversion::fromJson(json, asset2);
        std::cerr << "asset1: " << fty::Asset::toJson(asset1) << std::endl;
        std::cerr << "asset2: " << fty::Asset::toJson(asset2) << std::endl;
        CHECK(asset1 == asset2);
    } catch (std::exception &e) {
        CHECK(false); // exception not expected
    }
}

TEST_CASE("fty_common_asset test")
{
    printf (" * fty_common_asset: \n");

    //  @selftest
    try {
        //fty::BasicAsset a; // this causes g++ error, as expected
        fty::BasicAsset b ("id-1", "active", "device", "rackcontroller");
        CHECK(b.getId () == "id-1");
        CHECK(b.getStatus () == fty::BasicAsset::Status::Active);
        CHECK(b.getType () == persist::asset_type::DEVICE);
        CHECK(b.getSubtype () == persist::asset_subtype::RACKCONTROLLER);
        CHECK(b.getStatusString () == "active");
        CHECK(b.getTypeString () == "device");
        CHECK(b.getSubtypeString () == "rackcontroller");
        b.setStatus ("nonactive");
        CHECK(b.getStatus () == fty::BasicAsset::Status::Nonactive);
        b.setType (fty::TYPE_VIRTUAL_MACHINE);
        CHECK(b.getType () == persist::asset_type::VIRTUAL_MACHINE);
        b.setSubtype (fty::SUB_VMWARE_VM);
        CHECK(b.getSubtype () == persist::asset_subtype::VMWARE_VM);

        fty::BasicAsset bb (b);
        CHECK(b == bb);
        CHECK(bb.getId () == "id-1");
        CHECK(bb.getType () == persist::asset_type::VIRTUAL_MACHINE);
        bb.setType ("device");
        CHECK(bb.getType () == persist::asset_type::DEVICE);
        CHECK(b.getType () == persist::asset_type::VIRTUAL_MACHINE);
        CHECK(b != bb);

        fty::BasicAsset bJson1 ("id-1", "active", "device", "rackcontroller");
        cxxtools::SerializationInfo rootSi;
        rootSi <<= bJson1;
        std::cerr << JSON::writeToString(rootSi, true) << std::endl;

        fty::BasicAsset bJson2;
        rootSi >>= bJson2;
        CHECK(bJson1.getId() == bJson2.getId());
        CHECK(bJson1.getStatusString() == bJson2.getStatusString());
        CHECK(bJson1.getTypeString() == bJson2.getTypeString());
        CHECK(bJson1.getSubtypeString() == bJson2.getSubtypeString());

    } catch (std::exception &e) {
        CHECK(false); // exception not expected
    }

    try {
        fty::BasicAsset c ("id-2", "invalid_status", "device", "rackcontroller");
        CHECK(false);
    } catch (std::exception &e) {
        // exception is expected
    }

    try {
        fty::BasicAsset d ("id-3", "active", "invalid_type", "rackcontroller");
        std::cerr << "d: " << d.toJson() << std::endl;
        CHECK(d.getType () == persist::asset_type::TUNKNOWN);
    } catch (std::exception &e) {
        CHECK(false); // exception not expected
    }

    try {
        fty::BasicAsset e ("id-4", "active", "device", "invalid_subtype");
        std::cerr << "e: " << e.toJson() << std::endl;
        CHECK(e.getSubtype () == persist::asset_subtype::SUNKNOWN);
    } catch (std::exception &e) {
        CHECK(false); // exception not expected
    }

    try {
        fty::ExtendedAsset f ("id-5", "active", "device", "rackcontroller", "MyRack", "id-1", 1);
        CHECK(f.getName () == "MyRack");
        CHECK(f.getParentId () == "id-1");
        CHECK(f.getPriority () == 1);
        CHECK(f.getPriorityString () == "P1");

        fty::ExtendedAsset g ("id-6", "active", "device", "rackcontroller", "MyRack", "parent-1", "P2");
        CHECK(f != g);
        CHECK(g.getPriority () == 2);
        CHECK(g.getPriorityString () == "P2");
        g.setName ("MyNewRack");
        CHECK(g.getName () == "MyNewRack");
        g.setParentId ("parent-2");
        CHECK(g.getParentId () == "parent-2");
        g.setPriority ("P3");
        CHECK(g.getPriority () == 3);
        g.setPriority (4);
        CHECK(g.getPriority () == 4);

        fty::ExtendedAsset gg (g);
        CHECK(g == gg);
        CHECK(gg.getId () == "id-6");
        CHECK(gg.getName () == "MyNewRack");
        gg.setName ("MyOldRack");
        CHECK(gg.getName () == "MyOldRack");
        CHECK(g.getName () == "MyNewRack");
        CHECK(g != gg);

        fty::ExtendedAsset fJson1 ("id-5", "active", "device", "rackcontroller", "MyRack", "id-1", 1);
        cxxtools::SerializationInfo rootSi;
        rootSi <<= fJson1;
        std::cerr << JSON::writeToString(rootSi, true) << std::endl;

        fty::ExtendedAsset fJson2;
        rootSi >>= fJson2;
        CHECK(fJson1.getId() == fJson2.getId());
        CHECK(fJson1.getStatusString() == fJson2.getStatusString());
        CHECK(fJson1.getTypeString() == fJson2.getTypeString());
        CHECK(fJson1.getSubtypeString() == fJson2.getSubtypeString());
        CHECK(fJson1.getName() == fJson2.getName());
        CHECK(fJson1.getParentId() == fJson2.getParentId());
        CHECK(fJson1.getPriority() == fJson2.getPriority());

    } catch (std::exception &e) {
        CHECK(false); // exception not expected
    }

    try {
        fty::FullAsset h ("id-7", "active", "device", "rackcontroller", "MyRack", "id-1", 1, {{"aux1", "aval1"},
                {"aux2", "aval2"}}, {});
        CHECK(h.getAuxItem ("aux2") == "aval2");
        CHECK(h.getAuxItem ("aval3").empty ());
        CHECK(h.getExtItem ("eval1").empty ());
        h.setAuxItem ("aux4", "aval4");
        CHECK(h.getAuxItem ("aux4") == "aval4");
        h.setExtItem ("ext5", "eval5");
        CHECK(h.getExtItem ("ext5") == "eval5");
        h.setExt ({{"ext1", "eval1"}});
        CHECK(h.getExtItem ("ext1") == "eval1");
        CHECK(h.getExtItem ("ext5") != "eval5");
        CHECK(h.getItem ("aux2") == "aval2");
        CHECK(h.getItem ("ext1") == "eval1");
        CHECK(h.getItem ("notthere").empty ());
        fty::FullAsset hh (h);
        CHECK(h == hh);
        CHECK(hh.getExtItem ("ext1") == "eval1");
        CHECK(hh.getExtItem ("ext6").empty ());
        hh.setExtItem ("ext6", "eval6");
        CHECK(hh.getExtItem ("ext6") == "eval6");
        CHECK(h.getExtItem ("ext6").empty ());
        CHECK(h != hh);

        fty::FullAsset hJson1 ("id-7", "active", "device", "rackcontroller", "MyRack", "id-1", 1, {{"parent", "1"},{"parent_name.1","id-1"}}, {{"name","MyRack"}});
        cxxtools::SerializationInfo rootSi;
        rootSi <<= hJson1;
        std::cerr << JSON::writeToString(rootSi, true) << std::endl;

        fty::FullAsset hJson2;
        rootSi >>= hJson2;

        CHECK(hJson1.getId() == hJson2.getId());
        CHECK(hJson1.getStatusString() == hJson2.getStatusString());
        CHECK(hJson1.getTypeString() == hJson2.getTypeString());
        CHECK(hJson1.getSubtypeString() == hJson2.getSubtypeString());
        CHECK(hJson1.getName() == hJson2.getName());
        CHECK(hJson1.getParentId() == hJson2.getParentId());
        CHECK(hJson1.getPriority() == hJson2.getPriority());

        CHECK(hJson1.getAuxItem("parent") == hJson2.getAuxItem("parent"));
        CHECK(hJson1.getAuxItem("parent_name.1") == hJson2.getAuxItem("parent_name.1"));
        CHECK(hJson1.getExtItem("name") == hJson2.getExtItem("name"));

    } catch (std::exception &e) {
        CHECK(false); // exception not expected
    }
    //  @end

    printf ("OK\n");
}
