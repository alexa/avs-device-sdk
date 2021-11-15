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

#include "Requester.h"

#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <sys/stat.h>

#include "acsdkAssetManagerClient/AMD.h"
#include "acsdkAssetsCommon/AmdMetricWrapper.h"
#include "acsdkAssetsCommon/ArchiveWrapper.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

using namespace std;
using namespace chrono;
using namespace client;
using namespace common;
using namespace commonInterfaces;
using namespace alexaClientSDK::avsCommon::utils::timing;

static const auto s_metrics = AmdMetricsWrapper::creator("requester");

/// String to identify log entries originating from this file.
static const std::string TAG{"Requester"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)
#if UNIT_TEST == 1
// For tests, because we don't want to wait hours for it to finish...
static constexpr auto MAX_UPDATE_NOTIFICATIONS = 2;
static constexpr auto UPDATE_RETRY_INTERVAL = milliseconds(100);
#else
static constexpr auto MAX_UPDATE_NOTIFICATIONS = 10;
static constexpr auto UPDATE_RETRY_INTERVAL = seconds(30);
#endif

milliseconds Requester::START_TIME_OFFSET = milliseconds(0);

Requester::Requester(
        shared_ptr<StorageManager> storageManager,
        shared_ptr<AmdCommunicationInterface> communicationHandler,
        shared_ptr<RequesterMetadata> metadata,
        string metadataFilePath) :
        m_storageManager(move(storageManager)),
        m_communicationHandler(move(communicationHandler)),
        m_metadata(move(metadata)),
        m_metadataFilePath(move(metadataFilePath)),
        m_updateNotificationsSent(0),
        m_communicationHandlerRegistered(false) {
}

Requester::~Requester() {
    ACSDK_DEBUG9(LX("~Requester").m("Destroying requester").d("requester", name()));
    unique_lock<mutex> lock(m_eventMutex);
}

bool Requester::initializeFromStorage() {
    lock_guard<mutex> lock(m_eventMutex);
    m_resource = m_storageManager->acquireResource(m_metadata->getResourceId());
    if (m_resource == nullptr) {
        return false;
    }

    setStateLocked(State::LOADED);
    return true;
}

bool Requester::registerCommunicationHandlerPropsLocked() {
    if (m_communicationHandlerRegistered) {
        return true;
    }

    auto summary = m_metadata->getRequest()->getSummary();
    auto stateProp = summary + AMD::STATE_SUFFIX;
    auto priorityProp = summary + AMD::PRIORITY_SUFFIX;
    auto pathProp = summary + AMD::PATH_SUFFIX;

    if (!(m_stateProperty =
                  m_communicationHandler->registerProperty(stateProp, static_cast<int>(State::INIT), nullptr))) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("CommunicationHandlerRegisterFailed"));
        ACSDK_ERROR(LX("registerCommunicationHandlerPropsLocked").m("failed to register state property"));
        return false;
    }

    if (!(m_priorityProperty = m_communicationHandler->registerProperty(
                  priorityProp, static_cast<int>(Priority::UNUSED), shared_from_this()))) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("CommunicationHandlerRegisterFailed"));
        ACSDK_ERROR(LX("registerCommunicationHandlerPropsLocked").m("failed to register priority property"));
        return false;
    }

    if (!m_communicationHandler->registerFunction(pathProp, shared_from_this())) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("CommunicationHandlerRegisterFailed"));
        ACSDK_ERROR(LX("registerCommunicationHandlerPropsLocked").m("failed to register path function"));
        return false;
    }
    if (!(m_updateProperty =
                  m_communicationHandler->registerProperty(summary + AMD::UPDATE_SUFFIX, summary, nullptr))) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("CommunicationHandlerRegisterFailed"));
        ACSDK_ERROR(
                LX("registerCommunicationHandlerPropsLocked").m("failed to register path updated changed property"));
        return false;
    }

    m_communicationHandlerRegistered = true;
    return true;
}

void Requester::deregisterCommunicationHandlerPropsLocked(unique_lock<mutex>& lock) {
    if (!m_communicationHandlerRegistered) {
        return;
    }

    auto summary = m_metadata->getRequest()->getSummary();
    auto stateProp = summary + AMD::STATE_SUFFIX;
    auto priorityProp = summary + AMD::PRIORITY_SUFFIX;
    auto pathProp = summary + AMD::PATH_SUFFIX;

    lock.unlock();

    m_communicationHandler->deregisterProperty(stateProp, m_stateProperty);
    m_communicationHandler->deregisterProperty(priorityProp, m_priorityProperty);
    m_communicationHandler->deregister(pathProp, shared_from_this());
    m_communicationHandler->deregisterProperty(summary + AMD::UPDATE_SUFFIX, m_updateProperty);

    lock.lock();

    m_communicationHandlerRegistered = false;
}

size_t Requester::deleteAndCleanupLocked(unique_lock<mutex>& lock) {
    ACSDK_DEBUG(LX("deleteAndCleanupLocked").m("Releasing resources").d("requester", name()));
    m_storageReservationToken.reset();
    auto clearedTotal = m_storageManager->releaseResource(m_resource);
    m_resource.reset();
    clearedTotal += m_storageManager->releaseResource(m_pendingUpdate);
    m_pendingUpdate.reset();

    if (clearedTotal > 0) {
        ACSDK_INFO(LX("deleteAndCleanupLocked").m("Deleted resource referenced").d("requester", name()));
    }

    ACSDK_DEBUG(LX("deleteAndCleanupLocked").m("Clearing metadata").d("requester", name()));
    m_metadata->clear(m_metadataFilePath);

    ACSDK_DEBUG(LX("deleteAndCleanupLocked").m("Clearing properties"));
    setStateLocked(State::INVALID);
    // it's important to deregister the properties as soon as the deletion happens

    deregisterCommunicationHandlerPropsLocked(lock);

    ACSDK_INFO(LX("deleteAndCleanupLocked").m("Requester has been cleared").d("requester", name()));
    return clearedTotal;
}

bool Requester::handleAcquiredResourceLocked(unique_lock<mutex>& lock, const shared_ptr<Resource>& newResource) {
    // if we already have a downloaded artifact, then announce of a pending upgrade
    if (getState() == State::LOADED) {
        ACSDK_INFO(
                LX("handleAcquiredResourceLocked").m("Acquired an update, awaiting validation").d("artifact", name()));
        m_pendingUpdate = newResource;
        notifyUpdateIsAvailableLocked(lock);
        return true;
    }

    // resource should be null, but release this just to be on the safe side
    m_storageManager->releaseResource(m_resource);
    m_resource = newResource;
    m_metadata->setResourceId(newResource->getId());
    updateLastUsedTimestampLocked();
    if (!m_metadata->saveToFile(m_metadataFilePath)) {
        ACSDK_CRITICAL(LX("handleAcquiredResourceLocked")
                               .m("Failed to save the appropriate metadata for requester")
                               .d("requester", name()));
        handleDownloadFailureLocked(lock);
        return false;
    }

    ACSDK_INFO(LX("handleAcquiredResourceLocked").m("Downloaded artifact is ready"));
    setStateLocked(State::LOADED);
    return true;
}

void Requester::handleDownloadFailureLocked(unique_lock<mutex>& lock) {
    if (getState() == State::LOADED) {
        ACSDK_ERROR(LX("handleDownloadFailureLocked").m("Failed to update artifact").d("requester", name()));
        return;
    }

    deleteAndCleanupLocked(lock);
}

void Requester::setPriority(Priority newPriority) {
    ACSDK_INFO(LX("setPriority").m("Updating priority").d("artifact", name()).d("newPriority", toString(newPriority)));
    lock_guard<mutex> lock(m_eventMutex);
    if (m_communicationHandlerRegistered && m_priorityProperty != nullptr) {
        m_priorityProperty->setValue(static_cast<int>(newPriority));
    }
}

string Requester::getArtifactPath() {
    lock_guard<mutex> lock(m_eventMutex);
    if (m_resource == nullptr) {
        return "";
    }
    if (getState() == State::LOADED && !m_resource->getPath().empty()) {
        updateLastUsedTimestampLocked();
        m_metadata->saveToFile(m_metadataFilePath);
    }
    return m_resource->getPath();
}

void Requester::updateLastUsedTimestampLocked() {
    auto lastUsed = START_TIME_OFFSET + duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
    ACSDK_DEBUG(LX("updateLastUsedTimestampLocked")
                        .m("Changing usage timestamp")
                        .d("artifact", name())
                        .d("New timestamp", lastUsed.count()));
    m_metadata->setLastUsed(lastUsed);
}

void Requester::notifyUpdateIsAvailableLocked(unique_lock<mutex>& lock) {
    if (m_pendingUpdate == nullptr) {
        m_timer.stop();
        return;
    }
    // setup retry in case we do not get a response
    if (m_updateNotificationsSent == 0) {
        m_timer.start(UPDATE_RETRY_INTERVAL, Timer::PeriodType::RELATIVE, MAX_UPDATE_NOTIFICATIONS, [this] {
            unique_lock<mutex> lock(m_eventMutex);
            notifyUpdateIsAvailableLocked(lock);
        });
    }
    if (++m_updateNotificationsSent > MAX_UPDATE_NOTIFICATIONS) {
        ACSDK_ERROR(LX("notifyUpdateIsAvailableLocked")
                            .m("Tried notifying client, rejecting update")
                            .d("Times Notified ", MAX_UPDATE_NOTIFICATIONS)
                            .d("artifact", name()));
        handleUpdateLocked(lock, false);
        return;
    }
    auto updateEvent = m_metadata->getRequest()->getSummary() + AMD::UPDATE_SUFFIX;
    auto newPath = m_pendingUpdate->getPath();
    m_updateProperty->setValue(newPath);
}

void Requester::handleUpdate(bool accept) {
    unique_lock<mutex> lock(m_eventMutex);
    handleUpdateLocked(lock, accept);
}

void Requester::handleUpdateLocked(unique_lock<mutex>& lock, bool accept) {
    m_timer.stop();
    m_updateNotificationsSent = 0;
    if (!accept) {
        ACSDK_WARN(LX("handleUpdateLocked").m("Rejecting update").d("requester", name()));
        s_metrics().addCounter("updateRejected").addString("request", name());
        m_storageManager->releaseResource(m_pendingUpdate);
        m_pendingUpdate.reset();
        return;
    }

    if (m_pendingUpdate == nullptr) {
        ACSDK_ERROR(LX("handleUpdateLocked").m("There is no update to apply").d("requester", name()));
        s_metrics()
                .addCounter(METRIC_PREFIX_ERROR("updateFailed"))
                .addString("error", "nullPendingUpdate")
                .addString("request", name());
        return;
    }

    ACSDK_INFO(LX("handleUpdateLocked").m("Applying update").d("requester", name()));
    s_metrics().addCounter("updateAccepted").addString("request", name());
    m_metadata->setResourceId(m_pendingUpdate->getId());
    m_metadata->saveToFile(m_metadataFilePath);
    m_storageManager->releaseResource(m_resource);
    m_resource = move(m_pendingUpdate);
}
bool Requester::validateWriteRequest(const std::string& name, int newValue) {
    auto summary = m_metadata->getRequest()->getSummary();
    auto priorityProp = summary + AMD::PRIORITY_SUFFIX;
    if (name == priorityProp) {
        if (!isValidPriority(newValue)) {
            ACSDK_ERROR(LX("validateWriteRequest").d("invalid priority", newValue));
            return false;
        }
        ACSDK_DEBUG(LX("validateWriteRequest").m("Valid Priority"));
        return true;
    }

    //    Default return false.
    return false;
}
string Requester::functionToBeInvoked(const std::string& name) {
    auto summary = m_metadata->getRequest()->getSummary();
    auto pathProp = summary + AMD::PATH_SUFFIX;

    if (name == pathProp) {
        return this->getArtifactPath();
    }
    return "";
}

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
