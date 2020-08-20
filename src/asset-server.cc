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
#include "asset/asset-utils.h"
#include "include/asset/conversion/json.h"
#include "include/fty_asset_dto.h"
#include <algorithm>
#include <cxxtools/serializationinfo.h>
#include <fty_common_messagebus.h>
#include <functional>
#include <malamute.h>
#include <mlm_client.h>
#include <sstream>
#include <stdlib.h>
#include <time.h>

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

// generate asset name
static std::string createAssetName(const std::string& type, const std::string& subtype)
{
    std::string assetName;

    timeval t;
    gettimeofday(&t, NULL);
    srand(t.tv_sec * t.tv_usec);
    // generate 8 digit random integer
    unsigned long index = rand() % 100000000;

    std::string indexStr = std::to_string(index);

    // create 8 digit index with leading zeros
    indexStr = std::string(8 - indexStr.length(), '0') + indexStr;

    if (type == fty::TYPE_DEVICE) {
        assetName = subtype + "-" + indexStr;
    } else {
        assetName = type + "-" + indexStr;
    }

    return assetName;
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

void AssetServer::receiveMailboxClientNg(const std::string& queue)
{
    m_assetMsgQueue->receive(queue, [&](messagebus::Message m) {
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


    m_publisherCreateLight.reset(
        messagebus::MlmMessageBus(m_mailboxEndpoint, m_agentNameNg + "-create-light"));
    log_debug("New publisher client registered to endpoint %s with name %s", m_mailboxEndpoint.c_str(),
        (m_agentNameNg + "-create-light").c_str());

    m_publisherUpdateLight.reset(
        messagebus::MlmMessageBus(m_mailboxEndpoint, m_agentNameNg + "-update-light"));
    log_debug("New publisher client registered to endpoint %s with name %s", m_mailboxEndpoint.c_str(),
        (m_agentNameNg + "-update-light").c_str());

    m_publisherDeleteLight.reset(
        messagebus::MlmMessageBus(m_mailboxEndpoint, m_agentNameNg + "-delete-light"));
    log_debug("New publisher client registered to endpoint %s with name %s", m_mailboxEndpoint.c_str(),
        (m_agentNameNg + "-delete-light").c_str());
}

void AssetServer::resetPublisherClientNg()
{
    m_publisherCreate.reset();
    m_publisherUpdate.reset();
    m_publisherDelete.reset();
    m_publisherCreateLight.reset();
    m_publisherUpdateLight.reset();
    m_publisherDeleteLight.reset();
}

void AssetServer::connectPublisherClientNg()
{
    m_publisherCreate->connect();
    m_publisherUpdate->connect();
    m_publisherDelete->connect();
    m_publisherCreateLight->connect();
    m_publisherUpdateLight->connect();
    m_publisherDeleteLight->connect();
}

// new generation asset manipulation handler
void AssetServer::handleAssetManipulationReq(const messagebus::Message& msg)
{
    log_debug("Received new asset manipulation message with subject %s",
        msg.metaData().at(messagebus::Message::SUBJECT).c_str());

    // clang-format off
    static std::map<std::string, std::function<void(const messagebus::Message&)>> procMap = {
        { FTY_ASSET_SUBJECT_CREATE,       [&](const messagebus::Message& msg){ createAsset(msg); } },
        { FTY_ASSET_SUBJECT_UPDATE,       [&](const messagebus::Message& msg){ updateAsset(msg); } },
        { FTY_ASSET_SUBJECT_DELETE,       [&](const messagebus::Message& msg){ deleteAsset(msg); } },
        { FTY_ASSET_SUBJECT_GET,          [&](const messagebus::Message& msg){ getAsset(msg); } },
        { FTY_ASSET_SUBJECT_GET_BY_UUID,  [&](const messagebus::Message& msg){ getAsset(msg, true); } },
        { FTY_ASSET_SUBJECT_LIST,         [&](const messagebus::Message& msg){ listAsset(msg); } },
    };
    // clang-format on

    const std::string& messageSubject = value(msg.metaData(), messagebus::Message::SUBJECT);

    if (procMap.find(messageSubject) != procMap.end()) {
        procMap[messageSubject](msg);
    } else {
        log_warning("Handle asset manipulation - Unknown subject");
    }
}

void AssetServer::handleAssetSrrReq(const messagebus::Message& msg)
{
    log_debug("Process SRR request");

    using namespace dto;
    using namespace dto::srr;

    try {
        // Get request
        UserData data = msg.userData();
        Query    query;
        data >> query;

        messagebus::UserData respData;
        respData << (m_srrProcessor.processQuery(query));

        auto response = assetutils::createMessage(msg.metaData().find(messagebus::Message::SUBJECT)->second,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_srrAgentName,
            msg.metaData().find(messagebus::Message::REPLY_TO)->second, messagebus::STATUS_OK, respData);

        log_debug(
            "Sending response to: %s", msg.metaData().find(messagebus::Message::REPLY_TO)->second.c_str());
        m_srrClient->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);

    } catch (std::exception& e) {
        log_error("Unexpected error: %s", e.what());
    } catch (...) {
        log_error("Unexpected error: unknown");
    }
}

dto::srr::SaveResponse AssetServer::handleSave(const dto::srr::SaveQuery& query)
{
    using namespace dto;
    using namespace dto::srr;

    log_debug("Saving assets");
    std::map<FeatureName, FeatureAndStatus> mapFeaturesData;

    for (const auto& featureName : query.features()) {
        FeatureAndStatus fs1;
        Feature&         f1 = *(fs1.mutable_feature());


        if (featureName == FTY_ASSET_SRR_NAME) {
            f1.set_version(SRR_ACTIVE_VERSION);
            try {
                std::unique_lock<std::mutex> lock(m_srrLock);
                f1.set_data(assetutils::serialize(saveAssets()));
                fs1.mutable_status()->set_status(Status::SUCCESS);
            } catch (std::exception& e) {
                fs1.mutable_status()->set_status(Status::FAILED);
                fs1.mutable_status()->set_error(e.what());
            }

        } else {
            fs1.mutable_status()->set_status(Status::FAILED);
            fs1.mutable_status()->set_error("Feature is not supported!");
        }

        mapFeaturesData[featureName] = fs1;
    }

    return (createSaveResponse(mapFeaturesData, SRR_ACTIVE_VERSION)).save();
}

dto::srr::RestoreResponse AssetServer::handleRestore(const dto::srr::RestoreQuery& query)
{
    using namespace dto;
    using namespace dto::srr;

    log_debug("Restoring assets");
    std::map<FeatureName, FeatureStatus> mapStatus;

    for (const auto& item : query.map_features_data()) {
        const FeatureName& featureName = item.first;
        const Feature&     feature     = item.second;

        FeatureStatus featureStatus;
        // backup current assets
        cxxtools::SerializationInfo assetBackup = saveAssets();

        if (featureName == FTY_ASSET_SRR_NAME) {
            try {
                std::unique_lock<std::mutex> lock(m_srrLock);

                cxxtools::SerializationInfo si = assetutils::deserialize(feature.data());
                log_debug("Si=\n%s", feature.data().c_str());
                // clear database
                AssetImpl::deleteAll();
                restoreAssets(si);

                featureStatus.set_status(Status::SUCCESS);
            } catch (std::exception& e) {
                // if restore fails, recover previous status
                AssetImpl::deleteAll();
                restoreAssets(assetBackup);

                featureStatus.set_status(Status::FAILED);
                featureStatus.set_error(e.what());
            }

        } else {
            featureStatus.set_status(Status::FAILED);
            featureStatus.set_error("Feature is not supported!");
        }

        mapStatus[featureName] = featureStatus;
    }

    return (createRestoreResponse(mapStatus)).restore();
}

dto::srr::ResetResponse AssetServer::handleReset(const dto::srr::ResetQuery& query)
{
    using namespace dto;
    using namespace dto::srr;

    log_debug("Reset assets");
    std::map<FeatureName, FeatureStatus> mapStatus;

    const FeatureName& featureName = FTY_ASSET_SRR_NAME;
    FeatureStatus      featureStatus;
    featureStatus.set_status(Status::FAILED);
    featureStatus.set_error("Feature is not supported!");

    mapStatus[featureName] = featureStatus;

    return (createResetResponse(mapStatus)).reset();
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
        cxxtools::SerializationInfo        si    = assetutils::deserialize(msg.userData().front());
        const cxxtools::SerializationInfo& after = si.getMember("after");

        fty::Asset asset;
        // old interface replies only with updated asset
        using conversion::operator>>=;
        after >>= asset;

        send_create_or_update_asset(
            *this, asset.getInternalName(), "update", false /* read_only is not used */);
    } else if (subject == FTY_ASSET_SUBJECT_DELETED) {
        m_publisherDelete->publish(FTY_ASSET_TOPIC_DELETED, msg);
    } else if (subject == FTY_ASSET_SUBJECT_CREATED_L) {
        m_publisherCreateLight->publish(FTY_ASSET_TOPIC_CREATED_L, msg);
    } else if (subject == FTY_ASSET_SUBJECT_UPDATED_L) {
        m_publisherUpdateLight->publish(FTY_ASSET_TOPIC_UPDATED_L, msg);
    } else if (subject == FTY_ASSET_SUBJECT_DELETED_L) {
        m_publisherDeleteLight->publish(FTY_ASSET_TOPIC_DELETED_L, msg);
    }
}

void AssetServer::initSrr(const std::string& queue)
{
    m_srrClient.reset(messagebus::MlmMessageBus(m_srrEndpoint, m_srrAgentName));
    log_debug("New publisher client registered to endpoint %s with name %s", m_srrEndpoint.c_str(),
        (m_srrAgentName).c_str());

    m_srrClient->connect();

    m_srrProcessor.saveHandler    = std::bind(&AssetServer::handleSave, this, _1);
    m_srrProcessor.restoreHandler = std::bind(&AssetServer::handleRestore, this, _1);
    m_srrProcessor.resetHandler   = std::bind(&AssetServer::handleReset, this, _1);

    m_srrClient->receive(queue, [&](messagebus::Message m) {
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
        // asset manipulation is disabled
        if (getGlobalConfigurability() == 0) {
            throw std::runtime_error("Licensing limitation hit - asset manipulation is prohibited");
        }

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

        // internal name provided in the JSON payload is discarded
        // set internal name (<type/subtype>-<random id>)
        asset.setInternalName(createAssetName(asset.getAssetType(), asset.getAssetSubtype()));
        // store asset to db
        asset.create();
        // activate asset
        if (requestActivation) {
            try {
                asset.activate();
            } catch (std::exception& e) {
                // if activation fails, delete asset
                AssetImpl::deleteList({asset.getInternalName()}, false);
                throw std::runtime_error(e.what());
            }
        }

        // update asset data
        asset.load();

        auto response = assetutils::createMessage(FTY_ASSET_SUBJECT_CREATE,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_OK,
            fty::conversion::toJson(asset));

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);

        // full notification
        messagebus::Message notification = assetutils::createMessage(FTY_ASSET_SUBJECT_CREATED, "",
            m_agentNameNg, "", messagebus::STATUS_OK, fty::conversion::toJson(asset));
        sendNotification(notification);
        // light notification
        messagebus::Message notification_l = assetutils::createMessage(FTY_ASSET_SUBJECT_CREATED_L, "",
            m_agentNameNg, "", messagebus::STATUS_OK, asset.getInternalName());
        sendNotification(notification_l);
    } catch (std::exception& e) {
        log_error(e.what());
        // create response (error)
        auto response = assetutils::createMessage(FTY_ASSET_SUBJECT_CREATE,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_KO,
            "An error occurred while creating asset. " + std::string(e.what()));

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
        // asset manipulation is disabled
        if (getGlobalConfigurability() == 0) {
            throw std::runtime_error("Licensing limitation hit - asset manipulation is prohibited");
        }

        std::string    userData = msg.userData().front();
        fty::AssetImpl asset;

        fty::conversion::fromJson(userData, asset);

        // get current asset data from storage
        fty::AssetImpl currentAsset(asset.getInternalName());

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
        asset.update();
        // activate asset
        if (requestActivation) {
            try {
                asset.activate();
            } catch (std::exception& e) {
                // if activation fails, set status to nonactive
                asset.setAssetStatus(AssetStatus::Nonactive);
                asset.update();
                throw std::runtime_error(e.what());
            }
        }

        // update data from db
        asset.load();

        // build message
        // serialization info which contains asset before and after update
        cxxtools::SerializationInfo si;

        // before update
        cxxtools::SerializationInfo tmpSi;
        using conversion::          operator<<=;

        tmpSi <<= currentAsset;

        cxxtools::SerializationInfo& before = si.addMember("");
        before.setCategory(cxxtools::SerializationInfo::Category::Object);
        before = tmpSi;
        before.setName("before");

        // after update
        tmpSi.clear();
        tmpSi <<= asset;

        cxxtools::SerializationInfo& after = si.addMember("");
        after.setCategory(cxxtools::SerializationInfo::Category::Object);
        after = tmpSi;
        after.setName("after");

        // create response (ok)
        auto response = assetutils::createMessage(FTY_ASSET_SUBJECT_UPDATE,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_OK,
            fty::conversion::toJson(asset));

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);

        // full notification
        messagebus::Message notification = assetutils::createMessage(FTY_ASSET_SUBJECT_UPDATED, "",
            m_agentNameNg, "", messagebus::STATUS_OK, assetutils::serialize(si));
        sendNotification(notification);

        // light notification
        messagebus::Message notification_l = assetutils::createMessage(FTY_ASSET_SUBJECT_UPDATED_L, "",
            m_agentNameNg, "", messagebus::STATUS_OK, asset.getInternalName());
        sendNotification(notification_l);
    } catch (const std::exception& e) {
        log_error(e.what());
        // create response (error)
        auto response = assetutils::createMessage(FTY_ASSET_SUBJECT_UPDATE,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_KO,
            "An error occurred while updating asset. " + std::string(e.what()));

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    }
}

static std::string serializeDeleteStatus(AssetImpl::DeleteStatus statusList)
{
    cxxtools::SerializationInfo si;

    for (const auto& s : statusList) {
        cxxtools::SerializationInfo& status = si.addMember("");
        status.setCategory(cxxtools::SerializationInfo::Category::Object);
        status.addMember(s.first.getInternalName()) <<= s.second;
    }

    si.setCategory(cxxtools::SerializationInfo::Category::Array);

    return assetutils::serialize(si);
}

void AssetServer::deleteAsset(const messagebus::Message& msg)
{
    log_debug("subject DELETE");

    cxxtools::SerializationInfo si = assetutils::deserialize(msg.userData().front());

    messagebus::Message response;
    try {
        // asset manipulation is disabled
        if (getGlobalConfigurability() == 0) {
            throw std::runtime_error("Licensing limitation hit - asset manipulation is prohibited");
        }

        std::vector<std::string> assetInames;
        si >>= assetInames;

        AssetImpl::DeleteStatus deleted =
            AssetImpl::deleteList(assetInames, value(msg.metaData(), "RECURSIVE") == "YES");

        // send response
        response = assetutils::createMessage(value(msg.metaData(), messagebus::Message::SUBJECT),
            value(msg.metaData(), messagebus::Message::CORRELATION_ID), m_agentNameNg,
            value(msg.metaData(), messagebus::Message::FROM), messagebus::STATUS_OK,
            serializeDeleteStatus(deleted));

        // send one notification for each asset deleted
        for (const auto& status : deleted) {
            if (status.second == "OK") {
                // full notification
                messagebus::Message notification = assetutils::createMessage(FTY_ASSET_SUBJECT_DELETED, "",
                    m_agentNameNg, "", messagebus::STATUS_OK, fty::conversion::toJson(status.first));
                sendNotification(notification);

                // light notification
                messagebus::Message notification_l = assetutils::createMessage(FTY_ASSET_SUBJECT_DELETED_L,
                    "", m_agentNameNg, "", messagebus::STATUS_OK, status.first.getInternalName());
                sendNotification(notification_l);
            }
        }
    } catch (const std::exception& e) {
        response = assetutils::createMessage(value(msg.metaData(), messagebus::Message::SUBJECT),
            value(msg.metaData(), messagebus::Message::CORRELATION_ID), m_agentNameNg,
            value(msg.metaData(), messagebus::Message::FROM), messagebus::STATUS_KO, e.what());
    }

    m_assetMsgQueue->sendReply(value(msg.metaData(), messagebus::Message::REPLY_TO), response);
}

cxxtools::SerializationInfo AssetServer::saveAssets()
{
    using namespace fty::conversion;

    std::vector<std::string> assets = AssetImpl::list();

    cxxtools::SerializationInfo si;

    si.addMember("version") <<= SRR_ACTIVE_VERSION;

    cxxtools::SerializationInfo& data = si.addMember("data");

    for (const std::string assetName : assets) {
        AssetImpl a(assetName);

        log_debug("Saving asset %s...", a.getInternalName().c_str());

        cxxtools::SerializationInfo& siAsset = data.addMember("");
        AssetImpl::assetToSrr(a, siAsset);
    }

    data.setCategory(cxxtools::SerializationInfo::Array);

    return si;
}

static void buildRestoreTree(std::vector<AssetImpl>& v)
{
    std::map<std::string, std::vector<std::string>> ancestorMatrix;

    for (const AssetImpl& a : v) {
        // create node if doesn't exist
        ancestorMatrix[a.getInternalName()];
        if (!a.getParentIname().empty()) {
            ancestorMatrix[a.getParentIname()].push_back(a.getInternalName());
        }
    }

    // iterate on each asset
    for (const auto& a : ancestorMatrix) {
        std::string id       = a.first;
        const auto& children = a.second;

        // find ancestors of asset a
        for (auto& x : ancestorMatrix) {
            auto& parentOf = x.second;

            auto found = std::find(parentOf.begin(), parentOf.end(), id);
            // if x is parent of a, x inherits all children of a
            if (found != parentOf.end()) {
                for (const auto& child : children) {
                    parentOf.push_back(child);
                }
            }
        }
    }

    std::sort(v.begin(), v.end(), [&](const AssetImpl& l, const AssetImpl& r) {
        return ancestorMatrix[l.getInternalName()].size() > ancestorMatrix[r.getInternalName()].size();
    });
}

void AssetServer::restoreAssets(const cxxtools::SerializationInfo& si, bool tryActivate)
{
    using namespace fty::conversion;

    // if database is not empty, can't load assets
    if (AssetImpl::list().size() != 0) {
        throw std::runtime_error("Database already contains assets, impossible to restore from SRR");
    }

    std::string srrVersion;
    si.getMember("version") >>= srrVersion;

    if (srrVersion != SRR_ACTIVE_VERSION) {
        throw std::runtime_error("Version " + srrVersion + " is not supported");
    }

    const cxxtools::SerializationInfo& assets = si.getMember("data");
    std::vector<AssetImpl>             assetsToRestore;

    for (auto it = assets.begin(); it != assets.end(); ++it) {
        AssetImpl a;
        AssetImpl::srrToAsset(*it, a);

        assetsToRestore.push_back(a);
    }

    buildRestoreTree(assetsToRestore);

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

            // restore asset to db
            a.restore();

            // activate asset
            if (requestActivation) {
                try {
                    a.activate();
                } catch (std::exception& e) {
                    // if activation fails, delete asset
                    AssetImpl::deleteList({a.getInternalName()}, false);
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
                a.linkTo(l.sourceId, l.srcOut, l.destIn, l.linkType);
            }
        } catch (std::exception& e) {
            log_error(e.what());
        }
    }
}

void AssetServer::getAsset(const messagebus::Message& msg, bool getFromUuid)
{
    log_debug("subject GET%s", (getFromUuid ? "_FROM_UUID" : ""));

    try {
        std::string assetID = msg.userData().front();
        if (getFromUuid) {
            assetID = AssetImpl::getInameFromUuid(assetID);
            if (assetID.empty()) {
                throw std::runtime_error("requested UUID does not match any asset");
            }
        }

        fty::AssetImpl asset(assetID);

        // create response (ok)
        auto response = assetutils::createMessage(FTY_ASSET_SUBJECT_GET,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_OK,
            fty::conversion::toJson(asset));

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    } catch (std::exception& e) {
        log_error(e.what());
        // create response (error)
        auto response = assetutils::createMessage(FTY_ASSET_SUBJECT_GET,
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

        cxxtools::SerializationInfo si;
        si <<= assetList;

        // create response (ok)
        auto response = assetutils::createMessage(FTY_ASSET_SUBJECT_LIST,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_OK,
            assetutils::serialize(si));

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    } catch (std::exception& e) {
        log_error(e.what());
        // create response (error)
        auto response = assetutils::createMessage(FTY_ASSET_SUBJECT_LIST,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_KO,
            std::string(e.what()));

        // send response
        log_debug("sending response to %s", msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    }
}


} // namespace fty
