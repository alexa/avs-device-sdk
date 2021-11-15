/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#include "acsdkDavsClient/DavsClient.h"

#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Power/PowerMonitor.h>

#include <chrono>
#include "acsdkAssetsCommon/AmdMetricWrapper.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace davs {

using namespace std;
using namespace chrono;
using namespace common;
using namespace commonInterfaces;
using namespace davsInterfaces;
using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace rapidjson;
using namespace alexaClientSDK::avsCommon::utils::json::jsonUtils;

#if UNIT_TEST == 1
// For tests, because we don't want to wait hours for it to finish...
static constexpr auto BASE_BACKOFF_VALUE_MS = milliseconds(10);
static constexpr auto MAX_BACKOFF_VALUE_MS = seconds(1);
#else
static constexpr auto BASE_BACKOFF_VALUE_MS = milliseconds(500);
static constexpr auto MAX_BACKOFF_VALUE_MS = minutes(60);
#endif

static const auto s_metrics = AmdMetricsWrapper::creator("DavsClient");

static constexpr auto JSON_ARTIFACT_KEY_SYMBOL = "key";
static constexpr auto JSON_ARTIFACT_TYPE_SYMBOL = "type";
static constexpr auto JSON_ARTIFACT_LIST_SYMBOL = "artifactList";
/// String to identify log entries originating from this file.
static const string TAG{"DavsClient"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

shared_ptr<DavsClient> DavsClient::create(
        string workingDirectory,
        shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        shared_ptr<InternetConnectionMonitorInterface> wifiMonitor,
        shared_ptr<DavsEndpointHandlerInterface> davsEndpointHandler,
        shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        seconds forcedUpdateInterval) {
    if (workingDirectory.empty()) {
        ACSDK_CRITICAL(LX("create").m("Working directory is empty"));
        return nullptr;
    }
    if (authDelegate == nullptr) {
        ACSDK_CRITICAL(LX("create").m("Auth Delegate is null"));
        return nullptr;
    }
    if (wifiMonitor == nullptr) {
        ACSDK_CRITICAL(LX("create").m("Wifi Monitor is null"));
        return nullptr;
    }
    if (davsEndpointHandler == nullptr) {
        ACSDK_CRITICAL(LX("create").m("DAVS Endpoint Handler is null"));
        return nullptr;
    }

    filesystem::removeAll(workingDirectory);
    if (!filesystem::makeDirectory(workingDirectory)) {
        ACSDK_CRITICAL(LX("create").m("Failed to create working directory"));
        return nullptr;
    }

    auto client = shared_ptr<DavsClient>(new DavsClient(
            move(workingDirectory),
            move(authDelegate),
            move(davsEndpointHandler),
            move(metricRecorder),
            max(seconds(0), forcedUpdateInterval)));
    wifiMonitor->addInternetConnectionObserver(client);

    return client;
}

DavsClient::DavsClient(
        string workingDirectory,
        shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        shared_ptr<DavsEndpointHandlerInterface> davsEndpointHandler,
        shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        seconds forcedUpdateInterval) :
        m_workingDirectory(move(workingDirectory)),
        m_authDelegate(move(authDelegate)),
        m_davsEndpointHandler(move(davsEndpointHandler)),
        m_metricRecorder(move(metricRecorder)),
        m_forcedUpdateInterval(forcedUpdateInterval),
        m_powerResource(power::PowerMonitor::getInstance()->createLocalPowerResource(TAG)),
        m_isDeviceIdle(false),
        m_isConnected(false) {
    AmdMetricsWrapper::setStaticRecorder(m_metricRecorder);
}

string DavsClient::handleRequest(
        shared_ptr<DavsRequest> artifactRequest,
        shared_ptr<DavsDownloadCallbackInterface> downloadCallback,
        shared_ptr<DavsCheckCallbackInterface> checkCallback,
        bool enableAutoUpdate,
        bool downloadImmediately) {
    auto handler = DavsHandler::create(
            artifactRequest,
            downloadCallback,
            checkCallback,
            m_workingDirectory,
            BASE_BACKOFF_VALUE_MS,
            MAX_BACKOFF_VALUE_MS,
            m_authDelegate,
            m_davsEndpointHandler,
            m_powerResource,
            m_forcedUpdateInterval);
    if (handler == nullptr) {
        ACSDK_ERROR(LX("handleRequest").m("Failed to create a Davs Handler due to invalid parameters"));
        return "";
    }

    auto requestHash = artifactRequest->getSummary();
    ACSDK_INFO(LX("handleRequest").m("Registering artifact").d("artifact", requestHash));
    m_executor.submit([this, handler, requestHash, enableAutoUpdate, downloadImmediately] {
        s_metrics().addCounter("requestAndDownload-userInitiated");

        auto existingHandler = m_handlers.find(requestHash);
        if (existingHandler != m_handlers.end() && existingHandler->second->isRelevant()) {
            // This is a user-initiated, disable throttle for this request
            if (downloadImmediately) {
                existingHandler->second->requestAndDownload(true);
            }
            existingHandler->second->enableUpdate(enableAutoUpdate);
            return;
        }

        handler->setConnectionState(m_isConnected);
        if (downloadImmediately) {
            handler->requestAndDownload(true);
        }
        handler->enableUpdate(enableAutoUpdate);
        m_handlers[requestHash] = handler;
    });

    return requestHash;
}

string DavsClient::registerArtifact(
        shared_ptr<DavsRequest> artifactRequest,
        shared_ptr<DavsDownloadCallbackInterface> downloadCallback,
        shared_ptr<DavsCheckCallbackInterface> checkCallback,
        bool downloadImmediately) {
    return handleRequest(move(artifactRequest), move(downloadCallback), move(checkCallback), true, downloadImmediately);
}

void DavsClient::deregisterArtifact(const string& requestUUID) {
    ACSDK_INFO(LX("deregisterArtifact").m("Deregistering artifact").d("artifact", requestUUID));
    m_executor.submit([this, requestUUID] {
        auto handler = m_handlers.find(requestUUID);
        if (handler == m_handlers.end()) {
            return;
        }
        handler->second->cancel();
        m_handlers.erase(handler);
    });
}

string DavsClient::downloadOnce(
        shared_ptr<DavsRequest> artifactRequest,
        shared_ptr<DavsDownloadCallbackInterface> downloadCallback,
        shared_ptr<DavsCheckCallbackInterface> checkCallback) {
    return handleRequest(move(artifactRequest), move(downloadCallback), move(checkCallback), false, true);
}

void DavsClient::enableAutoUpdate(const string& requestUUID, bool enable) {
    m_executor.submit([this, requestUUID, enable] {
        auto handler = m_handlers.find(requestUUID);
        if (handler != m_handlers.end() && handler->second->isRelevant()) {
            handler->second->enableUpdate(enable);
        }
    });
}

bool DavsClient::getIdleState() const {
    return m_isDeviceIdle;
}

void DavsClient::setIdleState(const bool idleState) {
    m_executor.submit([this, idleState] {
        m_isDeviceIdle = idleState;

        for (const auto& pair : m_handlers) {
            if (pair.second->isRelevant()) {
                pair.second->setThrottled(!m_isDeviceIdle);
            }
        }
    });
}
void DavsClient::onConnectionStatusChanged(bool connected) {
    m_executor.submit([this, connected] {
        m_isConnected = connected;

        for (const auto& pair : m_handlers) {
            if (pair.second->isRelevant()) {
                pair.second->setConnectionState(m_isConnected);
            }
        }
    });
}
vector<DavsClient::ArtifactGroup> DavsClient::executeParseArtifactGroupFromJson(const string& jsonArtifactList) {
    Document document;
    vector<ArtifactGroup> artifactVector;

    if (!parseJSON(jsonArtifactList, &document)) {
        ACSDK_ERROR(LX("parseArtifactInfoFromJsonNotificationFailed").d("jsonArtifactList", jsonArtifactList));
        return artifactVector;
    }

    vector<std::map<std::string, std::string>> elements;
    if (!retrieveArrayOfStringMapFromArray(document, JSON_ARTIFACT_LIST_SYMBOL, elements)) {
        return artifactVector;
    }

    for (auto element : elements) {
        string typeValue{element[JSON_ARTIFACT_TYPE_SYMBOL]};
        string keyValue{element[JSON_ARTIFACT_KEY_SYMBOL]};
        if (typeValue.empty() || keyValue.empty()) {
            ACSDK_ERROR(LX("parseArtifactInfoFromJsonNotificationFailed").d("reason", "emptyMemberFound"));
            // Clear out everything and return an empty vector as we commit as a whole
            artifactVector.clear();
            return artifactVector;
        }
        artifactVector.emplace_back(ArtifactGroup{typeValue, keyValue});
    }
    return artifactVector;
}
void DavsClient::checkAndUpdateArtifactGroupFromJson(const string& jsonArtifactList) {
    m_executor.submit([this, jsonArtifactList] {
        auto artifactVector = executeParseArtifactGroupFromJson(jsonArtifactList);
        checkAndUpdateArtifactGroupVector(artifactVector);
    });
}
void DavsClient::checkAndUpdateArtifactGroupVector(const std::vector<DavsClient::ArtifactGroup>& artifactVector) {
    for (auto& it : artifactVector) {
        executeUpdateRegisteredArtifact(it);
    }
}
void DavsClient::executeUpdateRegisteredArtifact(const ArtifactGroup& artifactGroup) {
    bool artifactUpdated = false;
    for (const auto& pair : m_handlers) {
        auto& currentHandler = pair.second;
        if (currentHandler->isUpdateEnabled() && currentHandler->isRelevant() &&
            currentHandler->getDavsRequest()->getType() == artifactGroup.type &&
            currentHandler->getDavsRequest()->getKey() == artifactGroup.key) {
            currentHandler->requestAndDownload(false);
            artifactUpdated = true;
        }
    }

    // If nothing updated, there should be a notification
    if (!artifactUpdated) {
        ACSDK_INFO(LX("executeUpdateRegisteredArtifact")
                           .m("Could not find anything to update artifact group")
                           .d("type", artifactGroup.type)
                           .d("key", artifactGroup.key));
    }
}
}  // namespace davs
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
