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

#include "DavsRequester.h"

#include <AVSCommon/Utils/Error/FinallyGuard.h>
#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdkAssetsCommon/AmdMetricWrapper.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

using namespace std;
using namespace chrono;
using namespace client;
using namespace common;
using namespace commonInterfaces;
using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::error;

static const auto s_metrics = AmdMetricsWrapper::creator("davsRequester");

/// String to identify log entries originating from this file.
static const std::string TAG{"DavsRequester"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

bool DavsRequester::download() {
    ACSDK_INFO(LX("download").m("Requesting download").d("requester", name()));
    unique_lock<mutex> lock(m_eventMutex);
    auto state = getState();
    if (state != State::INVALID && state != State::INIT) {
        ACSDK_INFO(LX("download").m("Download is unnecessary").d("file", name()).d("Already in state", state));
        return true;
    }

    auto davsRequest = static_pointer_cast<DavsRequest>(m_metadata->getRequest());
    if (getPriority() == Priority::ACTIVE) {
        m_davsRequestId = m_davsClient->registerArtifact(
                davsRequest,
                static_pointer_cast<DavsRequester>(shared_from_this()),
                static_pointer_cast<DavsRequester>(shared_from_this()),
                true);
    } else {
        m_davsRequestId = m_davsClient->downloadOnce(
                davsRequest,
                static_pointer_cast<DavsRequester>(shared_from_this()),
                static_pointer_cast<DavsRequester>(shared_from_this()));
    }

    if (m_davsRequestId.empty()) {
        ACSDK_ERROR(LX("download").m("Could not register request with DAVS Client").d("requester", name()));
        handleDownloadFailureLocked(lock);
        return false;
    }

    if (!registerCommunicationHandlerPropsLocked()) {
        ACSDK_ERROR(LX("download").m("Could not register Communication Handler properties").d("requester", name()));
        handleDownloadFailureLocked(lock);
        return false;
    }

    setStateLocked(State::REQUESTING);
    ACSDK_INFO(LX("download").m("Creating a request").d("requester", name()));
    return true;
}

size_t DavsRequester::deleteAndCleanupLocked(unique_lock<mutex>& lock) {
    ACSDK_DEBUG(LX("deleteAndCleanupLocked").m("Deregistering Artifact from DavsClient").d("requester", name()));
    m_davsClient->deregisterArtifact(m_davsRequestId);
    m_davsRequestId.clear();
    return Requester::deleteAndCleanupLocked(lock);
}

void DavsRequester::adjustAutoUpdateBasedOnPriority(Priority newPriority) {
    lock_guard<mutex> lock(m_eventMutex);
    if (getState() != State::LOADED) {
        return;
    }
    if (newPriority != Priority::ACTIVE) {
        m_davsClient->enableAutoUpdate(m_davsRequestId, false);
        return;
    }

    // if for some reason we don't have a request ID for this ACTIVE artifact, then register our request with DAVS to
    // get updates.
    if (m_davsRequestId.empty()) {
        m_davsRequestId = m_davsClient->registerArtifact(
                static_pointer_cast<DavsRequest>(m_metadata->getRequest()),
                static_pointer_cast<DavsRequester>(shared_from_this()),
                static_pointer_cast<DavsRequester>(shared_from_this()),
                false);

    } else {
        m_davsClient->enableAutoUpdate(m_davsRequestId, true);
    }
}

bool DavsRequester::checkIfOkToDownload(shared_ptr<VendableArtifact> availableArtifact, size_t freeSpaceNeeded) {
    ACSDK_INFO(LX("checkIfOkToDownload").d("requester", name()));

    auto newUUID = availableArtifact->getUniqueIdentifier();
    unique_lock<mutex> lock(m_eventMutex);
    if (m_davsRequestId.empty()) {
        ACSDK_WARN(LX("checkIfOkToDownload").m("Got a check response from Davs Client even though we deregistered"));
        handleDownloadFailureLocked(lock);
        return false;
    }

    // if the current available artifact is the same as the one stored, then there's nothing to change
    if (newUUID == m_metadata->getResourceId()) {
        ACSDK_INFO(LX("checkIfOkToDownload").m("Artifact is already downloaded").d("requester", name()));
        return false;
    }

    if (m_pendingUpdate != nullptr && newUUID == m_pendingUpdate->getId()) {
        ACSDK_INFO(LX("checkIfOkToDownload").m("Artifact is already pending update").d("requester", name()));
        return false;
    }

    auto newResource = m_storageManager->acquireResource(newUUID);
    if (newResource != nullptr) {
        ACSDK_INFO(LX("checkIfOkToDownload")
                           .m("We already have this artifact available from another request")
                           .d("request", newResource->getPath()));
        handleAcquiredResourceLocked(lock, newResource);
        return false;
    }

    m_storageReservationToken.reset();
    lock.unlock();  // don't need the lock for this and the operation can be time consuming.
    // only reserve as much space as is required for the artifact (regardless if we're going to unpack it or not)
    // we may need to request a bit more space later after unpacking, but it's difficult to estimate how much up front
    auto reservation = m_storageManager->reserveSpace(availableArtifact->getArtifactSizeBytes());
    lock.lock();  // re-acquire the lock
    if (reservation == nullptr) {
        ACSDK_ERROR(LX("checkIfOkToDownload").m("Could not free up enough space").d("requester", name()));
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("insufficientSpace"));
        handleDownloadFailureLocked(lock);
        return false;
    }
    m_storageReservationToken = move(reservation);

    // if we already have an artifact available and there's a new one
    ACSDK_INFO(LX("checkIfOkToDownload")
                       .d("Requesting artifact", name())
                       .d("state", (getState() == State::LOADED) ? "update" : "download"));
    return true;
}

void DavsRequester::onCheckFailure(ResultCode errorCode) {
    ACSDK_ERROR(LX("onCheckFailure").m("Check failed").d("artifact", name()).d("error", static_cast<int>(errorCode)));

    unique_lock<mutex> lock(m_eventMutex);
    handleDownloadFailureLocked(lock);
}

void DavsRequester::onStart() {
    ACSDK_INFO(LX("onStart").m("Download has started").d("requester", name()));

    lock_guard<mutex> lock(m_eventMutex);
    if (getState() == State::LOADED) {
        return;
    }
    setStateLocked(State::DOWNLOADING);
}

void DavsRequester::onArtifactDownloaded(shared_ptr<VendableArtifact> downloadedArtifact, const string& path) {
    ACSDK_INFO(LX("onArtifactDownloaded").m("Download finished").d("requester", name()));

    FinallyGuard cleanupDirectories([&] {  // ok and required to pass by reference
        filesystem::removeAll(path);
    });

    unique_lock<mutex> lock(m_eventMutex);
    if (m_davsRequestId.empty()) {
        ACSDK_WARN(
                LX("onArtifactDownloaded").m("Got a download response from Davs Client even though we deregistered"));
        handleDownloadFailureLocked(lock);
        return;
    }

    auto newResource = m_storageManager->registerAndAcquireResource(
            move(m_storageReservationToken), downloadedArtifact->getUniqueIdentifier(), path);
    if (newResource == nullptr) {
        ACSDK_ERROR(LX("onArtifactDownloaded").m("Failed to register and acquire the resource").d("resource", name()));
        handleDownloadFailureLocked(lock);
        return;
    }

    handleAcquiredResourceLocked(lock, newResource);
}

void DavsRequester::onDownloadFailure(ResultCode errorCode) {
    ACSDK_ERROR(
            LX("onDownloadFailure").m("Download failed").d("artifact", name()).d("error", static_cast<int>(errorCode)));
    unique_lock<mutex> lock(m_eventMutex);
    handleDownloadFailureLocked(lock);
}

void DavsRequester::onProgressUpdate(int progress) {
    ACSDK_INFO(LX("onProgressUpdate").m("Download progress").d("artifact", name()).d("progress", progress));
}

bool DavsRequester::validateWriteRequest(const std::string& name, int newValue) {
    auto summary = m_metadata->getRequest()->getSummary();
    auto priorityProp = summary + AMD::PRIORITY_SUFFIX;
    auto result = Requester::validateWriteRequest(name, newValue);
    if (result && name == priorityProp) {
        adjustAutoUpdateBasedOnPriority(static_cast<Priority>(newValue));
        return true;
    }
    // Default return false.
    return false;
}

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
