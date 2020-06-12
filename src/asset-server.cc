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

#include "asset-server.h"
#include "asset/conversion/json.h"
#include "include/fty_asset_dto.h"
#include <cxxtools/jsondeserializer.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include <fty_common_messagebus.h>
#include <functional>
#include <malamute.h>
#include <mlm_client.h>
#include <sstream>

using namespace std::placeholders;

// REMOVE as soon as old interface is not needed anymore
// fwd declaration
void send_create_or_update_asset(
    const fty::AssetServer& config, const std::string& asset_name, const char* operation, bool read_only);

namespace fty {

// ===========================================================================================================

template <typename T>
struct identify
{
    using type = T;
};

template <typename K, typename V>
static V value(const std::map<K, V>& map, const typename identify<K>::type& key,
    const typename identify<V>::type& def = {})
{
    auto it = map.find(key);
    if (it != map.end()) {
        return it->second;
    }
    return def;
}

// ===========================================================================================================

void AssetServer::destroyMlmClient(mlm_client_t* client)
{
    mlm_client_destroy(&client);
}


AssetServer::AssetServer()
    : m_maxActivePowerDevices(-1)
    , m_globalConfigurability(1)
    , m_mailboxClient(mlm_client_new(), &destroyMlmClient)
    , m_streamClient(mlm_client_new(), &destroyMlmClient)
{
}

void AssetServer::createMailboxClientNg()
{
    m_assetMsgQueue.reset(messagebus::MlmMessageBus(m_mailboxEndpoint, m_agentNameNg));
    log_debug("New mailbox client registered to endpoint %s with name %s", m_mailboxEndpoint.c_str(),
        m_agentNameNg.c_str());
}

void AssetServer::resetMailboxClientNg()
{
    log_debug("Mailbox client %s deleted", m_agentNameNg.c_str());
    m_assetMsgQueue.reset();
}

void AssetServer::connectMailboxClientNg()
{
    m_assetMsgQueue->connect();
}

void AssetServer::receiveMailboxClientNg(const std::string& query)
{
    m_assetMsgQueue->receive(query, [&](messagebus::Message m) {
        this->handleAssetManipulationReq(m);
    });
}

void AssetServer::createPublisherClientNg()
{
    m_publisherCreate.reset(messagebus::MlmMessageBus(m_mailboxEndpoint, m_agentNameNg + "-create"));
    log_debug("New publisher client registered to endpoint %s with name %s", m_mailboxEndpoint.c_str(),
        (m_agentNameNg + "-create").c_str());

    m_publisherUpdate.reset(messagebus::MlmMessageBus(m_mailboxEndpoint, m_agentNameNg + "-update"));
    log_debug("New publisher client registered to endpoint %s with name %s", m_mailboxEndpoint.c_str(),
        (m_agentNameNg + "-update").c_str());

    m_publisherDelete.reset(messagebus::MlmMessageBus(m_mailboxEndpoint, m_agentNameNg + "-delete"));
    log_debug("New publisher client registered to endpoint %s with name %s", m_mailboxEndpoint.c_str(),
        (m_agentNameNg + "-delete").c_str());
}

void AssetServer::resetPublisherClientNg()
{
    m_publisherCreate.reset();
    m_publisherUpdate.reset();
    m_publisherDelete.reset();
}

void AssetServer::connectPublisherClientNg()
{
    m_publisherCreate->connect();
    m_publisherUpdate->connect();
    m_publisherDelete->connect();
}

// create response (data is a single string)
messagebus::Message createMessage(const std::string& subject, const std::string& correlationID,
    const std::string& from, const std::string& to, const std::string& status, const std::string& data)
{
    messagebus::Message msg;

    if (!subject.empty()) {
        msg.metaData().emplace(messagebus::Message::SUBJECT, subject);
    }
    if (!from.empty()) {
        msg.metaData().emplace(messagebus::Message::FROM, from);
    }
    if (!to.empty()) {
        msg.metaData().emplace(messagebus::Message::TO, to);
    }
    if (!correlationID.empty()) {
        msg.metaData().emplace(messagebus::Message::CORRELATION_ID, correlationID);
    }
    if (!status.empty()) {
        msg.metaData().emplace(messagebus::Message::STATUS, status);
    }

    msg.userData().push_back(data);

    return msg;
}

// create response (data is a vector of strings)
messagebus::Message createMessage(const std::string& subject, const std::string& correlationID,
    const std::string& from, const std::string& to, const std::string& status,
    const std::vector<std::string>& data)
{
    messagebus::Message msg;

    if (!subject.empty()) {
        msg.metaData().emplace(messagebus::Message::SUBJECT, subject);
    }
    if (!from.empty()) {
        msg.metaData().emplace(messagebus::Message::FROM, from);
    }
    if (!to.empty()) {
        msg.metaData().emplace(messagebus::Message::TO, to);
    }
    if (!correlationID.empty()) {
        msg.metaData().emplace(messagebus::Message::CORRELATION_ID, correlationID);
    }
    if (!status.empty()) {
        msg.metaData().emplace(messagebus::Message::STATUS, status);
    }

    for (const auto& e : data) {
        msg.userData().push_back(e);
    }

    return msg;
}

// new generation asset manipulation handler
void AssetServer::handleAssetManipulationReq(const messagebus::Message& msg)
{
    // asset manipulation is disabled
    if (getGlobalConfigurability() == 0) {
        throw std::runtime_error("Licensing limitation hit - asset manipulation is prohibited.");
    }

    // clang-format off
    static std::map<std::string, std::function<void(const messagebus::Message&)>> procMap = {
        { FTY_ASSET_SUBJECT_CREATE,      [&](const messagebus::Message& msg){ createAsset(msg); } },
        { FTY_ASSET_SUBJECT_UPDATE,      [&](const messagebus::Message& msg){ updateAsset(msg); } },
        { FTY_ASSET_SUBJECT_DELETE,      [&](const messagebus::Message& msg){ deleteAsset(msg); } },
        { FTY_ASSET_SUBJECT_DELETE_LIST, [&](const messagebus::Message& msg){ deleteAssetList(msg); } },
        { FTY_ASSET_SUBJECT_GET,         [&](const messagebus::Message& msg){ getAsset(msg); } },
        { FTY_ASSET_SUBJECT_LIST,        [&](const messagebus::Message& msg){ listAsset(msg); } },
    };
    // clang-format on

    const std::string& messageSubject = value(msg.metaData(), messagebus::Message::SUBJECT);

    if (procMap.find(messageSubject) != procMap.end()) {
        procMap[messageSubject](msg);
    } else {
        log_warning("Handle asset manipulation - Unknown subject");
    }
}

// static void sendResponse(std::unique_ptr<messagebus::MessageBus>& msgBus, const messagebus::Message& msg,
//     const dto::UserData& userData)
// {
//     try {
//         messagebus::Message resp;
//         resp.userData() = userData;
//         resp.metaData().emplace(
//             messagebus::Message::SUBJECT, msg.metaData().find(messagebus::Message::SUBJECT)->second);
//         resp.metaData().emplace(messagebus::Message::FROM, FTY_ASSET_SRR_NAME);
//         resp.metaData().emplace(
//             messagebus::Message::TO, msg.metaData().find(messagebus::Message::FROM)->second);
//         resp.metaData().emplace(messagebus::Message::CORRELATION_ID,
//             msg.metaData().find(messagebus::Message::CORRELATION_ID)->second);
//         msgBus->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, resp);
//     } catch (messagebus::MessageBusException& ex) {
//         log_error("Message bus error: %s", ex.what());
//     } catch (const std::exception& ex) {
//         log_error("Unexpected error: %s", ex.what());
//     }
// }

void AssetServer::handleAssetSrrReq(const messagebus::Message& msg)
{
    log_debug("Handle SRR request");

    const std::string& subject = msg.metaData().at(messagebus::Message::SUBJECT);

    if (subject == FTY_ASSET_SRR_SUBJECT_BACKUP) {
        log_debug("BACKUP");
        cxxtools::SerializationInfo si = saveAssets();
    } else if (subject == FTY_ASSET_SRR_SUBJECT_RESTORE) {
        log_debug("RESTORE");
        cxxtools::SerializationInfo si = saveAssets();
        restoreAssets(si);
    } else if (subject == FTY_ASSET_SRR_SUBJECT_RESET) {
        log_debug("RESET");
        AssetImpl::deleteAll();
    } else if (subject == "TEST") {
        log_debug("TEST");

        cxxtools::SerializationInfo si = saveAssets();

        std::ostringstream       output;
        cxxtools::JsonSerializer serializer(output);
        serializer.serialize(si);

        std::string json = output.str();

        auto response = createMessage(FTY_ASSET_SRR_SUBJECT_BACKUP,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_srrAgentName,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_OK, json);

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_srrClient->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);

        messagebus::Message notification =
            createMessage(FTY_ASSET_SRR_SUBJECT_BACKUP, "", m_srrAgentName, "", messagebus::STATUS_OK, json);

        m_srrClient->publish("BACKUP", notification);

        AssetImpl::deleteAll();
        restoreAssets(si);

    } else {
        log_error("Unkwnown subject %s", subject.c_str());
    }


    // using namespace dto;
    // using namespace dto::srr;

    // try {
    //     messagebus::UserData response;

    //     // Get request
    //     UserData data = msg.userData();
    //     Query    query;
    //     data >> query;

    //     response << (m_srrProcessor.processQuery(query));

    //     // Send response
    //     sendResponse(m_srrClient, msg, response);
    // } catch (std::exception& e) {
    //     log_error("Unexpected error: %s", e.what());
    // } catch (...) {
    //     log_error("Unexpected error: unknown");
    // }
}

dto::srr::SaveResponse AssetServer::handleSave(const dto::srr::SaveQuery& query)
{
    using namespace dto;
    using namespace dto::srr;

    log_debug("Saving assets");
    std::map<FeatureName, FeatureAndStatus> mapFeaturesData;

    return (createSaveResponse(mapFeaturesData, ACTIVE_VERSION)).save();
}
dto::srr::RestoreResponse AssetServer::handleRestore(const dto::srr::RestoreQuery& query)
{
    using namespace dto;
    using namespace dto::srr;

    log_debug("Restoring assets");
    std::map<FeatureName, FeatureStatus> mapStatus;

    return (createRestoreResponse(mapStatus)).restore();
}

// sends create/update/delete notification on both new and old interface
void AssetServer::sendNotification(const messagebus::Message& msg) const
{
    const std::string& subject = msg.metaData().at(messagebus::Message::SUBJECT);

    if (subject == FTY_ASSET_SUBJECT_CREATED) {
        m_publisherCreate->publish(FTY_ASSET_TOPIC_CREATED, msg);

        // REMOVE as soon as old interface is not needed anymore
        // old interface
        fty::Asset asset;
        fty::conversion::fromJson(msg.userData().back(), asset);
        send_create_or_update_asset(
            *this, asset.getInternalName(), "create", false /* read_only is not used */);
    } else if (subject == FTY_ASSET_SUBJECT_UPDATED) {
        m_publisherUpdate->publish(FTY_ASSET_TOPIC_UPDATED, msg);

        // REMOVE as soon as old interface is not needed anymore
        // old interface
        fty::Asset asset;
        fty::conversion::fromJson(
            msg.userData().back(), asset); // old interface replies only with updated asset
        send_create_or_update_asset(
            *this, asset.getInternalName(), "update", false /* read_only is not used */);
    } else if (subject == FTY_ASSET_SUBJECT_DELETED) {
        m_publisherDelete->publish(FTY_ASSET_TOPIC_DELETED, msg);
    }
}

void AssetServer::initSrr(const std::string& query)
{
    m_srrClient.reset(messagebus::MlmMessageBus(m_srrEndpoint, m_srrAgentName));
    log_debug("New publisher client registered to endpoint %s with name %s", m_srrEndpoint.c_str(),
        (m_srrAgentName).c_str());

    m_srrClient->connect();

    m_srrProcessor.saveHandler    = std::bind(&AssetServer::handleSave, this, _1);
    m_srrProcessor.restoreHandler = std::bind(&AssetServer::handleRestore, this, _1);

    m_srrClient->receive(query, [&](messagebus::Message m) {
        this->handleAssetSrrReq(m);
    });
}

void AssetServer::resetSrrClient()
{
    m_srrClient.reset();
}

void AssetServer::createAsset(const messagebus::Message& msg)
{
    log_debug("subject CREATE");

    bool tryActivate = value(msg.metaData(), METADATA_TRY_ACTIVATE) == "true";

    try {
        std::string    userData = msg.userData().front();
        fty::AssetImpl asset;
        fty::conversion::fromJson(userData, asset);

        bool requestActivation = (asset.getAssetStatus() == AssetStatus::Active);

        if (requestActivation && !asset.isActivable()) {
            if (tryActivate) {
                asset.setAssetStatus(fty::AssetStatus::Nonactive);
                requestActivation = false;
            } else {
                throw std::runtime_error(
                    "Licensing limitation hit - maximum amount of active power devices allowed in "
                    "license reached.");
            }
        }

        // store asset to db
        asset.save();
        // activate asset
        if (requestActivation) {
            try {
                asset.activate();
            } catch (std::exception& e) {
                // if activation fails, delete asset
                asset.remove(false);
                throw std::runtime_error(e.what());
            }
        }

        auto response = createMessage(FTY_ASSET_SUBJECT_CREATE,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_OK,
            fty::conversion::toJson(asset));

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);

        // send notification
        messagebus::Message notification = createMessage(FTY_ASSET_SUBJECT_CREATED, "", m_agentNameNg, "",
            messagebus::STATUS_OK, fty::conversion::toJson(asset));
        sendNotification(notification);
    } catch (std::exception& e) {
        log_error(e.what());
        // create response (error)
        auto response = createMessage(FTY_ASSET_SUBJECT_CREATE,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_KO,
            TRANSLATE_ME(e.what()));

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    }
}

void AssetServer::updateAsset(const messagebus::Message& msg)
{
    log_debug("subject UPDATE");

    bool tryActivate = value(msg.metaData(), METADATA_TRY_ACTIVATE) == "true";

    try {
        std::string    userData = msg.userData().front();
        fty::AssetImpl asset;

        fty::conversion::fromJson(userData, asset);

        // get current asset data
        fty::AssetImpl currentAsset(asset.getInternalName());
        // force ID of asset to update
        asset.setId(currentAsset.getId());

        // data vector (contains asset before and after update)
        std::vector<std::string> assetJsonVector;
        // before update
        assetJsonVector.push_back(fty::conversion::toJson(currentAsset));

        bool requestActivation = (currentAsset.getAssetStatus() == fty::AssetStatus::Nonactive &&
                                  asset.getAssetStatus() == fty::AssetStatus::Active);

        // if status changes from nonactive to active, request activation
        if (requestActivation && !asset.isActivable()) {
            if (tryActivate) {
                asset.setAssetStatus(fty::AssetStatus::Nonactive);
                requestActivation = false;
            } else {
                throw std::runtime_error(
                    "Licensing limitation hit - maximum amount of active power devices allowed in "
                    "license reached.");
            }
        }

        // store asset to db
        asset.save();
        // activate asset
        if (requestActivation) {
            try {
                asset.activate();
            } catch (std::exception& e) {
                // if activation fails, set status to nonactive
                asset.setAssetStatus(AssetStatus::Nonactive);
                asset.save();
                throw std::runtime_error(e.what());
            }
        }

        // after update
        assetJsonVector.push_back(fty::conversion::toJson(asset));

        // create response (ok)
        auto response = createMessage(FTY_ASSET_SUBJECT_UPDATE,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_OK,
            fty::conversion::toJson(asset));

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);

        // send notification
        messagebus::Message notification = createMessage(
            FTY_ASSET_SUBJECT_UPDATED, "", m_agentNameNg, "", messagebus::STATUS_OK, assetJsonVector);
        sendNotification(notification);
    } catch (const std::exception& e) {
        log_error(e.what());
        // create response (error)
        auto response = createMessage(FTY_ASSET_SUBJECT_UPDATE,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_KO,
            TRANSLATE_ME(e.what()));

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    }
}

void AssetServer::deleteAsset(const messagebus::Message& msg)
{
    log_debug("subject DELETE");

    messagebus::Message response;
    try {

        std::string    internalName = msg.userData().front();
        fty::AssetImpl asset(internalName);

        asset.remove(value(msg.metaData(), "RECURSIVE") == "YES");

        response = createMessage(value(msg.metaData(), messagebus::Message::SUBJECT),
            value(msg.metaData(), messagebus::Message::CORRELATION_ID), m_agentNameNg,
            value(msg.metaData(), messagebus::Message::FROM), messagebus::STATUS_OK, "");

        // send notification
        messagebus::Message notification = createMessage(FTY_ASSET_SUBJECT_DELETED, "", m_agentNameNg, "",
            messagebus::STATUS_OK, fty::conversion::toJson(asset));
        sendNotification(notification);
    } catch (const std::exception& e) {
        response = createMessage(value(msg.metaData(), messagebus::Message::SUBJECT),
            value(msg.metaData(), messagebus::Message::CORRELATION_ID), m_agentNameNg,
            value(msg.metaData(), messagebus::Message::FROM), messagebus::STATUS_KO, e.what());
    }

    m_assetMsgQueue->sendReply(value(msg.metaData(), messagebus::Message::REPLY_TO), response);
}

void AssetServer::deleteAssetList(const messagebus::Message& msg)
{
    log_debug("subject DELETE_LIST");

    std::istringstream input(msg.userData().front());

    cxxtools::SerializationInfo si;
    cxxtools::JsonDeserializer  deserializer(input);

    deserializer.deserialize(si);

    std::vector<std::string> assetInames;
    for (const auto& el : si.getMember("id")) {
        std::string elId;
        el.getValue(elId);
        assetInames.push_back(elId);
    }

    AssetImpl::deleteList(assetInames);
}

cxxtools::SerializationInfo AssetServer::saveAssets()
{
    using namespace fty::conversion;

    std::vector<std::string> assets = AssetImpl::list();

    cxxtools::SerializationInfo si;

    si.addMember("version") <<= ACTIVE_VERSION;

    cxxtools::SerializationInfo& data = si.addMember("data");

    for (const std::string assetName : assets) {
        AssetImpl a(assetName);

        log_debug("Saving asset %s...", a.getInternalName().c_str());

        cxxtools::SerializationInfo& siAsset = data.addMember("");
        siAsset <<= a;
    }

    data.setCategory(cxxtools::SerializationInfo::Array);

    return si;
}

static void printAssetTreeRec(const std::string& iname, int level)
{
    for (int i = 0; i < level; i++) {
        std::cout << "\t";
    }
    std::cout << iname << std::endl;

    AssetImpl a(iname);
    for (const auto& child : getChildren(a)) {
        printAssetTreeRec(child, level + 1);
    }
}

static void buildRestoreTree(
    std::vector<AssetImpl>& dest, const std::vector<AssetImpl> src, const AssetImpl& node)
{
    dest.push_back(node);
    std::vector<std::string> children = getChildren(node);
    for (const auto& c : children) {
        auto child = std::find_if(src.begin(), src.end(), [&](const AssetImpl& a) {
            return a.getInternalName() == c;
        });
        if (child == src.end()) {
            throw std::runtime_error("Cannot find asset " + child->getInternalName() + " to restore");
        }

        buildRestoreTree(dest, src, *child);
    }
}

void AssetServer::restoreAssets(const cxxtools::SerializationInfo& si)
{
    using namespace fty::conversion;

    // if database is not empty, can't load assets
    if (AssetImpl::list().size() != 0) {
        throw std::runtime_error("Database already contains assets, impossible to restore from SRR");
    }

    std::string srrVersion;
    si.getMember("version") >>= srrVersion;

    if (srrVersion != ACTIVE_VERSION) {
        throw std::runtime_error("Version " + srrVersion + " is not supported");
    }

    const cxxtools::SerializationInfo& assets = si.getMember("data");

    std::vector<AssetImpl> roots;
    std::vector<AssetImpl> list;
    std::vector<AssetImpl> assetsToRestore;

    for (auto it = assets.begin(); it != assets.end(); ++it) {
        AssetImpl a;
        *it >>= a;

        if (a.getParentIname() == "") {
            roots.push_back(a);
        }
        list.push_back(a);
    }

    // build restore tree
    for (const auto& r : roots) {
        buildRestoreTree(assetsToRestore, list, r);
    }

    // TODO move to message header
    bool tryActivate = true;

    std::map<std::string, std::string> assetInames;

    for (AssetImpl& a : assetsToRestore) {
        log_debug("Restoring asset %s...", a.getInternalName().c_str());

        try {
            bool requestActivation = (a.getAssetStatus() == AssetStatus::Active);

            if (requestActivation && !a.isActivable()) {
                if (tryActivate) {
                    a.setAssetStatus(fty::AssetStatus::Nonactive);
                    requestActivation = false;
                } else {
                    throw std::runtime_error(
                        "Licensing limitation hit - maximum amount of active power devices allowed in "
                        "license reached.");
                }
            }
            // update parent iname with new one
            if (a.getParentIname() != "") {
                a.setParentIname(assetInames[a.getParentIname()]);
            }
            // store previous iname
            std::string oldIname = a.getInternalName();

            // store asset to db
            a.save(false);

            // save new iname
            assetInames[oldIname] = a.getInternalName();

            // activate asset
            if (requestActivation) {
                try {
                    a.activate();
                } catch (std::exception& e) {
                    // if activation fails, delete asset
                    a.remove(false);
                    throw std::runtime_error(e.what());
                }
            }
        } catch (std::exception& e) {
            log_error(e.what());
        }
    }

    // restore links
    for (AssetImpl& a : assetsToRestore) {
        try {
            auto links = a.getLinkedAssets();
            for (const auto& l : links) {
                a.linkTo(assetInames[l]);
            }
        } catch (std::exception& e) {
            log_error(e.what());
        }
    }
}

void AssetServer::getAsset(const messagebus::Message& msg)
{
    log_debug("subject GET");

    try {
        std::string    assetIname = msg.userData().front();
        fty::AssetImpl asset(assetIname);

        // create response (ok)
        auto response = createMessage(FTY_ASSET_SUBJECT_GET,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_OK,
            fty::conversion::toJson(asset));

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    } catch (std::exception& e) {
        log_error(e.what());
        // create response (error)
        auto response = createMessage(FTY_ASSET_SUBJECT_GET,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_KO,
            TRANSLATE_ME(e.what()));

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    }
}

void AssetServer::listAsset(const messagebus::Message& msg)
{
    log_debug("subject LIST");

    try {
        std::vector<std::string> assetList = fty::AssetImpl::list();

        // create response (ok)
        auto response = createMessage(FTY_ASSET_SUBJECT_LIST,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_OK, assetList);

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    } catch (std::exception& e) {
        log_error(e.what());
        // create response (error)
        auto response = createMessage(FTY_ASSET_SUBJECT_LIST,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_KO,
            TRANSLATE_ME(e.what()));

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    }
}


} // namespace fty
