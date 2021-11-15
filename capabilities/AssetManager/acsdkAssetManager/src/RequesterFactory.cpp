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

#include "RequesterFactory.h"

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Power/PowerMonitor.h>

#include "acsdkAssetsCommon/AmdMetricWrapper.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

using namespace std;
using namespace client;
using namespace common;
using namespace commonInterfaces;
using namespace davsInterfaces;
using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;

static const auto s_metrics = AmdMetricsWrapper::creator("requesterFactory");

/// String to identify log entries originating from this file.
static const std::string TAG{"RequesterFactory"};
/// Tag to represent the power resource for URL requesters.
static const std::string URL_POWER_RESOURCE_TAG = "UrlDownloader";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

unique_ptr<RequesterFactory> RequesterFactory::create(
        shared_ptr<StorageManager> storageManager,
        shared_ptr<AmdCommunicationInterface> communicationHandler,
        shared_ptr<ArtifactHandlerInterface> davsClient,
        string urlTmpDirectory,
        shared_ptr<AuthDelegateInterface> authDelegate,
        shared_ptr<UrlAllowListWrapper> allowList) {
    if (storageManager == nullptr) {
        ACSDK_ERROR(LX("create").m("Null Storage Manager"));
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("nullStorageManager"));
        return nullptr;
    }
    if (nullptr == communicationHandler) {
        ACSDK_ERROR(LX("create").m("Null Communication Handler"));
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("nullCommunicationHandler"));
        return nullptr;
    }
    if (davsClient == nullptr) {
        ACSDK_ERROR(LX("create").m("Null Davs Client"));
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("nullDavsClient"));
        return nullptr;
    }
    if (urlTmpDirectory.empty()) {
        ACSDK_ERROR(LX("create").m("Working directory not provided"));
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("invalidWorkingDirectory"));
        return nullptr;
    }
    filesystem::removeAll(urlTmpDirectory);
    if (!filesystem::makeDirectory(urlTmpDirectory)) {
        ACSDK_ERROR(LX("create").m("Could not recreate working directory"));
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("createWorkingDirectory"));
        return nullptr;
    }
    if (authDelegate == nullptr) {
        ACSDK_ERROR(LX("create").m("Empty Auth Delegate"));
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("nullAuthDelegate"));
        return nullptr;
    }
    if (allowList == nullptr) {
        ACSDK_ERROR(LX("create").m("Null Url Allow List Wrapper"));
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("nullUrlAllowList"));
        return nullptr;
    }

    return unique_ptr<RequesterFactory>(new RequesterFactory(
            move(storageManager),
            move(communicationHandler),
            move(davsClient),
            move(urlTmpDirectory),
            authDelegate,
            allowList));
}

RequesterFactory::RequesterFactory(
        std::shared_ptr<StorageManager> storageManager,
        std::shared_ptr<AmdCommunicationInterface> communicationHandler,
        std::shared_ptr<ArtifactHandlerInterface> davsClient,
        std::string urlTmpDirectory,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        std::shared_ptr<UrlAllowListWrapper> allowList) :
        m_storageManager(std::move(storageManager)),
        m_communicationHandler(std::move(communicationHandler)),
        m_davsClient(std::move(davsClient)),
        m_urlTmpDirectory(std::move(urlTmpDirectory)),
        m_authDelegate(move(authDelegate)),
        m_urlPowerResource(power::PowerMonitor::getInstance()->createLocalPowerResource(URL_POWER_RESOURCE_TAG)),
        m_allowedUrlList(move(allowList)) {
}

shared_ptr<Requester> RequesterFactory::createFromStorage(const string& metadataFilePath) {
    shared_ptr<RequesterMetadata> metadata = RequesterMetadata::createFromFile(metadataFilePath);
    if (metadata == nullptr) {
        ACSDK_ERROR(LX("createFromStorage").m("Null Metadata"));
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("nullMetadata"));
        return nullptr;
    }

    auto summary = metadata->getRequest()->getSummary();
    auto requester = createFromMetadata(metadata, metadataFilePath);
    if (requester == nullptr) {
        ACSDK_ERROR(LX("createFromStorage").m("Failed to create requester for stored metadata").d("request", summary));
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("nullRequester"));
        return nullptr;
    }

    if (!requester->initializeFromStorage()) {
        ACSDK_CRITICAL(LX("createFromStorage")
                               .m("This should never happen, failed to acquire resource based on id")
                               .d("request", summary));
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("acquireResource"));
        requester->deleteAndCleanup();
        return nullptr;
    }

    return requester;
}

shared_ptr<Requester> RequesterFactory::createFromMetadata(
        const shared_ptr<RequesterMetadata>& metadata,
        const string& metadataFilePath) {
    if (metadata == nullptr) {
        ACSDK_ERROR(LX("createFromMetadata").m("Null Requester Metadata"));
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("nullRequesterMetadata"));
        return nullptr;
    }
    if (metadataFilePath.empty()) {
        ACSDK_ERROR(LX("createFromMetadata").m("Invalid Metadata File Path"));
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("invalidMetadataFilePath"));
        return nullptr;
    }

    shared_ptr<Requester> requester;
    auto type = metadata->getRequest()->getRequestType();
    if (type == Type::DAVS) {
        requester.reset(
                new DavsRequester(m_storageManager, m_communicationHandler, metadata, metadataFilePath, m_davsClient));
    } else if (type == Type::URL) {
        requester.reset(new UrlRequester(
                m_storageManager,
                m_communicationHandler,
                metadata,
                metadataFilePath,
                m_urlTmpDirectory,
                m_authDelegate,
                m_urlPowerResource,
                m_allowedUrlList));
    } else {
        return nullptr;
    }

    if (!requester->registerCommunicationHandlerPropsLocked()) {
        ACSDK_ERROR(LX("createFromMetadata").m("Failed to register Communication Handler Properties"));
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("communicationHandlerPropsRegisterFailed"));
        return nullptr;
    }

    ACSDK_DEBUG9(LX("createFromMetadata").d("Requester created", requester->name()).d("ID", metadata->getResourceId()));
    return requester;
}

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
