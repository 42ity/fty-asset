/*  =========================================================================
    asset-server - asset-server

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
    =========================================================================
*/

#pragma once
#include "asset/asset.h"
#include <fty_srr_dto.h>
#include <memory>
#include <mutex>

static constexpr const char* FTY_ASSET_MAILBOX = "FTY.Q.ASSET.QUERY";
// new interface mailbox subjects
static constexpr const char* FTY_ASSET_SUBJECT_CREATE      = "CREATE";
static constexpr const char* FTY_ASSET_SUBJECT_UPDATE      = "UPDATE";
static constexpr const char* FTY_ASSET_SUBJECT_DELETE      = "DELETE";
static constexpr const char* FTY_ASSET_SUBJECT_DELETE_LIST = "DELETE_LIST";
static constexpr const char* FTY_ASSET_SUBJECT_GET         = "GET";
static constexpr const char* FTY_ASSET_SUBJECT_LIST        = "LIST";

// new interface topics
static constexpr const char* FTY_ASSET_TOPIC_CREATED = "FTY.T.ASSET.CREATED";
static constexpr const char* FTY_ASSET_TOPIC_UPDATED = "FTY.T.ASSET.UPDATED";
static constexpr const char* FTY_ASSET_TOPIC_DELETED = "FTY.T.ASSET.DELETED";

// new interface topic subjects
static constexpr const char* FTY_ASSET_SUBJECT_CREATED = "CREATED";
static constexpr const char* FTY_ASSET_SUBJECT_UPDATED = "UPDATED";
static constexpr const char* FTY_ASSET_SUBJECT_DELETED = "DELETED";


static constexpr const char* METADATA_TRY_ACTIVATE      = "TRY_ACTIVATE";
static constexpr const char* METADATA_NO_ERROR_IF_EXIST = "NO_ERROR_IF_EXIST";

// SRR
static constexpr const char* ACTIVE_VERSION                = "1.0";
static constexpr const char* FTY_ASSET_SRR_NAME            = "asset-agent-srr";
static constexpr const char* FTY_ASSET_SRR_QUEUE           = "FTY.Q.SRR";
static constexpr const char* FTY_ASSET_SRR_SUBJECT_BACKUP  = "BACKUP";
static constexpr const char* FTY_ASSET_SRR_SUBJECT_RESTORE = "RESTORE";
static constexpr const char* FTY_ASSET_SRR_SUBJECT_RESET   = "RESET";

typedef struct _mlm_client_t mlm_client_t;
namespace messagebus {
class MessageBus;
class Message;
} // namespace messagebus

namespace fty {

class AssetServer
{
public:
    using MsgBusPtr = std::unique_ptr<messagebus::MessageBus>;

    AssetServer();
    ~AssetServer() = default;

    bool getTestMode() const
    {
        return m_testMode;
    }

    const std::string& getAgentName() const
    {
        return m_agentName;
    }

    const std::string& getMailboxEndpoint() const
    {
        return m_mailboxEndpoint;
    }

    const std::string& getStreamEndpoint() const
    {
        return m_streamEndpoint;
    }

    const mlm_client_t* getMailboxClient() const
    {
        return m_mailboxClient.get();
    }

    const mlm_client_t* getStreamClient() const
    {
        return m_streamClient.get();
    }

    int getMaxActivePowerDevices() const
    {
        return m_maxActivePowerDevices;
    }

    int getGlobalConfigurability() const
    {
        return m_globalConfigurability;
    }

    void setTestMode(bool test)
    {
        m_testMode = test;
    }

    void setAgentName(const std::string& name)
    {
        m_agentName = name;
    }

    void setMailboxEndpoint(const std::string& endpoint)
    {
        m_mailboxEndpoint = endpoint;
    }

    void setStreamEndpoint(const std::string& endpoint)
    {
        m_streamEndpoint = endpoint;
    }

    void setMaxActivePowerDevices(int m)
    {
        m_maxActivePowerDevices = m;
    }

    void setGlobalConfigurability(int c)
    {
        m_globalConfigurability = c;
    }

    // new generation interface
    const std::string& getAgentNameNg() const
    {
        return m_agentNameNg;
    }

    void setAgentNameNg(const std::string& name)
    {
        m_agentNameNg = name;
    }

    void setSrrEndpoint(const std::string& endpoint)
    {
        m_srrEndpoint = endpoint;
    }

    void setSrrAgentName(const std::string& agentName)
    {
        m_srrAgentName = agentName;
    }

    void createMailboxClientNg();
    void resetMailboxClientNg();
    void connectMailboxClientNg();
    void receiveMailboxClientNg(const std::string& query);

    void createPublisherClientNg();
    void resetPublisherClientNg();
    void connectPublisherClientNg();

    // notifications
    void sendNotification(const messagebus::Message&) const;

    // SRR
    void initSrr(const std::string& query);
    void resetSrrClient();

private:
    void createAsset(const messagebus::Message& msg);
    void updateAsset(const messagebus::Message& msg);
    void deleteAsset(const messagebus::Message& msg);
    void getAsset(const messagebus::Message& msg);
    void listAsset(const messagebus::Message& msg);
    void deleteAssetList(const messagebus::Message& msg);

    // SRR
    cxxtools::SerializationInfo saveAssets();
    void                        restoreAssets(const cxxtools::SerializationInfo& si);

private:
    static void destroyMlmClient(mlm_client_t* client);

    using MlmClientPtr = std::unique_ptr<mlm_client_t, decltype(&AssetServer::destroyMlmClient)>;


private:
    bool         m_testMode        = false;
    std::string  m_agentName       = "asset-agent";
    std::string  m_mailboxEndpoint = "ipc://@/malamute";
    std::string  m_streamEndpoint  = "ipc://@/malamute";
    int          m_maxActivePowerDevices;
    int          m_globalConfigurability;
    MlmClientPtr m_mailboxClient;
    MlmClientPtr m_streamClient;

    // new generation interface
    std::string m_agentNameNg = "asset-agent-ng";
    MsgBusPtr   m_assetMsgQueue;
    MsgBusPtr   m_publisherCreate;
    MsgBusPtr   m_publisherUpdate;
    MsgBusPtr   m_publisherDelete;

    // topic handlers
    void handleAssetManipulationReq(const messagebus::Message& msg);
    void handleAssetSrrReq(const messagebus::Message& msg);

    // SRR
    std::string                 m_srrEndpoint  = "ipc://@/malamute";
    std::string                 m_srrAgentName = "asset-agent-srr";
    MsgBusPtr                   m_srrClient;
    std::mutex                  m_srrLock;
    dto::srr::SrrQueryProcessor m_srrProcessor;

    // SRR handlers
    dto::srr::SaveResponse    handleSave(const dto::srr::SaveQuery& query);
    dto::srr::RestoreResponse handleRestore(const dto::srr::RestoreQuery& query);
};

messagebus::Message createMessage(const std::string& subject, const std::string& correlationID,
    const std::string& from, const std::string& to, const std::string& status, const std::string& data);
messagebus::Message createMessage(const std::string& subject, const std::string& correlationID,
    const std::string& from, const std::string& to, const std::string& status,
    const std::vector<std::string>& data);

} // namespace fty
