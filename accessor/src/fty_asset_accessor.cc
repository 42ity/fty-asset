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
#include <fty/convert.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#define RECV_TIMEOUT_S 5  // messagebus request timeout (seconds)

namespace fty
{
    static constexpr const char *ASSET_AGENT = "asset-agent-ng";
    static constexpr const char *ASSET_AGENT_QUEUE = "FTY.Q.ASSET.QUERY";
    static constexpr const char *ACCESSOR_NAME = "fty-asset-accessor";
    static constexpr const char *ENDPOINT = MLM_DEFAULT_ENDPOINT;

    /// static helper to send a MessageBus synchronous request
    static messagebus::Message sendSyncReq(const std::string& command, messagebus::UserData data)
    {
        // generate unique ID interface
        std::stringstream ss;
        ss << ACCESSOR_NAME << "-" << std::setfill('0') << std::setw(sizeof(pid_t)*2) << std::hex << std::this_thread::get_id();

        std::string clientName = ss.str();

        std::unique_ptr<messagebus::MessageBus> interface(messagebus::MlmMessageBus(ENDPOINT, clientName));

        interface->connect();

        messagebus::Message msg;

        msg.metaData().emplace(messagebus::Message::CORRELATION_ID, messagebus::generateUuid());
        msg.metaData().emplace(messagebus::Message::SUBJECT, command);
        msg.metaData().emplace(messagebus::Message::FROM, clientName);
        msg.metaData().emplace(messagebus::Message::TO, ASSET_AGENT);
        msg.metaData().emplace(messagebus::Message::REPLY_TO, clientName);

        msg.userData() = data;

        return interface->request(ASSET_AGENT_QUEUE, msg, RECV_TIMEOUT_S);
    }

    /// static helper to send a MessageBus asynch request
    static void sendAsyncReq(const std::string& command, messagebus::UserData data)
    {
        // generate unique ID interface
        std::stringstream ss;
        ss << ACCESSOR_NAME << "-" << std::setfill('0') << std::setw(sizeof(pid_t)*2) << std::hex << std::this_thread::get_id();

        std::string clientName = ss.str();

        std::unique_ptr<messagebus::MessageBus> interface(messagebus::MlmMessageBus(ENDPOINT, clientName));

        interface->connect();

        messagebus::Message msg;

        msg.metaData().emplace(messagebus::Message::CORRELATION_ID, messagebus::generateUuid());
        msg.metaData().emplace(messagebus::Message::SUBJECT, command);
        msg.metaData().emplace(messagebus::Message::FROM, clientName);
        msg.metaData().emplace(messagebus::Message::TO, ASSET_AGENT);
        msg.metaData().emplace(messagebus::Message::REPLY_TO, clientName);

        msg.userData() = data;

        return interface->sendRequest(ASSET_AGENT_QUEUE, msg);
    }

    /// returns the asset database ID, given the internal name
    fty::Expected<uint32_t> AssetAccessor::assetInameToID(const std::string &iname)
    {
        messagebus::Message ret;

        try
        {
            ret = sendSyncReq("GET_ID", {iname});
        }
        catch (messagebus::MessageBusException &e)
        {
            return fty::unexpected("MessageBus request failed: {}", e.what());
        }

        if (ret.metaData().at(messagebus::Message::STATUS) != messagebus::STATUS_OK)
        {
            return fty::unexpected("Request of ID from iname failed");
        }

        cxxtools::SerializationInfo si;
        JSON::readFromString(ret.userData().front(), si);

        std::string data;

        si >>= data;

        return fty::convert<uint32_t>(data);
    }

    /// returns the full fty::Asset, given the internal name
    fty::Expected<fty::Asset> AssetAccessor::getAsset(const std::string& iname)
    {
        messagebus::Message ret;

        try
        {
            ret = sendSyncReq("GET", {iname});
        }
        catch (messagebus::MessageBusException &e)
        {
            return fty::unexpected("MessageBus request failed: {}", e.what());
        }

        if (ret.metaData().at(messagebus::Message::STATUS) != messagebus::STATUS_OK)
        {
            return fty::unexpected("Request of fty::FullAsset from iname failed");
        }

        Asset asset;
        fty::Asset::fromJson(ret.userData().front(), asset);

        return asset;
    }

    /// triggers an update notification. It receives the DTOs of the asset before and after the update
    void AssetAccessor::notifyStatusUpdate(const std::string& iname, const std::string& oldStatus, const std::string& newStatus)
    {
        if(!iname.empty() && !oldStatus.empty() && !newStatus.empty()) {
            cxxtools::SerializationInfo si;
            si.addMember("iname") <<= iname;
            si.addMember("oldStatus") <<= oldStatus;
            si.addMember("newStatus") <<= newStatus;

            std::string json = JSON::writeToString(si, false);

            sendAsyncReq("STATUS_UPDATE", {json});
        } else {
            log_error("Invalid data. Update status notification will not be requested");
        }
    }

    /// triggers an update notification. It receives the DTOs of the asset before and after the update
    void AssetAccessor::notifyAssetUpdate(const Asset& oldAsset, const Asset& newAsset)
    {
        cxxtools::SerializationInfo si;
        si.addMember("before") <<= oldAsset;
        si.addMember("after") <<= newAsset;

        std::string json = JSON::writeToString(si, false);

        sendAsyncReq("NOTIFY", {json});
    }
} // namespace fty
