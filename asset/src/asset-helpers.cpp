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

#include "asset/asset-helpers.h"
#include "asset/asset-db.h"
#include <fty_asset_dto.h>
#include <fty_common_agents.h>
#include <fty_common_db_connection.h>
#include <fty_common_mlm.h>
#include <fty_log.h>
#include <openssl/sha.h>
#include <ctime>
#include <regex>
#include <sstream>
#include <time.h>
#include <utility>
#include <uuid/uuid.h>

#define AGENT_ASSET_ACTIVATOR "etn-licensing-credits"

#define COMMAND_IS_ASSET_ACTIVABLE_INAME "IS_ASSET_ACTIVABLE_INAME"
#define COMMAND_ACTIVATE_ASSET_INAME     "ACTIVATE_ASSET_INAME"
#define COMMAND_DEACTIVATE_ASSET_INAME   "DEACTIVATE_ASSET_INAME"

namespace fty::asset {

Uuid generateUUID(const AssetFilter& assetFilter)
{
    static std::string ns = "\x93\x3d\x6c\x80\xde\xa9\x8c\x6b\xd1\x11\x8b\x3b\x46\xa1\x81\xf1";
    Uuid               result;

    if (!assetFilter.manufacturer.empty() && !assetFilter.model.empty() && !assetFilter.serial.empty()) {
        log_debug("generate full UUID");

        std::string src = ns + assetFilter.manufacturer + assetFilter.model + assetFilter.serial;
        // hash must be zeroed first
        std::array<unsigned char, SHA_DIGEST_LENGTH> hash;
        hash.fill(0);

        SHA1(reinterpret_cast<const unsigned char*>(src.c_str()), src.length(), hash.data());

        hash[6] &= 0x0F;
        hash[6] |= 0x50;
        hash[8] &= 0x3F;
        hash[8] |= 0x80;

        char uuid_char[37];
        memset(uuid_char, 0, sizeof(uuid_char));
        uuid_unparse_lower(hash.data(), uuid_char);

        result.uuid = uuid_char;
        result.type = UUID_TYPE_VERSION_5;
    } else {
        log_debug("generate random UUID");
        uuid_t u;
        uuid_generate_random(u);

        char uuid_char[37];
        memset(uuid_char, 0, sizeof(uuid_char));
        uuid_unparse_lower(u, uuid_char);

        result.uuid = uuid_char;
        result.type = UUID_TYPE_VERSION_4;
    }

    return result;
}

AssetExpected<uint32_t> checkElementIdentifier(const std::string& paramName, const std::string& paramValue)
{
    assert(!paramName.empty());
    if (paramValue.empty()) {
        return unexpected(error(Errors::ParamRequired).format(paramName));
    }

    static std::string prohibited = "_@%;\"";

    for (auto a : prohibited) {
        if (paramValue.find(a) != std::string::npos) {
            std::string err      = "value '{}' contains prohibited characters ({})"_tr.format(paramValue, prohibited);
            std::string expected = "valid identifier"_tr;
            return unexpected(error(Errors::BadParams).format(paramName, err, expected));
        }
    }

    if (auto eid = db::nameToAssetId(paramValue)) {
        return *eid;
    } else {
        std::string err      = "value '{}' is not valid identifier. Error: {}"_tr.format(paramValue, eid.error());
        std::string expected = "existing identifier"_tr;
        return unexpected(error(Errors::BadParams).format(paramName, err, expected));
    }
}

AssetExpected<std::string> sanitizeDate(const std::string& inp)
{
    static std::vector<std::string> formats = {"%d-%m-%Y", "%Y-%m-%d", "%d-%b-%y", "%d.%m.%Y", "%d %m %Y", "%m/%d/%Y"};

    struct tm timeinfo;
    for (const auto& fmt : formats) {
        if (!strptime(inp.c_str(), fmt.c_str(), &timeinfo)) {
            continue;
        }
        std::array<char, 11> buff;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        std::strftime(buff.data(), buff.size(), fmt.c_str(), &timeinfo);
#pragma GCC diagnostic pop
        return std::string(buff.begin(), buff.end());
    }

    return unexpected("Not is ISO date"_tr);
}

AssetExpected<double> sanitizeValueDouble(const std::string& key, const std::string& value)
{
    try {
        std::size_t pos     = 0;
        double      d_value = std::stod(value, &pos);
        if (pos != value.length()) {
            return unexpected(error(Errors::BadParams).format(key, value, "value should be a number"_tr));
        }
        return d_value;
    } catch (const std::exception&) {
        return unexpected(error(Errors::BadParams).format(key, value, "value should be a number"_tr));
    }
}

AssetExpected<void> tryToPlaceAsset(uint32_t id, uint32_t parentId, uint32_t size, uint32_t loc)
{
    auto attr = db::selectExtAttributes(parentId);
    if (!attr) {
        return {};
    }

    if (!loc) {
        return unexpected("Position is wrong, should be greater than 0"_tr);
    }

    if (!size) {
        return unexpected("Size is wrong, should be greater than 0"_tr);
    }

    if (!attr->count("u_size")) {
        return unexpected("Size is not set"_tr);
    }

    std::vector<bool> place;
    place.resize(convert<size_t>(attr->at("u_size").value), false);

    auto children = db::selectAssetsByParent(parentId);
    if (!children) {
        return {};
    }

    for (const auto& child : *children) {
        if (child == id) {
            continue;
        }

        auto chAttr = db::selectExtAttributes(child);
        if (!chAttr) {
            continue;
        }
        if (!chAttr->count("u_size") || !chAttr->count("location_u_pos")) {
            continue; // the child does not have u_size/location_u_pos, ignore it
        }

        size_t isize = 0;
        size_t iloc = 0;
        try {
            isize = convert<size_t>(chAttr->at("u_size").value);
            iloc  = convert<size_t>(chAttr->at("location_u_pos").value) - 1;
        } catch (...) {
            return unexpected("Asset child u_size/location_u_pos is not a number"_tr);
        }

        for (size_t i = iloc; i < iloc + isize; ++i) {
            if (i < place.size()) {
                place[i] = true;
            }
        }
    }

    for (size_t i = loc - 1; i < loc + size - 1; ++i) {
        if (i >= place.size()) {
            return unexpected("Asset is out bounds"_tr);
        }
        if (place[i]) {
            return unexpected("Asset place is occupied"_tr);
        }
    }

    return {};
}

AssetExpected<void> checkDuplicatedAsset(const AssetFilter& assetFilter)
{
    std::map<std::string, std::string> mapFilter;

    auto uuidAsset = generateUUID(assetFilter);
    if (uuidAsset.type == UUID_TYPE_VERSION_5) {
        mapFilter = {{"keytag", "uuid"}, {"value", uuidAsset.uuid}};
    } else {
        mapFilter = {{"keytag", "ip.1"}, {"value", assetFilter.ipAddr}};
    }

    auto res = fty::asset::db::selectExtAttributes(mapFilter);
    if (!res) {
        return unexpected("Select data base failed"_tr);
    }
    if (res->size() == 1) {
        std::string err = "Asset '{}' already exist, duplicate it is forbidden"_tr.format(uuidAsset.uuid);
        logError(err);
        return unexpected(error(Errors::ElementAlreadyExist).format(err));
    }
    return {};
}

static AssetExpected<std::vector<std::string>> activateRequest(const std::string& command, const std::string& data)
{
    try {
        mlm::MlmSyncClient client(AGENT_FTY_ASSET, AGENT_ASSET_ACTIVATOR);

        logDebug("Request {}, command({}), data({})", AGENT_ASSET_ACTIVATOR, command, data);

        std::vector<std::string> payload = {command, data};

        std::vector<std::string> receivedFrames = client.syncRequestWithReply(payload);

        // check if the first frame we get is an error
        if (receivedFrames.empty() || receivedFrames[0] == "ERROR") {
            if (receivedFrames.size() == 2) {
                return unexpected(receivedFrames.at(1));
            }
            else {
                return unexpected("Missing data for error");
            }
        }
        return receivedFrames;
    }
    catch (const std::exception& e) {
        return unexpected(e.what());
    }
}

AssetExpected<bool> activation::isActivable(const std::string& assetIName)
{
    const std::string jsonIName{"\"" + assetIName + "\""}; // json compliant (single string)
    if (auto ret = activateRequest(COMMAND_IS_ASSET_ACTIVABLE_INAME, jsonIName)) {
        logDebug("asset is activable? {}/{}", assetIName, ret->at(0));
        return fty::convert<bool>(ret->at(0));
    }
    else {
        logError("asset activable: {}/'{}'", assetIName, ret.error());
        return unexpected(ret.error());
    }
}

AssetExpected<bool> activation::isActivable(const FullAsset& asset)
{
    return isActivable(asset.getId());
}

AssetExpected<void> activation::activate(const std::string& assetIName)
{
    const std::string jsonListIName{"[\"" + assetIName + "\"]"}; // json compliant (string list)
    if (auto ret = activateRequest(COMMAND_ACTIVATE_ASSET_INAME, jsonListIName); !ret) {
        logError("asset {}: {}", assetIName, ret.error());
        return unexpected(ret.error());
    }
    return {};
}

AssetExpected<void> activation::activate(const FullAsset& asset)
{
    return activate(asset.getId());
}

AssetExpected<void> activation::deactivate(const std::string& assetIName)
{
    const std::string jsonListIName{"[\"" + assetIName + "\"]"}; // json compliant (string list)
    if (auto ret = activateRequest(COMMAND_DEACTIVATE_ASSET_INAME, jsonListIName); !ret) {
        logError("asset {}: {}", assetIName, ret.error());
        return unexpected(ret.error());
    }
    return {};
}

AssetExpected<void> activation::deactivate(const FullAsset& asset)
{
    return deactivate(asset.getId());
}

AssetExpected<std::string> normName(const std::string& origName, uint32_t maxLen, uint32_t assetId)
{
    if (origName.length() < maxLen) {
        return origName;
    }

    static std::regex  rex("^.*~(\\d+)$");
    static std::string sql = R"(
        SELECT value
        FROM   t_bios_asset_ext_attributes
        WHERE
            keytag = 'name'
            AND (
                value = :name OR
                value LIKE :mask1 OR
                value LIKE :mask2
            )
            AND id_asset_element != : assetId
    )";

    std::string name = origName.substr(0, maxLen);

    try {
        fty::db::Connection conn;

        // clang-format off
        auto rows = conn.select(sql,
            "name"_p    = name,
            "mask1"_p   = name.substr(0, maxLen-2) + "~%",
            "mask2"_p   = name.substr(0, maxLen-3) + "~%",
            "assetId"_p = assetId
        );
        // clang-format on

        int         num = -1;
        std::smatch match;
        for (const auto& row : rows) {
            std::string val = row.get("value");
            if (std::regex_search(val, match, rex)) {
                int tnum = fty::convert<int>(match[1].str());
                if (tnum > num) {
                    num = tnum;
                }
            } else if (num == -1) {
                num = 0;
            }
        }

        if (num != -1) {
            std::string suffix = fty::convert<std::string>(num + 1);

            name = origName.substr(0, maxLen - 1 - suffix.length());
            name = fmt::format("{}~{}", name, suffix);
        }
    } catch (const std::exception& ex) {
        logError(ex.what());
        return fty::unexpected("Exception: {}", ex.what());
    }
    return name;
}

} // namespace fty::asset
