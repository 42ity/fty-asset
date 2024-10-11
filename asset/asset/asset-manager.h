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

#pragma once

#include "asset-db.h"
#include "asset-dto.h"
#include "error.h"
#include <fty/expected.h>
#include <fty/translate.h>
#include <fty_common_db_asset.h>
#include <fty_common_db_exception.h>
#include <map>
#include <string>

namespace fty::asset {

enum class OrderDir
{
    Asc,
    Desc
};

class AssetManager
{
public:
    using AssetList  = std::vector<std::pair<uint32_t, std::string>>;
    using ImportList = std::map<size_t, Expected<uint32_t>>;

public:
    static AssetExpected<Dto> getDto(const std::string& iname);
    static AssetExpected<db::WebAssetElementExt> getItem(uint32_t id);

    static AssetExpected<AssetList> getItems(const std::string& typeName, const std::string& subtypeName,
        const std::string& order = "name", OrderDir orderDir = OrderDir::Asc);

    static AssetExpected<AssetList> getItems(const std::string& typeName, const std::vector<std::string>& subtypeName,
        const std::string& order = "name", OrderDir orderDir = OrderDir::Asc);

    static AssetExpected<db::AssetElement> deleteAsset(uint32_t id, bool sendNotify = true);

    static std::map<std::string, AssetExpected<db::AssetElement>> deleteAsset(
        const std::map<uint32_t, std::string>& ids, bool sendNotify = true);

    static AssetExpected<db::AssetElement> deleteAsset(const db::AssetElement& element, bool sendNotify = true);

    static AssetExpected<uint32_t> createAsset(
        const std::string& json, const std::string& user, bool sendNotify = true);

    static AssetExpected<uint32_t> createAsset(
        const cxxtools::SerializationInfo& serializationInfo, const std::string& user, bool sendNotify = true);

    static AssetExpected<ImportList> importCsv(const std::string& csv, const std::string& user, bool sendNotify = true);
    static AssetExpected<std::string> exportCsv(const std::optional<db::AssetElement>& dc = std::nullopt);

private:
    static AssetExpected<db::AssetElement> deleteDcRoomRowRack(const db::AssetElement& element);
    static AssetExpected<db::AssetElement> deleteGroup(const db::AssetElement& element);
    static AssetExpected<db::AssetElement> deleteDevice(const db::AssetElement& element);
    static AssetExpected<uint32_t>         importAsset(const cxxtools::SerializationInfo& si, const std::string& user, bool sendNotify, fty::Translate& msg);
};

} // namespace fty::asset
