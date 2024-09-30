/*  =========================================================================
    asset_conversion_proto - asset/conversion/proto

    Copyright (C) 2016 - 2020 Eaton

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
    =========================================================================
*/

#include "conversion/proto.h"

#include <fty_asset_dto.h>
#include <fty_common_db.h>
#include <fty_log.h>
#include <fty/convert.h>
#include <fty_proto.h>
#include <string>

namespace fty {
namespace conversion {

    // fty-proto/Asset conversion
    // return a valid fty_proto_t* object, else throw exception
    fty_proto_t* toFtyProto(const fty::Asset& asset, const std::string& operation, bool test)
    {
        fty_proto_t* proto = fty_proto_new(FTY_PROTO_ASSET);
        if (!proto) {
            log_error("fty_proto_new() failed");
            throw std::runtime_error("Memory allocation failed");
        }

        fty_proto_set_name(proto, "%s", asset.getInternalName().c_str());
        fty_proto_set_operation(proto, "%s", operation.c_str());

        fty_proto_aux_insert(proto, "priority", "%s", std::to_string(asset.getPriority()).c_str());
        fty_proto_aux_insert(proto, "type", "%s", asset.getAssetType().c_str());
        fty_proto_aux_insert(proto, "subtype", "%s", asset.getAssetSubtype().c_str());
        fty_proto_aux_insert(proto, "status", "%s", assetStatusToString(asset.getAssetStatus()).c_str());

        // aux/parent
        std::string parent{"0"};
        if (!test && !asset.getParentIname().empty()) {
            try {
                auto parentId = DBAssets::name_to_asset_id(asset.getParentIname());
                if (parentId < 0) {
                    log_error("Could not find parent ID from iname %s", asset.getParentIname().c_str());
                    throw std::runtime_error("Could not find parent ID from iname " + asset.getParentIname());
                }
                parent = std::to_string(parentId);
            }
            catch (const std::exception& e) {
                fty_proto_destroy(&proto);
                log_error("Invalid conversion from Asset to fty_proto_t : %s", e.what());
                throw std::runtime_error("Invalid conversion from Asset to fty_proto_t : " + std::string(e.what()));
            }
        }
        fty_proto_aux_insert(proto, "parent", "%s", parent.c_str());

        // extended attributes
        for (const auto& i : asset.getExt()) {
            fty_proto_ext_insert(proto, i.first.c_str(), "%s", i.second.getValue().c_str());
        }

        return proto;
    }

    void fromFtyProto(fty_proto_t* proto, fty::Asset& asset, bool extAttributeReadOnly, bool test)
    {
        if (fty_proto_id(proto) != FTY_PROTO_ASSET) {
            log_error("proto is not a FTY_PROTO_ASSET");
            throw std::invalid_argument("Wrong message type");
        }

        std::string assetStatus(fty_proto_aux_string(proto, "status", "active"));

        asset.setInternalName(fty_proto_name(proto));
        asset.setAssetStatus(fty::stringToAssetStatus(assetStatus));
        asset.setAssetType(fty_proto_aux_string(proto, "type", ""));
        asset.setAssetSubtype(fty_proto_aux_string(proto, "subtype", ""));
        asset.setPriority(static_cast<int>(fty_proto_aux_number(proto, "priority", 5)));

        //parent
        if (test) {
            asset.setParentIname("test-parent");
        }
        else {
            std::string parentId(fty_proto_aux_string(proto, "parent", ""));
            if (parentId != "0") {
                try {
                    auto parentIname = DBAssets::id_to_name_ext_name(fty::convert<uint32_t>(parentId)).first;
                    if (parentIname.empty()) {
                        log_error("Could not get internal name from ID %s", parentId.c_str());
                        throw std::runtime_error("Could not get internal name from ID " + parentId);
                    }
                    asset.setParentIname(parentIname);
                }
                catch (const std::exception& e) {
                    log_error("Invalid conversion from fty_proto_t to Asset: %s", e.what());
                    throw std::runtime_error("Invalid conversion from fty_proto_t to Asset: " + std::string(e.what()));
                }
            }
        }

        zhash_t* hash = fty_proto_ext(proto);

        for (auto* item = zhash_first(hash); item; item = zhash_next(hash)) {
            asset.setExtEntry(zhash_cursor(hash), static_cast<const char*>(item), extAttributeReadOnly);
        }

        // force RW for specific attributes (if default is read only)
        if (extAttributeReadOnly) {
            // PQSWPRG-7607 HOTFIX: force RW for friendly name ('name' ext. attribute) and also for ip.1
            asset.setExtEntry("name", asset.getExtEntry("name"), false);
            asset.setExtEntry("ip.1", asset.getExtEntry("ip.1"), false);
            // all endpoint attribs are RW
            for (const auto& att : asset.getExt()) {
                //An endpoint is always "endpoint.<params>"
                if(att.first.find("endpoint.") == 0) {
                    asset.setExtEntry(att.first, att.second.getValue(), false);
                }
            }
        }//
    }

}
} // namespace fty::conversion
