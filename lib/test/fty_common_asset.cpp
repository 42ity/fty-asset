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
        assert(asset1 == asset2);
    } catch (std::exception &e) {
        assert (false); // exception not expected
    }
}

TEST_CASE("fty_common_asset test")
{
    printf (" * fty_common_asset: \n");

    //  @selftest
    try {
        //fty::BasicAsset a; // this causes g++ error, as expected
        fty::BasicAsset b ("id-1", "active", "device", "rackcontroller");
        assert (b.getId () == "id-1");
        assert (b.getStatus () == fty::BasicAsset::Status::Active);
        assert (b.getType () == persist::asset_type::DEVICE);
        assert (b.getSubtype () == persist::asset_subtype::RACKCONTROLLER);
        assert (b.getStatusString () == "active");
        assert (b.getTypeString () == "device");
        assert (b.getSubtypeString () == "rackcontroller");
        b.setStatus ("nonactive");
        assert (b.getStatus () == fty::BasicAsset::Status::Nonactive);
        b.setType (fty::TYPE_VIRTUAL_MACHINE);
        assert (b.getType () == persist::asset_type::VIRTUAL_MACHINE);
        b.setSubtype (fty::SUB_VMWARE_VM);
        assert (b.getSubtype () == persist::asset_subtype::VMWARE_VM);

        fty::BasicAsset bb (b);
        assert (b == bb);
        assert (bb.getId () == "id-1");
        assert (bb.getType () == persist::asset_type::VIRTUAL_MACHINE);
        bb.setType ("device");
        assert (bb.getType () == persist::asset_type::DEVICE);
        assert (b.getType () == persist::asset_type::VIRTUAL_MACHINE);
        assert (b != bb);

        fty::BasicAsset bJson1 ("id-1", "active", "device", "rackcontroller");
        cxxtools::SerializationInfo rootSi;
        rootSi <<= bJson1;
        std::cerr << JSON::writeToString(rootSi, true) << std::endl;

        fty::BasicAsset bJson2;
        rootSi >>= bJson2;
        assert (bJson1.getId() == bJson2.getId());
        assert (bJson1.getStatusString() == bJson2.getStatusString());
        assert (bJson1.getTypeString() == bJson2.getTypeString());
        assert (bJson1.getSubtypeString() == bJson2.getSubtypeString());

    } catch (std::exception &e) {
        assert (false); // exception not expected
    }

    try {
        fty::BasicAsset c ("id-2", "invalid_status", "device", "rackcontroller");
        assert(false);
    } catch (std::exception &e) {
        // exception is expected
    }

    try {
        fty::BasicAsset d ("id-3", "active", "invalid_type", "rackcontroller");
        std::cerr << "d: " << d.toJson() << std::endl;
        assert (d.getType () == persist::asset_type::TUNKNOWN);
    } catch (std::exception &e) {
        assert (false); // exception not expected
    }

    try {
        fty::BasicAsset e ("id-4", "active", "device", "invalid_subtype");
        std::cerr << "e: " << e.toJson() << std::endl;
        assert (e.getSubtype () == persist::asset_subtype::SUNKNOWN);
    } catch (std::exception &e) {
        assert (false); // exception not expected
    }

    try {
        fty::ExtendedAsset f ("id-5", "active", "device", "rackcontroller", "MyRack", "id-1", 1);
        assert (f.getName () == "MyRack");
        assert (f.getParentId () == "id-1");
        assert (f.getPriority () == 1);
        assert (f.getPriorityString () == "P1");

        fty::ExtendedAsset g ("id-6", "active", "device", "rackcontroller", "MyRack", "parent-1", "P2");
        assert (f != g);
        assert (g.getPriority () == 2);
        assert (g.getPriorityString () == "P2");
        g.setName ("MyNewRack");
        assert (g.getName () == "MyNewRack");
        g.setParentId ("parent-2");
        assert (g.getParentId () == "parent-2");
        g.setPriority ("P3");
        assert (g.getPriority () == 3);
        g.setPriority (4);
        assert (g.getPriority () == 4);

        fty::ExtendedAsset gg (g);
        assert (g == gg);
        assert (gg.getId () == "id-6");
        assert (gg.getName () == "MyNewRack");
        gg.setName ("MyOldRack");
        assert (gg.getName () == "MyOldRack");
        assert (g.getName () == "MyNewRack");
        assert (g != gg);

        fty::ExtendedAsset fJson1 ("id-5", "active", "device", "rackcontroller", "MyRack", "id-1", 1);
        cxxtools::SerializationInfo rootSi;
        rootSi <<= fJson1;
        std::cerr << JSON::writeToString(rootSi, true) << std::endl;

        fty::ExtendedAsset fJson2;
        rootSi >>= fJson2;
        assert (fJson1.getId() == fJson2.getId());
        assert (fJson1.getStatusString() == fJson2.getStatusString());
        assert (fJson1.getTypeString() == fJson2.getTypeString());
        assert (fJson1.getSubtypeString() == fJson2.getSubtypeString());
        assert (fJson1.getName() == fJson2.getName());
        assert (fJson1.getParentId() == fJson2.getParentId());
        assert (fJson1.getPriority() == fJson2.getPriority());

    } catch (std::exception &e) {
        assert (false); // exception not expected
    }

    try {
        fty::FullAsset h ("id-7", "active", "device", "rackcontroller", "MyRack", "id-1", 1, {{"aux1", "aval1"},
                {"aux2", "aval2"}}, {});
        assert (h.getAuxItem ("aux2") == "aval2");
        assert (h.getAuxItem ("aval3").empty ());
        assert (h.getExtItem ("eval1").empty ());
        h.setAuxItem ("aux4", "aval4");
        assert (h.getAuxItem ("aux4") == "aval4");
        h.setExtItem ("ext5", "eval5");
        assert (h.getExtItem ("ext5") == "eval5");
        h.setExt ({{"ext1", "eval1"}});
        assert (h.getExtItem ("ext1") == "eval1");
        assert (h.getExtItem ("ext5") != "eval5");
        assert (h.getItem ("aux2") == "aval2");
        assert (h.getItem ("ext1") == "eval1");
        assert (h.getItem ("notthere").empty ());
        fty::FullAsset hh (h);
        assert (h == hh);
        assert (hh.getExtItem ("ext1") == "eval1");
        assert (hh.getExtItem ("ext6").empty ());
        hh.setExtItem ("ext6", "eval6");
        assert (hh.getExtItem ("ext6") == "eval6");
        assert (h.getExtItem ("ext6").empty ());
        assert (h != hh);

        fty::FullAsset hJson1 ("id-7", "active", "device", "rackcontroller", "MyRack", "id-1", 1, {{"parent", "1"},{"parent_name.1","id-1"}}, {{"name","MyRack"}});
        cxxtools::SerializationInfo rootSi;
        rootSi <<= hJson1;
        std::cerr << JSON::writeToString(rootSi, true) << std::endl;

        fty::FullAsset hJson2;
        rootSi >>= hJson2;

        assert (hJson1.getId() == hJson2.getId());
        assert (hJson1.getStatusString() == hJson2.getStatusString());
        assert (hJson1.getTypeString() == hJson2.getTypeString());
        assert (hJson1.getSubtypeString() == hJson2.getSubtypeString());
        assert (hJson1.getName() == hJson2.getName());
        assert (hJson1.getParentId() == hJson2.getParentId());
        assert (hJson1.getPriority() == hJson2.getPriority());

        assert (hJson1.getAuxItem("parent") == hJson2.getAuxItem("parent"));
        assert (hJson1.getAuxItem("parent_name.1") == hJson2.getAuxItem("parent_name.1"));
        assert (hJson1.getExtItem("name") == hJson2.getExtItem("name"));

    } catch (std::exception &e) {
        assert (false); // exception not expected
    }
    //  @end

    printf ("OK\n");
}
