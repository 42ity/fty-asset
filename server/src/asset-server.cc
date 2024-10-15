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
#include "asset/asset-helpers.h"
#include "utilspp.h"
#include "fty-lock.h"

#include <fty_asset_dto.h>
#include <fty/convert.h>
#include <fty_common.h>
#include <fty_common_messagebus.h>

#include <malamute.h>

#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <list>

#include <cxxtools/serializationinfo.h>

using namespace std::placeholders;

// REMOVE as soon as old interface is not needed anymore
// fwd declaration (fty_asset_server.cc)
void send_create_or_update_asset(const fty::AssetServer& config, const std::string& asset_name, const char* operation);

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
    if (client) {
        mlm_client_t* c = client;
        mlm_client_destroy(&c);
    }
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
    const std::string endpoint{m_mailboxEndpoint};

    m_assetMsgQueue.reset(messagebus::MlmMessageBus(endpoint, m_agentNameNg));
    log_debug("New mailbox client registered to endpoint %s with name %s", endpoint.c_str(),
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
    log_debug("createPublisherClientNg");

    const std::string endpoint{m_streamEndpoint};

    m_publisherCreate.reset(messagebus::MlmMessageBus(endpoint, m_agentNameNg + "-create"));
    log_debug("New publisher client registered to endpoint %s with name %s", endpoint.c_str(),
        (m_agentNameNg + "-create").c_str());

    m_publisherUpdate.reset(messagebus::MlmMessageBus(endpoint, m_agentNameNg + "-update"));
    log_debug("New publisher client registered to endpoint %s with name %s", endpoint.c_str(),
        (m_agentNameNg + "-update").c_str());

    m_publisherDelete.reset(messagebus::MlmMessageBus(endpoint, m_agentNameNg + "-delete"));
    log_debug("New publisher client registered to endpoint %s with name %s", endpoint.c_str(),
        (m_agentNameNg + "-delete").c_str());

    m_publisherCreateLight.reset(messagebus::MlmMessageBus(endpoint, m_agentNameNg + "-create-light"));
    log_debug("New publisher client registered to endpoint %s with name %s", endpoint.c_str(),
        (m_agentNameNg + "-create-light").c_str());

    m_publisherUpdateLight.reset(messagebus::MlmMessageBus(endpoint, m_agentNameNg + "-update-light"));
    log_debug("New publisher client registered to endpoint %s with name %s", endpoint.c_str(),
        (m_agentNameNg + "-update-light").c_str());

    m_publisherDeleteLight.reset(messagebus::MlmMessageBus(endpoint, m_agentNameNg + "-delete-light"));
    log_debug("New publisher client registered to endpoint %s with name %s", endpoint.c_str(),
        (m_agentNameNg + "-delete-light").c_str());
}

void AssetServer::resetPublisherClientNg()
{
    log_debug("resetPublisherClientNg");

    m_publisherCreate.reset();
    m_publisherUpdate.reset();
    m_publisherDelete.reset();
    m_publisherCreateLight.reset();
    m_publisherUpdateLight.reset();
    m_publisherDeleteLight.reset();
}

void AssetServer::connectPublisherClientNg()
{
    log_debug("connectPublisherClientNg");

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
    static std::map<std::string, std::function<void(const messagebus::Message&)>> procMap = {
        { FTY_ASSET_SUBJECT_CREATE,       [&](const messagebus::Message& m){ createAsset(m); } },
        { FTY_ASSET_SUBJECT_UPDATE,       [&](const messagebus::Message& m){ updateAsset(m); } },
        { FTY_ASSET_SUBJECT_DELETE,       [&](const messagebus::Message& m){ deleteAsset(m); } },
        { FTY_ASSET_SUBJECT_GET,          [&](const messagebus::Message& m){ getAsset(m); } },
        { FTY_ASSET_SUBJECT_GET_BY_UUID,  [&](const messagebus::Message& m){ getAsset(m); } },
        { FTY_ASSET_SUBJECT_LIST,         [&](const messagebus::Message& m){ listAsset(m); } },
        { FTY_ASSET_SUBJECT_GET_ID,       [&](const messagebus::Message& m){ getAssetID(m); } },
        { FTY_ASSET_SUBJECT_GET_INAME,    [&](const messagebus::Message& m){ getAssetIname(m); } },
        { FTY_ASSET_SUBJECT_STATUS_UPD,   [&](const messagebus::Message& m){ notifyStatusUpdate(m); } },
        { FTY_ASSET_SUBJECT_NOTIFY,       [&](const messagebus::Message& m){ notifyAsset(m); } }
    };

    const std::string& subject = value(msg.metaData(), messagebus::Message::SUBJECT);

    log_debug("Received new asset manipulation message with subject %s", subject.c_str());

    if (procMap.find(subject) != procMap.end()) {
        procMap[subject](msg);
    }
    else {
        log_warning("Handle asset manipulation - Unknown subject (%s)", subject.c_str());
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
        Query query;
        data >> query;

        messagebus::UserData respData;
        respData << (m_srrProcessor.processQuery(query));

        auto response = assetutils::createMessage(
            msg.metaData().find(messagebus::Message::SUBJECT)->second,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_srrAgentName,
            msg.metaData().find(messagebus::Message::REPLY_TO)->second,
            messagebus::STATUS_OK,
            respData);

        log_debug("Sending response to: %s", msg.metaData().find(messagebus::Message::REPLY_TO)->second.c_str());
        m_srrClient->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);

    }
    catch (const std::exception& e) {
        log_error("Unexpected error: %s", e.what());
    }
    catch (...) {
        log_error("Unexpected error: unknown");
    }
}

dto::srr::SaveResponse AssetServer::handleSave(const dto::srr::SaveQuery& query)
{
    log_debug("SRR Saving assets");

    using namespace dto;
    using namespace dto::srr;

    std::map<FeatureName, FeatureAndStatus> mapFeaturesData;

    for (const auto& featureName : query.features()) {
        FeatureAndStatus fs1;
        Feature&         f1 = *(fs1.mutable_feature());

        if (featureName == FTY_ASSET_SRR_NAME) {
            f1.set_version(SRR_ACTIVE_VERSION);
            try {
                Lock lock(m_srrLock);
                auto savedAssets = saveAssets();
                f1.set_data(JSON::writeToString(savedAssets, false));
                fs1.mutable_status()->set_status(Status::SUCCESS);
            }
            catch (const std::exception& e) {
                fs1.mutable_status()->set_status(Status::FAILED);
                fs1.mutable_status()->set_error(e.what());
            }

        }
        else {
            fs1.mutable_status()->set_status(Status::FAILED);
            fs1.mutable_status()->set_error("Feature is not supported!");
        }

        mapFeaturesData[featureName] = fs1;
    }

    return (createSaveResponse(mapFeaturesData, SRR_ACTIVE_VERSION)).save();
}

dto::srr::RestoreResponse AssetServer::handleRestore(const dto::srr::RestoreQuery& query)
{
    log_debug("SRR Restoring assets");

    using namespace dto;
    using namespace dto::srr;

    std::map<FeatureName, FeatureStatus> mapStatus;

    for (const auto& item : query.map_features_data()) {
        const FeatureName& featureName = item.first;
        const Feature&     feature     = item.second;

        FeatureStatus featureStatus;
        if (featureName == FTY_ASSET_SRR_NAME) {

            //NOT USED backup current assets
            //cxxtools::SerializationInfo assetBackup = saveAssets();

            try {
                Lock lock(m_srrLock);

                cxxtools::SerializationInfo si;
                JSON::readFromString(feature.data(), si);
                restoreAssets(si);

                featureStatus.set_status(Status::SUCCESS);
            }
            catch (const std::exception& e) {

                featureStatus.set_status(Status::FAILED);
                featureStatus.set_error(e.what());
            }
        }
        else {
            featureStatus.set_status(Status::FAILED);
            featureStatus.set_error("Feature is not supported!");
        }

        mapStatus[featureName] = featureStatus;
    }

    return (createRestoreResponse(mapStatus)).restore();
}

dto::srr::ResetResponse AssetServer::handleReset(const dto::srr::ResetQuery& /*query*/)
{
    log_debug("SRR Reset assets");

    using namespace dto;
    using namespace dto::srr;

    std::map<FeatureName, FeatureStatus> mapStatus;
    const FeatureName& featureName = FTY_ASSET_SRR_NAME;

    FeatureStatus featureStatus;
    try {
        AssetImpl::deleteAll(false);
        featureStatus.set_status(Status::SUCCESS);
    }
    catch (const std::exception& e) {
        featureStatus.set_status(Status::FAILED);
        featureStatus.set_error("Reset of assets failed");
    }

    mapStatus[featureName] = featureStatus;

    return (createResetResponse(mapStatus)).reset();
}

// sendNotification simple (subject, data) interface
void AssetServer::sendNotification(const std::string& subject, const std::string& data) const
{
    sendNotification(assetutils::createMessage(
        subject,
        "", // correlation id
        m_agentNameNg,
        "", // to
        messagebus::STATUS_OK,
        data)
    );
}

// sends create/update/delete notification on both new and old interface
void AssetServer::sendNotification(const messagebus::Message& msg) const
{
    const std::string& subject = msg.metaData().at(messagebus::Message::SUBJECT);

    if (subject == FTY_ASSET_SUBJECT_UPDATED) {
        m_publisherUpdate->publish(FTY_ASSET_TOPIC_UPDATED, msg);

        // REMOVE as soon as old interface is not needed anymore
        {
            // old interface replies only with updated asset (after)
            cxxtools::SerializationInfo si;
            JSON::readFromString(msg.userData().front(), si);
            const cxxtools::SerializationInfo& after = si.getMember("after");
            fty::Asset asset;
            after >>= asset;

            send_create_or_update_asset(*this, asset.getInternalName(), "update");
        }
    }
    else if (subject == FTY_ASSET_SUBJECT_UPDATED_L) {
        m_publisherUpdateLight->publish(FTY_ASSET_TOPIC_UPDATED_L, msg);
    }
    else if (subject == FTY_ASSET_SUBJECT_CREATED) {
        m_publisherCreate->publish(FTY_ASSET_TOPIC_CREATED, msg);

        // REMOVE as soon as old interface is not needed anymore
        {
            // old interface
            fty::Asset asset;
            fty::Asset::fromJson(msg.userData().front(), asset);

            send_create_or_update_asset(*this, asset.getInternalName(), "create");
        }
    }
    else if (subject == FTY_ASSET_SUBJECT_CREATED_L) {
        m_publisherCreateLight->publish(FTY_ASSET_TOPIC_CREATED_L, msg);
    }
    else if (subject == FTY_ASSET_SUBJECT_DELETED) {
        m_publisherDelete->publish(FTY_ASSET_TOPIC_DELETED, msg);
    }
    else if (subject == FTY_ASSET_SUBJECT_DELETED_L) {
        m_publisherDeleteLight->publish(FTY_ASSET_TOPIC_DELETED_L, msg);
    }
    else {
        log_debug("sendNotification() subject not handled (%s)", subject.c_str());
    }
}

void AssetServer::initSrr(const std::string& queue)
{
    std::string endpoint{m_srrEndpoint};

    log_debug("New publisher client registered to endpoint %s with name %s",
        endpoint.c_str(), (m_srrAgentName).c_str());

    m_srrClient.reset(messagebus::MlmMessageBus(endpoint, m_srrAgentName));

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

void AssetServer::createAsset(const messagebus::Message& msg) const
{
    const std::string subject{msg.metaData().find(messagebus::Message::SUBJECT)->second};
    log_debug("subject %s", subject.c_str());

    messagebus::Message response;

    bool tryActivate = value(msg.metaData(), METADATA_TRY_ACTIVATE) == "true";

    try {
        // asset manipulation is disabled
        if (getGlobalConfigurability() == 0) {
            throw std::runtime_error("Licensing limitation hit - asset manipulation is prohibited");
        }

        fty::AssetImpl asset;
        fty::Asset::fromJson(msg.userData().front(), asset);

        auto ipAddr = asset.getExtEntry("ip.1");
        asset::AssetFilter filter(asset.getManufacturer(), asset.getModel(), asset.getSerialNo(), ipAddr);
        auto ret = asset::checkDuplicatedAsset(filter);
        if (!ret) {
            throw std::runtime_error("Asset already exists");
        }

        bool requestActivation = (asset.getAssetStatus() == AssetStatus::Active);

        if (requestActivation && !asset.isActivable()) {
            if (tryActivate) {
                asset.setAssetStatus(fty::AssetStatus::Nonactive);
                requestActivation = false;
            }
            else {
                throw std::runtime_error("Licensing limitation hit - maximum amount of active power devices allowed in license reached.");
            }
        }

        // store asset to db
        asset.create();

        // activate asset
        if (requestActivation) {
            try {
                asset.activate();
            }
            catch (const std::exception& e) {
                // if activation fails, delete asset
                AssetImpl::deleteList({asset.getInternalName()}, false);
                throw std::runtime_error(e.what());
            }
        }

        // update asset data
        asset.load();

        // create response (ok)
        response = assetutils::createMessage(
            subject,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second,
            messagebus::STATUS_OK,
            fty::Asset::toJson(asset));

        // full notification
        sendNotification(FTY_ASSET_SUBJECT_CREATED, response.userData().front() /*toJson(asset)*/);
        // light notification
        sendNotification(FTY_ASSET_SUBJECT_CREATED_L, asset.getInternalName());
    }
    catch (const std::exception& e) {
        log_error("Unexpected error: '%s'", e.what());

        // create response (error)
        response = assetutils::createMessage(
            subject,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second,
            messagebus::STATUS_KO,
            TRANSLATE_ME("An error occurred creating asset (%s)", e.what()));
    }

    // send response
    const std::string to{msg.metaData().find(messagebus::Message::REPLY_TO)->second};
    log_debug("sending response to '%s'", to.c_str());
    m_assetMsgQueue->sendReply(to, response);
}

void AssetServer::updateAsset(const messagebus::Message& msg) const
{
    const std::string subject{msg.metaData().find(messagebus::Message::SUBJECT)->second};
    log_debug("subject %s", subject.c_str());

    messagebus::Message response;

    bool tryActivate = value(msg.metaData(), METADATA_TRY_ACTIVATE) == "true";

    try {
        // asset manipulation is disabled
        if (getGlobalConfigurability() == 0) {
            throw std::runtime_error("Licensing limitation hit - asset manipulation is prohibited");
        }

        fty::AssetImpl asset;
        fty::Asset::fromJson(msg.userData().front(), asset);

        // get current asset data from storage
        fty::AssetImpl currentAsset(asset.getInternalName());

        bool requestActivation = (currentAsset.getAssetStatus() == fty::AssetStatus::Nonactive
                                  && asset.getAssetStatus() == fty::AssetStatus::Active);
        bool requestDeactivation = (currentAsset.getAssetStatus() == fty::AssetStatus::Active
                                  && asset.getAssetStatus() == fty::AssetStatus::Nonactive);

        // if status changes from nonactive to active, request activation
        if (requestActivation && !asset.isActivable()) {
            if (tryActivate) {
                asset.setAssetStatus(fty::AssetStatus::Nonactive);
                requestActivation = false;
            }
            else {
                throw std::runtime_error(
                    "Licensing limitation hit - maximum amount of active power devices allowed in "
                    "license reached.");
            }
        }

        // store asset to db
        asset.update();

        if (requestActivation) {
            // activate asset
            try {
                asset.activate();
            }
            catch (const std::exception& e) {
                // if activation fails, set status to nonactive
                asset.setAssetStatus(AssetStatus::Nonactive);
                asset.update();
                throw std::runtime_error(e.what());
            }
        }
        else if(requestDeactivation) {
            // deactivate asset
            asset.deactivate();
        }

        // update asset from db
        asset.load();

        // create response (ok)
        response = assetutils::createMessage(
            subject,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second,
            messagebus::STATUS_OK,
            fty::Asset::toJson(asset));

        notifyAssetUpdate(currentAsset, asset);
    }
    catch (const std::exception& e) {
        log_error("Unexpected error: '%s'", e.what());

        // create response (error)
        response = assetutils::createMessage(
            subject,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second,
            messagebus::STATUS_KO,
            TRANSLATE_ME("An error occurred updating asset (%s)", e.what()));
    }

    // send response
    const std::string to{msg.metaData().find(messagebus::Message::REPLY_TO)->second};
    log_debug("sending response to '%s'", to.c_str());
    m_assetMsgQueue->sendReply(to, response);
}

static std::string serializeDeleteStatus(DeleteStatus statusList)
{
    cxxtools::SerializationInfo si;

    for (const auto& s : statusList) {
        cxxtools::SerializationInfo& status = si.addMember("");
        status.setCategory(cxxtools::SerializationInfo::Category::Object);
        status.addMember(s.first.getInternalName()) <<= s.second;
    }

    si.setCategory(cxxtools::SerializationInfo::Category::Array);

    return JSON::writeToString(si, false);
}

void AssetServer::deleteAsset(const messagebus::Message& msg) const
{
    const std::string subject{msg.metaData().find(messagebus::Message::SUBJECT)->second};
    log_debug("subject %s", subject.c_str());

    messagebus::Message response;

    try {
        cxxtools::SerializationInfo si;
        JSON::readFromString(msg.userData().front(), si);

        // asset manipulation is disabled
        if (getGlobalConfigurability() == 0) {
            throw std::runtime_error("Licensing limitation hit - asset manipulation is prohibited");
        }

        std::vector<std::string> assetInames;
        si >>= assetInames;

        DeleteStatus deleted = AssetImpl::deleteList(assetInames, value(msg.metaData(), "RECURSIVE") == "YES");

        // create response (ok)
        response = assetutils::createMessage(
            subject,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second,
            messagebus::STATUS_OK,
            serializeDeleteStatus(deleted));

        // send one notification for each deleted asset
        for (const auto& status : deleted) {
            if (status.second == "OK") {
                // full notification
                sendNotification(FTY_ASSET_SUBJECT_DELETED, fty::Asset::toJson(status.first));
                 // light notification
                sendNotification(FTY_ASSET_SUBJECT_DELETED_L, status.first.getInternalName());
            }
        }
    }
    catch (const std::exception& e) {
        log_error("Unexpected error: '%s'", e.what());

        // create response (error)
        response = assetutils::createMessage(
            subject,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second,
            messagebus::STATUS_KO,
            TRANSLATE_ME("An error occurred deleting asset (%s)", e.what()));
    }

    // send response
    const std::string to{msg.metaData().find(messagebus::Message::REPLY_TO)->second};
    log_debug("sending response to '%s'", to.c_str());
    m_assetMsgQueue->sendReply(to, response);
}

void AssetServer::getAsset(const messagebus::Message& msg) const
{
    const std::string subject{msg.metaData().find(messagebus::Message::SUBJECT)->second};
    log_debug("subject %s", subject.c_str());

    messagebus::Message response;

    bool getByUuid = (subject == FTY_ASSET_SUBJECT_GET_BY_UUID); // vs. by assetID

    try {
        std::string assetID = msg.userData().front();
        if (getByUuid) {
            assetID = AssetImpl::getInameFromUuid(assetID);
            if (assetID.empty()) {
                throw std::runtime_error("requested UUID does not match any asset");
            }
        }

        fty::AssetImpl asset(assetID);

        if (value(msg.metaData(), METADATA_WITH_PARENTS_LIST) == "true") {
            asset.updateParentsList();
        }

        // create response (ok)
        response = assetutils::createMessage(
            subject,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second,
            messagebus::STATUS_OK,
            fty::Asset::toJson(asset));
    }
    catch (const std::exception& e) {
        log_error("Unexpected error: '%s'", e.what());

        // create response (error)
        response = assetutils::createMessage(
            subject,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second,
            messagebus::STATUS_KO,
            TRANSLATE_ME("An error occurred reading asset (%s)", e.what()));
    }

    // send response
    const std::string to{msg.metaData().find(messagebus::Message::REPLY_TO)->second};
    log_debug("sending response to '%s'", to.c_str());
    m_assetMsgQueue->sendReply(to, response);
}

void AssetServer::listAsset(const messagebus::Message& msg) const
{
    const std::string subject{msg.metaData().find(messagebus::Message::SUBJECT)->second};
    log_debug("subject %s", subject.c_str());

    messagebus::Message response;

    try {
        AssetFilters filters;

        if (!msg.userData().empty()) {
            cxxtools::SerializationInfo siFilters;
            JSON::readFromString(msg.userData().front(), siFilters);

            siFilters >>= filters;

            log_debug("Applied filters:");
            for (const auto& p : filters) {
                std::string line = p.first + " -";
                for (const auto& v : p.second) {
                    line.append(" " + v);
                }
                log_debug("%s", line.c_str());
            }
        }

        bool idOnly = true;
        if (value(msg.metaData(), METADATA_ID_ONLY) == "false") {
            idOnly = false;
        }

        std::vector<std::string>    inameList = fty::AssetImpl::list(filters);
        cxxtools::SerializationInfo si;

        if (idOnly) {
            si <<= inameList;
        }
        else {
            bool withParentsList = value(msg.metaData(), METADATA_WITH_PARENTS_LIST) == "true";

            for (const auto& iname : inameList) {
                try {
                    fty::AssetImpl asset(iname);
                    if (withParentsList) {
                        asset.updateParentsList();
                    }
                    cxxtools::SerializationInfo& data = si.addMember("");
                    data <<= asset;
                    data.setCategory(cxxtools::SerializationInfo::Category::Object);
                }
                catch (const std::exception& e) {
                    log_error("Could not retrieve asset %s: %s", iname.c_str(), e.what());
                }
            }
            si.setCategory(cxxtools::SerializationInfo::Category::Array);
        }

        // create response (ok)
        response = assetutils::createMessage(
            subject,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second,
            messagebus::STATUS_OK,
            JSON::writeToString(si, false));
    }
    catch (const std::exception& e) {
        log_error("Unexpected error: '%s'", e.what());

        // create response (error)
        response = assetutils::createMessage(
            subject,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second,
            messagebus::STATUS_KO,
            TRANSLATE_ME("An error occurred listing assets (%s)", e.what()));
    }

    // send response
    const std::string to{msg.metaData().find(messagebus::Message::REPLY_TO)->second};
    log_debug("sending response to '%s'", to.c_str());
    m_assetMsgQueue->sendReply(to, response);
}

void AssetServer::getAssetID(const messagebus::Message& msg) const
{
    const std::string subject{msg.metaData().find(messagebus::Message::SUBJECT)->second};
    log_debug("subject %s", subject.c_str());

    messagebus::Message response;

    try {
        std::string assetIname = msg.userData().front();
        cxxtools::SerializationInfo si;
        si <<= AssetImpl::getIDFromIname(assetIname);

        // create response (ok)
        response = assetutils::createMessage(
            subject,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second,
            messagebus::STATUS_OK,
            JSON::writeToString(si, false));
    }
    catch (const std::exception& e) {
        log_error("Unexpected error: '%s'", e.what());

        // create response (error)
        response = assetutils::createMessage(
            subject,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second,
            messagebus::STATUS_KO,
            TRANSLATE_ME("An error occurred reading asset (%s)", e.what()));
    }

    // send response
    const std::string to{msg.metaData().find(messagebus::Message::REPLY_TO)->second};
    log_debug("sending response to '%s'", to.c_str());
    m_assetMsgQueue->sendReply(to, response);
}

void AssetServer::getAssetIname(const messagebus::Message& msg) const
{
    const std::string subject{msg.metaData().find(messagebus::Message::SUBJECT)->second};
    log_debug("subject %s", subject.c_str());

    messagebus::Message response;

    try {
        std::string assetID = msg.userData().front();
        cxxtools::SerializationInfo si;
        si <<= AssetImpl::getInameFromID(fty::convert<std::uint32_t>(assetID));

        // create response (ok)
        response = assetutils::createMessage(
            subject,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second,
            messagebus::STATUS_OK,
            JSON::writeToString(si, false));
    }
    catch (const std::exception& e) {
        log_error("Unexpected error: '%s'", e.what());

        // create response (error)
        response = assetutils::createMessage(
            subject,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second,
            m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second,
            messagebus::STATUS_KO,
            TRANSLATE_ME("An error occurred reading asset (%s)", e.what()));
    }

    // send response
    const std::string to{msg.metaData().find(messagebus::Message::REPLY_TO)->second};
    log_debug("sending response to '%s'", to.c_str());
    m_assetMsgQueue->sendReply(to, response);
}

void AssetServer::notifyStatusUpdate(const messagebus::Message& msg) const
{
    const std::string subject{msg.metaData().find(messagebus::Message::SUBJECT)->second};
    log_debug("subject %s", subject.c_str());

    try {
        cxxtools::SerializationInfo si;
        JSON::readFromString(msg.userData().front(), si);

        std::string iname;
        std::string oldStatus;
        std::string newStatus;

        si.getMember("iname") >>= iname;
        si.getMember("oldStatus") >>= oldStatus;
        si.getMember("newStatus") >>= newStatus;

        AssetStatus oldSt = stringToAssetStatus(oldStatus);
        AssetStatus newSt = stringToAssetStatus(newStatus);

        // notify only if status changed & consistent
        if (oldSt != newSt
            && oldSt != AssetStatus::Unknown
            && newSt != AssetStatus::Unknown
        ) {
            log_debug("Notifying status update for asset %s (%s -> %s)",
                iname.c_str(), oldStatus.c_str(), newStatus.c_str());

            AssetImpl after(iname);

            if (after.getAssetStatus() != newSt) {
                throw std::runtime_error("Current asset status does not match requested notification");
            }

            AssetImpl before(after);
            before.setAssetStatus(oldSt);

            notifyAssetUpdate(before, after);
        }
    }
    catch (const std::exception& e) {
        log_error("Unexpected error: '%s'", e.what());
    }

    // no response sent
}

void AssetServer::notifyAsset(const messagebus::Message& msg) const
{
    const std::string subject{msg.metaData().find(messagebus::Message::SUBJECT)->second};
    log_debug("subject %s", subject.c_str());

    try {
        cxxtools::SerializationInfo si;
        JSON::readFromString(msg.userData().front(), si);

        //const auto& oldAssetSi = si.getMember("before");
        //Asset oldAsset;
        //oldAssetSi >>= oldAsset;

        const auto& newAssetSi = si.getMember("after");
        Asset newAsset;
        newAssetSi >>= newAsset;

        log_debug("Sending notification for asset %s", newAsset.getInternalName().c_str());

        // full notification
        sendNotification(FTY_ASSET_SUBJECT_UPDATED, JSON::writeToString(si, false));
        // light notification
        sendNotification(FTY_ASSET_SUBJECT_UPDATED_L, newAsset.getInternalName());
    }
    catch (const std::exception& e) {
        log_error("Unexpected error: '%s'", e.what());
    }

    // no response sent
}

void AssetServer::notifyAssetUpdate(const Asset& before, const Asset& after) const
{
    try {
        cxxtools::SerializationInfo si;
        si.addMember("before") <<= before;
        si.addMember("after") <<= after;

        // full notification
        sendNotification(FTY_ASSET_SUBJECT_UPDATED, JSON::writeToString(si, false));
        // light notification
        sendNotification(FTY_ASSET_SUBJECT_UPDATED_L, after.getInternalName());
    }
    catch (const std::exception& e) {
        log_error("Unexpected error: '%s'", e.what());
    }
}

// SRR
cxxtools::SerializationInfo AssetServer::saveAssets(bool saveVirtualAssets) const
{
    std::vector<std::string> assets = AssetImpl::listAll();

    cxxtools::SerializationInfo si;

    si.addMember("version") <<= SRR_ACTIVE_VERSION;

    cxxtools::SerializationInfo& data = si.addMember("data");

    for (const std::string& assetName : assets) {
        AssetImpl a(assetName);

        if (!saveVirtualAssets && a.isVirtual()) {
            log_debug("Save: skip virtual asset %s (id_secondary: %s)",
                a.getInternalName().c_str(), a.getSecondaryID().c_str());
            continue;
        }

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
    std::string srrVersion;
    si.getMember("version") >>= srrVersion;

    if (fty::convert<float>(srrVersion) > fty::convert<float>(SRR_ACTIVE_VERSION)) {
        throw std::runtime_error("Version " + srrVersion + " is not supported");
    }

    std::vector<AssetImpl> assetsToRestore;

    const cxxtools::SerializationInfo& assets = si.getMember("data");
    for (auto it = assets.begin(); it != assets.end(); ++it) {
        AssetImpl a;
        AssetImpl::srrToAsset(*it, a);

        // IPMPROG-8971/IPMPROG-9035: do not restore virtual assets saved by mistake
        if (a.isVirtual()) {
            log_debug("Restore: skip virtual asset %s (id_secondary: %s)", a.getInternalName().c_str(), a.getSecondaryID().c_str());
            continue;
        }

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
                }
                else {
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
                }
                catch (const std::exception& e) {
                    // if activation fails, delete asset
                    AssetImpl::deleteList({a.getInternalName()}, false);
                    throw std::runtime_error(e.what());
                }
            }
        }
        catch (const std::exception& e) {
            log_error("Unexpected error: '%s'", e.what());
        }
    }

    // restore links
    for (AssetImpl& a : assetsToRestore) {
        try {
            // save links
            a.update();
        }
        catch (const std::exception& e) {
            log_error("Unexpected error: '%s'", e.what());
        }
    }
}

} // namespace fty
