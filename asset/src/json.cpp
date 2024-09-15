/*
 *
 * Copyright (C) 2015 - 2018 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "asset/json.h"
#include "asset/asset-computed.h"
#include "asset/asset-manager.h"
#include <fty/string-utils.h>
#include <fty_common.h>
#include <fty_common_db_asset.h>
#include <fty_shm.h>
#include <fty_log.h>

namespace fty::asset {

static double s_rack_realpower_nominal(const std::string& name)
{
    std::string value;
    int r = fty::shm::read_metric_value(name, "realpower.nominal", value);
    if (r != 0) {
        log_warning("Failed to read realpower.nominal@%s", name.c_str());
        return 0.0;
    }

    try {
        return std::stod(value);
    }
    catch (const std::exception&) {
        log_error("realpower.nominal@%s is NaN (value: %s)", name.c_str(), value.c_str());
    }
    return std::nan("");
}

// property json (name, value) serializer (<string>, <bool>)
static std::string s_jsonify(const std::string& name, const std::string& value)
{
    return "\"" + name + "\":\"" + value + "\"";
}
static std::string s_jsonify(const std::string& name, bool value)
{
    return "\"" + name + "\":" + (value ? "true" : "false");
}

// asset json serializer
std::string getJsonAsset(uint32_t elemId)
{
    // Get informations from database
    auto tmp = AssetManager::getItem(elemId);
    if (!tmp) {
        log_error("%s", tmp.error().toString().c_str());
        return "";
    }

    // get asset friendly name (ext. name)
    std::string asset_ext_name;
    {
        std::pair<std::string, std::string> asset_names = DBAssets::id_to_name_ext_name(tmp->id);
        if (asset_names.first.empty() && asset_names.second.empty()) {
            log_error("Database failure");
            return "";
        }
        asset_ext_name = asset_names.second; // asset friendly name
    }

    std::string json = "{";
    json.reserve(1024 * 6); // experimental max value (to limit memory realloc)

    json += s_jsonify("id", tmp->name)
         + "," + s_jsonify("name", asset_ext_name)
         + "," + s_jsonify("status", tmp->status)
         + "," + s_jsonify("priority", "P" + std::to_string(tmp->priority))
         + "," + s_jsonify("type", tmp->typeName)
         + "," + s_jsonify("power_devices_in_uri", "/api/v1/assets\?in=" + tmp->name + "&sub_type=epdu,pdu,feed,genset,ups,sts,rackcontroller");

    // if element is located, then print the location
    if (tmp->parentId != 0) {
        std::string parent_name, parent_ext_name;
        {
            auto parent_names = db::idToNameExtName(tmp->parentId);
            if (parent_names) {
                parent_name     = parent_names->first; // parent assetId
                parent_ext_name = parent_names->second; // parent friendly name
            }
        }
        // Get informations from database
        auto parentAsset = AssetManager::getItem(tmp->parentId);

        json += "," + s_jsonify("location_uri", "/api/v1/asset/" + parent_name)
             + "," + s_jsonify("location_id", parent_name)
             + "," + s_jsonify("location", parent_ext_name)
             + "," + s_jsonify("location_type", trimmed(parentAsset->typeName));
    }
    else {
        json += "," + s_jsonify("location", "")
             + "," + s_jsonify("location_type", "");
    }

    // every element (except groups) can be placed in some group
    json += ", \"groups\": [";
    if (!tmp->groups.empty()) {
        bool first = true;
        for (const auto& group : tmp->groups) {
            std::pair<std::string, std::string> names = DBAssets::id_to_name_ext_name(group.first);
            if (names.first.empty() && names.second.empty()) {
                log_error("Database failure");
                return "";
            }

            json += std::string{first ? "" : ","} + "{"
                 + s_jsonify("id", group.second) // assetId
                 + "," + s_jsonify("name", names.second) // ext. name
                 + "}";
            first = false;
        }
    }
    json += "]";

    // Device is special element with more attributes
    if (tmp->typeId == persist::asset_type::DEVICE) {
        json += ", \"powers\": [";

        if (!tmp->powers.empty()) {
            bool first = true;
            for (const auto& oneLink : tmp->powers) {
                auto link_names = db::idToNameExtName(oneLink.srcId);
                if (!link_names || (link_names->first.empty() && link_names->second.empty())) {
                    log_error("Database failure");
                    return "";
                }

                json += std::string{first ? "" : ","} + "{"
                     + s_jsonify("src_name", link_names->second)
                     + "," + s_jsonify("src_id", oneLink.srcName);
                if (!oneLink.srcSocket.empty()) {
                    json += "," + s_jsonify("src_socket", oneLink.srcSocket);
                }
                if (!oneLink.destSocket.empty()) {
                    json += "," + s_jsonify("dest_socket", oneLink.destSocket);
                }
                json += "}";
                first = false;
            }
        }

        json += "]";
    }

    // ACE: to be consistent with RFC-11 this was put here
    if (tmp->typeId == persist::asset_type::GROUP) {
        auto it = tmp->extAttributes.find("type");
        if (it != tmp->extAttributes.end()) {
            json += "," + s_jsonify("sub_type", trimmed(it->second.value));
            tmp->extAttributes.erase(it);
        }
    }
    else {
        json += "," + s_jsonify("sub_type", trimmed(tmp->subtypeName));

        json += ", \"parents\": [";
        bool first = true;
        for (const auto& it : tmp->parents) {
            std::pair<std::string, std::string> it_names = DBAssets::id_to_name_ext_name(std::get<0>(it));
            if (it_names.first.empty() && it_names.second.empty()) {
                log_error("Database failure");
                return "";
            }

            json += std::string{first ? "" : ","} + "{"
                 + s_jsonify("id", std::get<1>(it))
                 + "," + s_jsonify("name", it_names.second) // ext. name
                 + "," + s_jsonify("type", std::get<2>(it))
                 + "," + s_jsonify("sub_type", std::get<3>(it))
                 + "}";
            first = false;
        }
        json += "]";
    }

    // Map(<key>, pair(<value>, <readOnly>))
    using Outlet = std::map<std::string, std::pair<std::string, bool>>;

    // built datas from extended attributes
    std::map<std::string, Outlet> outlets;
    std::vector<std::string> ips;
    std::vector<std::string> macs;
    std::vector<std::string> fqdns;
    std::vector<std::string> hostnames;

    // Print "ext" attributes
    json += ", \"ext\": [";
    {
        bool firstExtAttribute = true;

        if (!tmp->assetTag.empty()) {
            json += std::string{firstExtAttribute ? "" : ","} + "{"
                 + s_jsonify("asset_tag", tmp->assetTag)
                 + "," + s_jsonify("read_only", false)
                 + "}";
            firstExtAttribute = false;
        }

        if (!tmp->extAttributes.empty()) {
            static const std::regex r_outlet_main("^outlet\\.(label|switchable)$");
            static const std::regex r_outlet("^outlet\\.([0-9]*)\\.(label|group|type|switchable)$");
            static const std::regex r_ip("^ip\\.[0-9][0-9]*$");
            static const std::regex r_mac("^mac\\.[0-9][0-9]*$");
            static const std::regex r_hostname("^hostname\\.[0-9][0-9]*$");
            static const std::regex r_fqdn("^fqdn\\.[0-9][0-9]*$");
            std::smatch r_match;

            const bool isUPS{tmp->subtypeName == "ups"};

            for (const auto& oneExt : tmp->extAttributes) {
                const std::string attrName = oneExt.first; // key
                const std::string attrValue = oneExt.second.value;
                const bool isReadOnly = oneExt.second.readOnly;

                // filter attributes printed yet
                if ((attrName == "name") || (attrName == "location_type")) {
                    continue;
                }
                // Add main outlet for ups
                else if (isUPS && std::regex_match(attrName, r_match, r_outlet_main)) {
                    if (r_match.size() == 2) {
                        auto key = r_match[1].str();
                        auto it  = outlets.find("0"); // main outlet
                        if (it == outlets.cend()) {
                            auto r = outlets.emplace("0", Outlet());
                            it = r.first;
                        }
                        it->second[key].first  = attrValue;
                        it->second[key].second = isReadOnly;
                    }
                    continue;
                }
                // Add other outlets
                else if (std::regex_match(attrName, r_match, r_outlet)) {
                    if (r_match.size() == 3) {
                        auto oNumber = r_match[1].str();
                        auto key     = r_match[2].str();
                        auto it      = outlets.find(oNumber);
                        if (it == outlets.cend()) {
                            auto r = outlets.emplace(oNumber, Outlet());
                            it = r.first;
                        }
                        it->second[key].first  = attrValue;
                        it->second[key].second = isReadOnly;
                    }
                    continue;
                }
                else if (std::regex_match(attrName, r_ip)) {
                    ips.push_back(attrValue);
                    continue;
                }
                else if (std::regex_match(attrName, r_mac)) {
                    macs.push_back(attrValue);
                    continue;
                }
                else if (std::regex_match(attrName, r_fqdn)) {
                    fqdns.push_back(attrValue);
                    continue;
                }
                else if (std::regex_match(attrName, r_hostname)) {
                    hostnames.push_back(attrValue);
                    continue;
                }

                // If we are here then this attribute is not special
                // and should be returned as "ext"
                json += std::string{firstExtAttribute ? "" : ","} + "{"
                     + s_jsonify(attrName, attrValue)
                     + "," + s_jsonify("read_only", isReadOnly)
                     + "}";
                firstExtAttribute = false;
            }
        }
    }
    json += "]"; // end "ext"

    // HOTFIX: construct all missing outlets for sts device
    if ((tmp->subtypeName == "sts") && (outlets.size() == 0)) {
        // get outlet.count
        auto it = tmp->extAttributes.find("outlet.count");
        if (it != tmp->extAttributes.end()) {
            const int outletCount = std::atoi(it->second.value.c_str());
            // construct each missing outlet, e.g for N outlets:
            // "outlets":{
            //  "1":[{name: "label", value: "Outlet 1", read_only: true}],
            //   ...
            //  "N":[{name: "label", value: "Outlet N", read_only: true}],
            // }
            for (int iOutlet = 1; iOutlet <= outletCount; iOutlet++) {
                const std::string outletId = std::to_string(iOutlet);
                const std::string outletName = std::string{"Outlet "} + outletId;

                auto r = outlets.emplace(outletId, Outlet());
                r.first->second["label"].first  = outletName;
                r.first->second["label"].second = true; // ReadOnly
            }
        }
    }

    // fn to print a named vector of strings in json (nop if vs is empty)
    auto printVS = [&json](const std::string& name, const std::vector<std::string>& vs) -> void
    {
        if (vs.empty()) { return; }
        std::string aux;
        bool first = true;
        for (const auto& s : vs) {
            aux += std::string{first ? "" : ","} + "\"" + s + "\"";
            first = false;
        }
        json += ", \"" + name + "\": [" + aux + "]";
    };

    // Print "ips"
    printVS("ips", ips);
    // Print "macs"
    printVS("macs", macs);
    // Print "fqdns"
    printVS("fqdns", fqdns);
    // Print "hostnames"
    printVS("hostnames", hostnames);

    // Print "outlets"
    if (!outlets.empty()) {
        //using Outlet = std::map<std::string, std::pair<std::string, bool>>;
        //std::map<std::string, Outlet> outlets;

        json += ", \"outlets\": {";
        bool firstOutlet = true;
        for (const auto& oneOutlet : outlets) {
            json += std::string{firstOutlet ? "" : ","}
                 + "\"" + oneOutlet.first + "\": [";

            // loop on outlet keys
            bool firstKey = true;
            for (const auto& it : oneOutlet.second) {
                const std::string name = it.first; // key
                const std::string value = it.second.first;
                const bool isReadOnly = it.second.second;

                json += std::string{firstKey ? "" : ","} + "{"
                     + s_jsonify("name", name)
                     + "," + s_jsonify("value", value)
                     + "," + s_jsonify("read_only", isReadOnly)
                     + "}";
                firstKey = false;
            }

            json += "]";
            firstOutlet = false;
        }
        json += "}";
    }

    // Print "computed" (object)
    json += ", \"computed\": {";
    if (persist::is_rack(tmp->typeId)) {
        int    freeusize         = free_u_size(tmp->id);
        double realpower_nominal = s_rack_realpower_nominal(tmp->name.c_str());

        std::map<std::string, int> outlets_available;
        int r = rack_outlets_available(tmp->id, outlets_available);
        if (r != 0) {
            log_error("Database failure");
            return "";
        }

        json += "\"freeusize\":" + (freeusize >= 0 ? std::to_string(freeusize) : "null")
             + "," + "\"realpower.nominal\":" + (!std::isnan(realpower_nominal) ? std::to_string(realpower_nominal) : "null");

        json += ",\"outlet.available\": {";
        bool first = true;
        for (const auto& it : outlets_available) {
            json += std::string{first ? "" : ","}
                 + "\"" + it.first + "\":" + ((it.second >= 0) ? std::to_string(it.second) : "null");
            first = false;
        }
        json += "}";
    }
    json += "}"; // end "computed"

    json += "}"; // end json

    //log_debug("=== getJsonAsset '%s': %s", tmp->name.c_str(), json.c_str());
    return json;
}

} // namespace fty::asset
