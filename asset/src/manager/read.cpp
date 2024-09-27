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

#include "asset/asset-db.h"
#include "asset/asset-manager.h"
#include <fty_common_asset_types.h>
#include <fty_common_db_connection.h>
#include <fty_log.h>

namespace fty::asset {

// throw on error
static std::vector<std::tuple<uint32_t, std::string, std::string, std::string>> getParents(uint32_t id)
{
    std::vector<std::tuple<uint32_t, std::string, std::string, std::string>> ret{};

    auto cb = [&ret](const fty::db::Row& row) {
        static const std::vector<std::tuple<std::string, std::string, std::string, std::string>> NAMES = {
            std::make_tuple("id_parent1", "parent_name1", "id_type_parent1", "id_subtype_parent1"),
            std::make_tuple("id_parent2", "parent_name2", "id_type_parent2", "id_subtype_parent2"),
            std::make_tuple("id_parent3", "parent_name3", "id_type_parent3", "id_subtype_parent3"),
            std::make_tuple("id_parent4", "parent_name4", "id_type_parent4", "id_subtype_parent4"),
            std::make_tuple("id_parent5", "parent_name5", "id_type_parent5", "id_subtype_parent5"),
            std::make_tuple("id_parent6", "parent_name6", "id_type_parent6", "id_subtype_parent6"),
            std::make_tuple("id_parent7", "parent_name7", "id_type_parent7", "id_subtype_parent7"),
            std::make_tuple("id_parent8", "parent_name8", "id_type_parent8", "id_subtype_parent8"),
            std::make_tuple("id_parent9", "parent_name9", "id_type_parent9", "id_subtype_parent9"),
            std::make_tuple("id_parent10", "parent_name10", "id_type_parent10", "id_subtype_parent10"),
        };

        for (const auto& it : NAMES) {
            std::string name = row.get(std::get<1>(it));
            if (name.empty()) { continue; }

            uint32_t pid        = row.get<uint32_t>(std::get<0>(it));
            uint16_t id_type    = row.get<uint16_t>(std::get<2>(it));
            uint16_t id_subtype = row.get<uint16_t>(std::get<3>(it));

            ret.push_back(std::make_tuple(pid, name, persist::typeid_to_type(id_type), persist::subtypeid_to_subtype(id_subtype)));
        }
    };

    int r = db::selectAssetElementSuperParent(id, cb);
    if (r == -1) {
        logError("select_asset_element_super_parent failed");
        throw std::runtime_error("DBAssets::select_asset_element_super_parent () failed.");
    }

    return ret;
}

AssetExpected<AssetManager::AssetList> AssetManager::getItems(
    const std::string& typeName, const std::string& subtypeName, const std::string& order, OrderDir orderDir)
{
    return getItems(typeName, std::vector<std::string>{subtypeName}, order, orderDir);
}

AssetExpected<AssetManager::AssetList> AssetManager::getItems(
    const std::string& typeName, const std::vector<std::string>& subtypeName, const std::string& order, OrderDir orderDir)
{
    try {
        uint16_t typeId = persist::type_to_typeid(typeName);
        if (typeId == persist::asset_type::TUNKNOWN) {
            return unexpected("Expected datacenters,rooms,ros,racks,devices"_tr);
        }

        std::vector<uint16_t> subtypeIds;
        if (typeName == "device") {
            for (const auto& name : subtypeName) {
                auto id = persist::subtype_to_subtypeid(name);
                if (id == persist::asset_subtype::SUNKNOWN) {
                    return unexpected("Expected ups, epdu, pdu, genset, sts, server, feed"_tr);
                }
                subtypeIds.emplace_back(id);
            }
        }

        auto els = db::selectShortElements(typeId, subtypeIds, order, orderDir == OrderDir::Asc ? "ASC" : "DEST");
        if (!els) {
            return unexpected(els.error());
        }
        return *els;
    }
    catch (const std::exception& e) {
        return unexpected(e.what());
    }
}

AssetExpected<Dto> AssetManager::getDto(const std::string& iname)
{
    try {
        auto id = db::nameToAssetId(iname);
        if (!id) {
            return fty::unexpected(id.error());
        }

        Dto asset;
        if (auto ret = db::selectAssetElementById(*id, asset); !ret) {
            return fty::unexpected(ret.error());
        }
        return std::move(asset);
    }
    catch (const std::exception& e) {
        return unexpected(e.what());
    }
}

AssetExpected<db::WebAssetElementExt> AssetManager::getItem(uint32_t id)
{
    try {
        db::WebAssetElementExt el;
        if (auto ret = db::selectAssetElementWebById(id, el); !ret) {
            return unexpected(ret.error());
        }

        if (auto ret = db::selectExtAttributes(id)) {
            el.extAttributes = *ret;
        }
        else {
            return unexpected(ret.error());
        }

        if (auto ret = db::selectAssetElementGroups(id)) {
            el.groups = *ret;
        }
        else {
            return unexpected(ret.error());
        }

        if (el.typeId == persist::asset_type::DEVICE) {
            if (auto powers = db::selectAssetDeviceLinksTo(id, INPUT_POWER_CHAIN)) {
                el.powers = *powers;
            } else {
                return unexpected(powers.error());
            }
        }

        el.parents = getParents(id);

        return std::move(el);
    }
    catch (const std::exception& e) {
        return unexpected(e.what());
    }
}

} // namespace fty::asset
