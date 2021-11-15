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

#include "acsdkAssetManager/AssetManager.h"

#include <algorithm>
#include <utility>

#include "RequestFactory.h"
#include "RequesterFactory.h"
#include "StorageManager.h"
#include "acsdkAssetsCommon/AmdMetricWrapper.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

using namespace std;
using namespace chrono;
using namespace davs;
using namespace client;
using namespace common;
using namespace commonInterfaces;
using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::metrics;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;

static const auto s_metrics = AmdMetricsWrapper::creator("assetManager");

/// String to identify log entries originating from this file.
static const std::string LOGGER_TAG{"AssetManager"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(LOGGER_TAG, event)

shared_ptr<AssetManager> AssetManager::create(
        const shared_ptr<AmdCommunicationInterface>& communicationHandler,
        const shared_ptr<DavsClient>& davsClient,
        const string& artifactsDirectory,
        const shared_ptr<AuthDelegateInterface>& authDelegate,
        const shared_ptr<UrlAllowListWrapper>& allowUrlList) {
    if (communicationHandler == nullptr) {
        ACSDK_CRITICAL(LX("create").m("Null Communication Handler"));
        return nullptr;
    }
    if (davsClient == nullptr) {
        ACSDK_CRITICAL(LX("create").m("Null Davs Client"));
        return nullptr;
    }
    if (artifactsDirectory.empty() || artifactsDirectory == "/") {
        ACSDK_CRITICAL(LX("create").m("Invalid artifacts home directory"));
        return nullptr;
    }
    if (!filesystem::makeDirectory(artifactsDirectory)) {
        ACSDK_CRITICAL(
                LX("create").m("Could not create AssetManager's base directory").d("directory", artifactsDirectory));

        return nullptr;
    }
    if (authDelegate == nullptr) {
        ACSDK_CRITICAL(LX("create").m("Null Auth Delegate"));
        return nullptr;
    }
    if (allowUrlList == nullptr) {
        ACSDK_CRITICAL(LX("create").m("Null Url Allow List Wrapper"));
        return nullptr;
    }

    auto assetManager = shared_ptr<AssetManager>(
            new AssetManager(communicationHandler, davsClient, artifactsDirectory, authDelegate, allowUrlList));

    if (!assetManager->init()) {
        ACSDK_CRITICAL(LX("create").m("Failed to initialize AssetManager"));
        return nullptr;
    }

    return assetManager;
}

AssetManager::AssetManager(
        shared_ptr<AmdCommunicationInterface> communicationHandler,
        shared_ptr<DavsClient> davsClient,
        const string& baseDirectory,
        shared_ptr<AuthDelegateInterface> authDelegate,
        shared_ptr<UrlAllowListWrapper> allowList) :
        m_communicationHandler(move(communicationHandler)),
        m_davsClient(move(davsClient)),
        m_resourcesDirectory(baseDirectory + "/resources"),
        m_requestsDirectory(baseDirectory + "/requests"),
        m_urlTmpDirectory(baseDirectory + "/urlWorkingDir"),
        m_authDelegate(move(authDelegate)),
        m_urlAllowList(move(allowList)) {
}

AssetManager::~AssetManager() {
    m_executor.shutdown();
}

bool AssetManager::init() {
    if (!filesystem::makeDirectory(m_resourcesDirectory)) {
        ACSDK_CRITICAL(LX("init").m("Could not make resources directory"));
        return false;
    }
    if (!filesystem::makeDirectory(m_requestsDirectory)) {
        ACSDK_CRITICAL(LX("init").m("Could not make requesters directory"));
        return false;
    }

    m_storageManager = StorageManager::create(m_resourcesDirectory, shared_from_this());
    if (m_storageManager == nullptr) {
        ACSDK_CRITICAL(LX("init").m("Could not create Storage Manager"));
        return false;
    }

    m_requesterFactory = RequesterFactory::create(
            m_storageManager, m_communicationHandler, m_davsClient, m_urlTmpDirectory, m_authDelegate, m_urlAllowList);
    if (m_requesterFactory == nullptr) {
        ACSDK_CRITICAL(LX("init").m("Could not create Requester Factory"));
        return false;
    }

    milliseconds latestTime(0);
    auto metadataFiles = filesystem::list(m_requestsDirectory, filesystem::FileType::REGULAR_FILE);
    for (const auto& metadataFile : metadataFiles) {
        auto metadataFilePath = m_requestsDirectory + "/" + metadataFile;
        auto requester = m_requesterFactory->createFromStorage(metadataFilePath);
        if (requester != nullptr) {
            ACSDK_INFO(LX("init").m("Loaded stored requester").d("requester", requester->name()));
            latestTime = max(latestTime, requester->getLastUsed());
            m_requesters.emplace(move(requester));
        } else {
            ACSDK_ERROR(LX("init").m("Failed to load stored requester, cleaning it up!"));
            filesystem::removeAll(metadataFilePath);
        }
    }

    // be sure to have the storage manager erase any artifacts that got unreferenced.
    m_storageManager->purgeUnreferenced();

    // update the start time offset based on the latest requester that was stored.
    Requester::START_TIME_OFFSET = latestTime;

    m_communicationHandler->registerFunction(AMD::REGISTER_PROP, shared_from_this());
    m_communicationHandler->registerFunction(AMD::REMOVE_PROP, shared_from_this());

    return true;
}

bool AssetManager::downloadArtifact(const shared_ptr<commonInterfaces::ArtifactRequest>& request) {
    if (request == nullptr) {
        ACSDK_ERROR(LX("downloadArtifact").m("Received null request"));
        return false;
    }

    lock_guard<mutex> lock(m_requestersMutex);
    auto requester = findRequesterLocked(request);
    if (requester == nullptr) {
        ACSDK_INFO(LX("downloadArtifact").m("Creating new requester").d("request", request->getSummary()));
        auto metadataFilePath = m_requestsDirectory + "/" + request->getSummary();
        requester = m_requesterFactory->createFromMetadata(RequesterMetadata::create(request), metadataFilePath);

        if (requester == nullptr) {
            ACSDK_ERROR(LX("downloadArtifact").m("Could not create requester").d("request", request->getSummary()));
            return false;
        }

        m_requesters.emplace(requester);
    } else {
        ACSDK_INFO(LX("downloadArtifact").m("Requester already registered").d("requester", requester->name()));
    }
    return requester->download();
}

bool AssetManager::queueDownloadArtifact(const std::string& requestString) {
    auto request = RequestFactory::create(requestString);
    if (request == nullptr) {
        ACSDK_ERROR(LX("queueDownloadArtifact").d("Received invalid request", requestString));
        return false;
    }
    queueDownloadArtifact(request);
    return true;
}
void AssetManager::deleteArtifact(const string& summaryString) {
    ACSDK_INFO(LX("deleteArtifact").m("Deleting requester").d("requester", summaryString));

    lock_guard<mutex> lock(m_requestersMutex);
    for (const auto& requester : m_requesters) {
        if (requester->getArtifactRequest()->getSummary() == summaryString) {
            requester->deleteAndCleanup();
            m_requesters.erase(requester);
            return;
        }
    }

    ACSDK_ERROR(LX("deleteArtifact"));
}

void AssetManager::handleUpdate(const std::string& summaryString, bool acceptUpdate) {
    ACSDK_INFO(LX("handleUpdate")
                       .m("Artifact Update for requester")
                       .d("acceptUpdate", acceptUpdate ? "Applying" : "Rejecting")
                       .d("requester", summaryString));
    lock_guard<mutex> lock(m_requestersMutex);
    for (const auto& requester : m_requesters) {
        if (requester->getArtifactRequest()->getSummary() == summaryString) {
            requester->handleUpdate(acceptUpdate);
            return;
        }
    }

    ACSDK_ERROR(LX("handleUpdate")
                        .m("Could not find a requester to handle update with summary")
                        .d("summary", summaryString));
}

shared_ptr<Requester> AssetManager::findRequesterLocked(const shared_ptr<ArtifactRequest>& request) const {
    if (request == nullptr) {
        return nullptr;
    }

    for (const auto& requester : m_requesters) {
        if (*requester->getArtifactRequest() == *request) {
            return requester;
        }
    }
    return nullptr;
}

bool AssetManager::freeUpSpace(size_t requestedAmount) {
    ACSDK_DEBUG(LX("freeUpSpace").m("Requesting space").d("numberOfBytes", requestedAmount));
    if (requestedAmount == 0) {
        return true;
    }

    lock_guard<mutex> lock(m_requestersMutex);

    // delete all nullptrs in m_requester set
    for (const auto& requester : m_requesters) {
        if (requester == nullptr) {
            ACSDK_ERROR(LX("freeUpSpace").m("Nullptr found in m_requesters"));
            s_metrics().addCounter("nullptrFoundInRequesters");
            m_requesters.erase(requester);
        }
    }

    vector<shared_ptr<Requester>> sortedRequesters(m_requesters.begin(), m_requesters.end());
    sort(sortedRequesters.begin(),
         sortedRequesters.end(),
         [](const shared_ptr<Requester>& lhs, const shared_ptr<Requester>& rhs) {
             if (lhs->getPriority() > rhs->getPriority()) {
                 return true;
             }
             return lhs->getPriority() == rhs->getPriority() && lhs->getLastUsed() < rhs->getLastUsed();
         });

    size_t deletedAmount = 0;
    for (auto& requester : sortedRequesters) {
        if (!requester->isDownloaded()) {
            ACSDK_DEBUG(
                    LX("freeUpSpace").m("Skipping over since it's not downloaded").d("requester", requester->name()));
            continue;
        }

        // we've reached the end of our list and there is no more inactive sortedRequesters to delete, we've failed.
        if (requester->getPriority() == Priority::ACTIVE || requester->getPriority() == Priority::PENDING_ACTIVATION) {
            ACSDK_ERROR(LX("freeUpSpace").m("No more inactive requesters found, cannot delete any more artifacts"));
            break;
        }

        auto size = requester->deleteAndCleanup();
        auto name = requester->name();
        m_requesters.erase(requester);
        deletedAmount += size;
        ACSDK_INFO(LX("freeUpSpace")
                           .m("Deleted request and cleared up of space")
                           .d("name", name)
                           .d("space cleared", size));
        if (deletedAmount >= requestedAmount) {
            ACSDK_INFO(LX("freeUpSpace")
                               .m("Successfully cleared")
                               .d("cleared bytes", deletedAmount)
                               .d("requested bytes", requestedAmount));
            return true;
        }
    }

    auto remainingAmount = requestedAmount - deletedAmount;
    ACSDK_ERROR(LX("freeUpSpace").m("Could not free up enough space").d("bytes remaining", remainingAmount));
    s_metrics().addCounter(METRIC_PREFIX_ERROR("freeUpSpaceFailed")).addString("remaining", to_string(remainingAmount));
    return false;
}

void AssetManager::onIdleChanged(int value) {
    bool isIdle = (static_cast<IdleState>(value) != IdleState::ACTIVE);
    m_davsClient->setIdleState(isIdle);
}

size_t AssetManager::getBudget() {
    return m_storageManager->getBudget();
}

void AssetManager::setBudget(uint32_t value) {
    m_storageManager->setBudget(value);
}

bool AssetManager::functionToBeInvoked(const std::string& name, std::string value) {
    if (name == AMD::REGISTER_PROP) {
        return queueDownloadArtifact(value);
    }
    if (name == AMD::REMOVE_PROP) {
        deleteArtifact(value);
        return true;
    }
    ACSDK_ERROR(LX("functionToBeInvoked").m("Invalid function name").d("name", name));
    return false;
}

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
