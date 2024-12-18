/*  =========================================================================
    fty_asset_accessor

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

#include "fty_asset_accessor.h"

#include <cxxtools/serializationinfo.h>
#include <fty_common_json.h>
#include <fty_common_messagebus.h>
#include <fty_log.h>
#include <fty/convert.h>

namespace fty
{
    static constexpr const char *ASSET_AGENT = "asset-agent-ng";
    static constexpr const char *ASSET_AGENT_QUEUE = "FTY.Q.ASSET.QUERY";
    static constexpr const char *ACCESSOR_NAME = "fty-asset-accessor";
    static constexpr const char *ENDPOINT = MLM_DEFAULT_ENDPOINT;

    /// send a MessageBus synchronous request (can throw)
    static messagebus::Message sendSyncReq(const std::string& command, const messagebus::UserData& userdata)
    {
        // generate unique ID interface
        const std::string clientName{messagebus::getClientId(ACCESSOR_NAME)};

        const messagebus::MetaData metadata = {
            {messagebus::Message::TO, ASSET_AGENT},
            {messagebus::Message::CORRELATION_ID, messagebus::generateUuid()},
            {messagebus::Message::SUBJECT, command},
            {messagebus::Message::FROM, clientName},
            {messagebus::Message::REPLY_TO, clientName},
        };

        const int timeout_s{5}; // request timeout (seconds)
        std::unique_ptr<messagebus::MessageBus> interface(messagebus::MlmMessageBus(ENDPOINT, clientName));
        interface->connect();
        return interface->request(ASSET_AGENT_QUEUE, messagebus::Message{metadata, userdata}, timeout_s);
    }

    /// send a MessageBus asynchronous request (can throw)
    static void sendAsyncReq(const std::string& command, const messagebus::UserData& userdata)
    {
        // generate unique ID interface
        const std::string clientName{messagebus::getClientId(ACCESSOR_NAME)};

        const messagebus::MetaData metadata = {
            {messagebus::Message::TO, ASSET_AGENT},
            {messagebus::Message::CORRELATION_ID, messagebus::generateUuid()},
            {messagebus::Message::SUBJECT, command},
            {messagebus::Message::FROM, clientName},
            {messagebus::Message::REPLY_TO, clientName},
        };

        std::unique_ptr<messagebus::MessageBus> interface(messagebus::MlmMessageBus(ENDPOINT, clientName));
        interface->connect();
        interface->sendRequest(ASSET_AGENT_QUEUE, messagebus::Message{metadata, userdata});
        // no response expected
    }

    /// returns the asset database ID, given the internal name
    fty::Expected<uint32_t> AssetAccessor::assetInameToID(const std::string &iname)
    {
        try {
            if (iname.empty()) {
                throw std::runtime_error("iname is empty");
            }

            messagebus::Message ret = sendSyncReq("GET_ID", {iname});
            if (ret.metaData().at(messagebus::Message::STATUS) != messagebus::STATUS_OK) {
                throw std::runtime_error("response status is not OK");
            }

            if (ret.userData().empty()) {
                throw std::runtime_error("response userData is empty");
            }

            cxxtools::SerializationInfo si;
            JSON::readFromString(ret.userData().front(), si);
            std::string data;
            si >>= data;
            return fty::convert<uint32_t>(data);
        }
        catch (const messagebus::MessageBusException &e) {
            return fty::unexpected("assetInameToID request failed: {}", e.what());
        }
        catch (const std::exception &e) {
            return fty::unexpected("assetInameToID failed: {}", e.what());
        }
        return fty::unexpected("assetInameToID failed");
    }

    /// returns the Asset definition, given the internal name
    fty::Expected<fty::Asset> AssetAccessor::getAsset(const std::string& iname)
    {
        try {
            if (iname.empty()) {
                throw std::runtime_error("iname is empty");
            }

            messagebus::Message ret = sendSyncReq("GET", {iname});
            if (ret.metaData().at(messagebus::Message::STATUS) != messagebus::STATUS_OK) {
                throw std::runtime_error("response status is not OK");
            }

            if (ret.userData().empty()) {
                throw std::runtime_error("response userData is empty");
            }

            Asset asset;
            fty::Asset::fromJson(ret.userData().front(), asset);
            return asset;
        }
        catch (const messagebus::MessageBusException &e) {
            return fty::unexpected("getAsset request failed: {}", e.what());
        }
        catch (const std::exception &e) {
            return fty::unexpected("getAsset failed: {}", e.what());
        }
        return fty::unexpected("getAsset failed");
    }

    /// triggers a status update notification
    /// given the iname and status of the asset before and after the update.
    /// assume status in {"active", "inactive"}
    void AssetAccessor::notifyStatusUpdate(const std::string& iname, const std::string& oldStatus, const std::string& newStatus)
    {
        try {
            if (iname.empty()) {
                throw std::runtime_error("iname is empty");
            }
            if (oldStatus.empty() || newStatus.empty()) {
                throw std::runtime_error("oldStatus or newStatus is empty");
            }

            cxxtools::SerializationInfo si;
            si.addMember("iname") <<= iname;
            si.addMember("oldStatus") <<= oldStatus;
            si.addMember("newStatus") <<= newStatus;

            sendAsyncReq("STATUS_UPDATE", {JSON::writeToString(si, false)});
        }
        catch (const messagebus::MessageBusException &e) {
            logError("notifyStatusUpdate request failed: {}", e.what());
        }
        catch (const std::exception &e) {
            logError("notifyStatusUpdate failed: {}", e.what());
        }
    }

    /// triggers an asset update notification
    /// given the DTOs of the asset before and after the update
    void AssetAccessor::notifyAssetUpdate(const Asset& before, const Asset& after)
    {
        try {
            cxxtools::SerializationInfo si;
            si.addMember("before") <<= before;
            si.addMember("after") <<= after;

            sendAsyncReq("NOTIFY", {JSON::writeToString(si, false)});
        }
        catch (const messagebus::MessageBusException &e) {
            logError("notifyAssetUpdate request failed: {}", e.what());
        }
        catch (const std::exception &e) {
            logError("notifyAssetUpdate failed: {}", e.what());
        }
    }
} // namespace fty
