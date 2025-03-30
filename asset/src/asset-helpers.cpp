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
    if (!assetFilter.manufacturer.empty() && !assetFilter.serial.empty()) {
        // manufacturer & serial are defined (as for a real device)

        auto getNamespace = [] () {
            static uuid_t ns_uuid = ""; // namespace uuid
            static bool first{true};
            if (first) {
                const char* NS = "\x93\x3d\x6c\x80\xde\xa9\x8c\x6b\xd1\x11\x8b\x3b\x46\xa1\x81\xf1";
                uuid_parse(const_cast<char*>(NS), ns_uuid);
                first = false; // ns_uuid is built once (constant)
            }
            return ns_uuid;
        };

        auto sanitizedMacAddress = [&assetFilter] () {
            std::string macAddr{assetFilter.macAddress};
            // rm spaces & colons from macAddress
            for (const auto& c : {' ', ':'}) {
                std::string::iterator end_pos = std::remove(macAddr.begin(), macAddr.end(), c);
                macAddr.erase(end_pos, macAddr.end());
            }
            return macAddr; // sanitized
        };

        // set src = uppercase(manufacturer+serial+sanitize(macAddress))
        std::string src = assetFilter.manufacturer + assetFilter.serial + sanitizedMacAddress();
        std::transform(src.begin(), src.end(), src.begin(), ::toupper);

        // build sha1 uuid (namespace + src)
        uuid_t aux;
        uuid_generate_sha1(aux, getNamespace(), src.c_str(), src.length());

        char uuid[UUID_STR_LEN];
        memset(uuid, 0, sizeof(uuid));
        uuid_unparse_lower(aux, uuid);

        log_debug("SHA1 UUID: %s", uuid);

        return Uuid{uuid, UUID_TYPE_VERSION_5}; // sha1
    }
    else {
        // build random uuid
        uuid_t aux;
        uuid_generate_random(aux);

        char uuid[UUID_STR_LEN];
        memset(uuid, 0, sizeof(uuid));
        uuid_unparse_lower(aux, uuid);

        log_debug("RAND UUID: %s", uuid);

        return Uuid{uuid, UUID_TYPE_VERSION_4}; // random
    }
}

AssetExpected<uint32_t> checkElementIdentifier(const std::string& paramName, const std::string& paramValue)
{
    if (paramValue.empty()) {
        return unexpected(error(Errors::ParamRequired).format(paramName));
    }

    const std::string prohibitedChars = "_@%;\"";
    for (const auto& c : prohibitedChars) {
        if (paramValue.find(c) != std::string::npos) {
            std::string err = "value '{}' contains prohibited characters ({})"_tr.format(paramValue, prohibitedChars);
            std::string expected = "valid identifier"_tr;
            return unexpected(error(Errors::BadParams).format(paramName, err, expected));
        }
    }

    auto eid = db::nameToAssetId(paramValue);
    if (!eid) {
        std::string err = "value '{}' is not valid identifier. Error: {}"_tr.format(paramValue, eid.error());
        std::string expected = "existing identifier"_tr;
        return unexpected(error(Errors::BadParams).format(paramName, err, expected));
    }

    return eid.value(); // ok
}

AssetExpected<std::string> sanitizeDate(const std::string& dateIn)
{
    static std::vector<std::string> formats = {
        "%d-%m-%Y",
        "%Y-%m-%d",
        "%d-%b-%y",
        "%d.%m.%Y",
        "%d %m %Y",
        "%m/%d/%Y"
    };

    struct tm timeinfo;
    for (const auto& fmt : formats) {
        if (!strptime(dateIn.c_str(), fmt.c_str(), &timeinfo)) {
            continue; // fmt don't match dateIn
        }

        char buf[64];
        memset(buf, 0, sizeof(buf));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        std::strftime(buf, sizeof(buf), fmt.c_str(), &timeinfo);
#pragma GCC diagnostic pop
        return std::string{buf};
    }

    return unexpected("Not is ISO date"_tr);
}

AssetExpected<double> sanitizeValueDouble(const std::string& key, const std::string& value)
{
    try {
        std::size_t pos = 0;
        double d_value = std::stod(value, &pos);
        if (pos != value.length()) {
            return unexpected(error(Errors::BadParams).format(key, value, "value should be a number"_tr));
        }
        return d_value;
    }
    catch (const std::exception&) {
        return unexpected(error(Errors::BadParams).format(key, value, "value should be a number"_tr));
    }
}

AssetExpected<void> tryToPlaceAsset(uint32_t id, uint32_t parentId, uint32_t size, uint32_t loc)
{
    auto attr = db::selectExtAttributes(parentId);
    if (!attr) {
        return {}; // ok ?!
    }

    if (size == 0) {
        return unexpected("Size is wrong, should be greater than 0"_tr);
    }
    if (loc == 0) {
        return unexpected("Position is wrong, should be greater than 0"_tr);
    }
    if (attr->count("u_size") == 0) {
        return unexpected("Size is not set"_tr);
    }

    std::vector<bool> place;
    place.resize(convert<size_t>(attr->at("u_size").value), false);

    auto children = db::selectAssetsByParent(parentId);
    if (!children) {
        return {};
    }

    // get serial_no for id
    std::string id_serial_no; //empty
    {
        auto ext = db::selectExtAttributes(id);
        if (ext && (ext->count("serial_no") != 0)) {
            id_serial_no = ext->at("serial_no").value;
        }
    }

    for (const auto& child : *children) {
        if (child == id) {
            continue;
        }

        auto chAttr = db::selectExtAttributes(child);
        if (!chAttr
            || (chAttr->count("u_size") == 0)
            || (chAttr->count("location_u_pos") == 0)
        ) {
            continue; // the child does not have u_size/location_u_pos, ignore it
        }

        // handle Hercule UPS exception (multi-card device)
        if (!id_serial_no.empty()
            && (chAttr->count("serial_no") != 0)
            && (id_serial_no == chAttr->at("serial_no").value)
        ) {
            continue; // same device, ignore it (can overlap)
        }

        size_t isize = 0;
        size_t iloc = 0;
        try {
            isize = convert<size_t>(chAttr->at("u_size").value);
            iloc  = convert<size_t>(chAttr->at("location_u_pos").value) - 1;
        }
        catch (...) {
            return unexpected("Asset child u_size/location_u_pos is not a number"_tr);
        }

        for (size_t i = iloc; i < (iloc + isize); ++i) {
            if (i < place.size()) {
                place[i] = true;
            }
        }
    }

    for (size_t i = loc - 1; i < (loc + size - 1); ++i) {
        if (i >= place.size()) {
            return unexpected("Asset is out bounds"_tr);
        }
        if (place[i]) {
            return unexpected("Asset place is occupied"_tr);
        }
    }

    return {}; // ok, loc/size is a free place
}

AssetExpected<void> checkDuplicatedAsset(const AssetFilter& assetFilter)
{
    Uuid uuid = generateUUID(assetFilter);

    // choose extended asset keytag/value to search in db
    std::string keytag, value;
    switch (uuid.type) {
        case UUID_TYPE_VERSION_4:
            keytag = "ip.1";
            value = assetFilter.ipAddr;
            break;
        case UUID_TYPE_VERSION_5:
            keytag = "uuid";
            value = uuid.uuid;
            break;
        default:
            logError("Unexpected uuid type ({})", uuid.type);
            return unexpected("Unexpected uuid type ({})"_tr.format(uuid.type));
    }

    auto res = fty::asset::db::selectExtAttributes({{"keytag", keytag}, {"value", value}});
    if (!res) {
        return unexpected("Select data base failed"_tr);
    }
    if (res->size() != 0) {
        std::string criteria = keytag + "=" + value;
        logError("Asset with {} already exist", criteria);
        return unexpected(error(Errors::ElementAlreadyExist).format(criteria));
    }

    return {}; // ok, no duplicate in db
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
            AND id_asset_element != :assetId
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

        int num = -1;
        std::smatch match;
        for (const auto& row : rows) {
            std::string val = row.get("value");
            if (std::regex_search(val, match, rex)) {
                int tnum = fty::convert<int>(match[1].str());
                if (tnum > num) {
                    num = tnum;
                }
            }
            else if (num == -1) {
                num = 0;
            }
        }

        if (num != -1) {
            std::string suffix = fty::convert<std::string>(num + 1);

            name = origName.substr(0, maxLen - 1 - suffix.length());
            name = fmt::format("{}~{}", name, suffix);
        }
    }
    catch (const std::exception& e) {
        logError("{}", e.what());
        return fty::unexpected("Exception: {}", e.what());
    }
    return name;
}

} // namespace fty::asset
