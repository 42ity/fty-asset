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

#include "asset/asset-cam.h"
#include "asset/asset-configure-inform.h"
#include "asset/asset-helpers.h"
#include "asset/asset-import.h"
#include "asset/asset-manager.h"
#include "asset/csv.h"
#include <cxxtools/serializationinfo.h>
#include <fty_common_json.h>
#include <fty_asset_dto.h>
#include <fty_log.h>
#include <mutex>

namespace fty::asset {

#define AGENT_ASSET_ACTIVATOR "etn-licensing-credits"
#define CREATE_MODE_ONE_ASSET 1
#define CREATE_MODE_CSV       2

// ensure only 1 request is processed at the time
static std::mutex g_modification;

AssetExpected<uint32_t> AssetManager::createAsset(const std::string& json, const std::string& user, bool sendNotify)
{
    auto msg = "Request CREATE asset {} FAILED: {}"_tr;

    // Read json, transform to csv, use existing functionality
    cxxtools::SerializationInfo si;
    try {
        JSON::readFromString(json, si);
    }
    catch (const std::exception& e) {
        logError("{}", e.what());
        return unexpected(msg.format("", e.what()));
    }

    auto ret = importAsset(si, user, sendNotify, msg);
    if (!ret) {
        return unexpected(ret.error());
    }
    return ret.value();
}

AssetExpected<uint32_t> AssetManager::createAsset(
    const cxxtools::SerializationInfo& serializationInfo, const std::string& user, bool sendNotify)
{
    auto msg = "Request CREATE asset {} FAILED: {}"_tr;
    auto ret = importAsset(serializationInfo, user, sendNotify, msg);
    if (!ret) {
        return unexpected(ret.error());
    }
    return ret.value();
}

AssetExpected<uint32_t> AssetManager::importAsset(
    const cxxtools::SerializationInfo& si, const std::string& user, bool sendNotify, fty::Translate& msg)
{
    std::string itemName;
    si.getMember("name", itemName);

    logDebug("Create asset {}", itemName);

    const std::lock_guard<std::mutex> lock(g_modification);

    CsvMap cm;
    try {
        cm = CsvMap_from_serialization_info(si);
        cm.setCreateUser(user);
        cm.setCreateMode(CREATE_MODE_ONE_ASSET);
    }
    catch (const std::invalid_argument& e) {
        logError("{}", e.what());
        return unexpected(msg.format(itemName, e.what()));
    }
    catch (const std::exception& e) {
        logError("{}", e.what());
        return unexpected(msg.format(itemName, e.what()));
    }

    if (cm.cols() == 0 || cm.rows() == 0) {
        logError("Request CREATE asset {} FAILED, import is empty", itemName);
        return unexpected(msg.format(itemName, "Cannot import empty document."_tr));
    }

    // in underlying functions like update_device
    if (!cm.hasTitle("type")) {
        logError("Type is not set");
        return unexpected(msg.format(itemName, "Asset type is not set"));
    }
    if (cm.hasTitle("id")) {
        logError("key 'id' is forbidden to be used");
        return unexpected(msg.format(itemName, "key 'id' is forbidden to be used"_tr));
    }
    // If descriminant are available, check to not duplicate asset.
    if (cm.hasTitle("manufacturer") && cm.hasTitle("model") && cm.hasTitle("serial_no")) {
        logDebug("All discriminant data are available, checking to not duplicate asset");

        std::string ipAddr = cm.hasTitle("ip.1") ? cm.get(1, "ip.1") : "";
        AssetFilter assetFilter{cm.get(1, "manufacturer"), cm.get(1, "model"), cm.get(1, "serial_no"), ipAddr};

        auto ret = checkDuplicatedAsset(assetFilter);
        if (!ret) {
            return unexpected(msg.format(itemName, ret.error()));
        }
    }
    else {
        logError("Discriminant data are not availables, can not check duplicated asset");
    }

    if (sendNotify) {
        try {
            if (auto ret = activation::isActivable(itemName)) {
                if (!*ret) {
                    return unexpected(msg.format(itemName, "Asset cannot be activated"_tr));
                }
            }
            else {
                logError("{}", ret.error());
                return unexpected(msg.format(itemName, ret.error()));
            }
        }
        catch (const std::invalid_argument& e) {
            logError("{}", e.what());
            return unexpected(msg.format(itemName, e.what()));
        }
        catch (const std::exception& e) {
            logError("{}", e.what());
            return unexpected(msg.format(itemName, e.what()));
        }
    }

    // actual insert - throws exceptions
    Import import(cm);
    if (auto res = import.process(sendNotify)) {
        // here we are -> insert was successful
        // ATTENTION:  1. sending messages is "hidden functionality" from user
        //             2. if any error would occur during the sending message,
        //                user will never know if element was actually inserted
        //                or not
        const auto& imported = import.items();
        if (imported.find(1) == imported.end()) {
            return unexpected(msg.format(itemName, "Import failed"_tr));
        }

        if (imported.at(1)) {
            if (sendNotify) {
                const std::string agentName{generateMlmClientId("asset.create")};
                if (auto sent = sendConfigure(*(imported.at(1)), import.operation(), agentName); !sent) {
                    logError("{}", sent.error());
                    return unexpected(
                        "Error during configuration sending of asset change notification. Consult system log."_tr);
                }

                // no unexpected errors was detected
                // process results
                auto ret = db::idToNameExtName(imported.at(1)->id);
                if (!ret) {
                    logError("{}", ret.error());
                    return unexpected(msg.format(itemName, "Database failure"_tr));
                }

                try {
                    ExtMap map;
                    getExtMapFromSi(si, map);

                    const auto& assetIname = ret.value().first;

                    deleteMappings(assetIname);
                    auto credentialList = getCredentialMappings(map);
                    createMappings(assetIname, credentialList);
                }
                catch (const std::exception& e) {
                    logError("Failed to update CAM: {}", e.what());
                }
            }
            return imported.at(1)->id;
        }
        else {
            return unexpected(msg.format(itemName, imported.at(1).error()));
        }
    }
    else {
        return unexpected(msg.format(itemName, res.error()));
    }
}

} // namespace fty::asset
