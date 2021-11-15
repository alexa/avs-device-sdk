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

#ifndef ACSDKASSETMANAGER_SRC_REQUESTER_H_
#define ACSDKASSETMANAGER_SRC_REQUESTER_H_

#include <AVSCommon/Utils/Timing/Timer.h>
#include <mutex>
#include <string>

#include "RequesterMetadata.h"
#include "Resource.h"
#include "StorageManager.h"
#include "acsdkAssetManagerClient/AMD.h"
#include "acsdkAssetsInterfaces/Communication/AmdCommunicationInterface.h"
#include "acsdkAssetsInterfaces/State.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

class RequesterFactory;

/**
 * This class represents an artifact as it exists on the system or as it is being downloaded. Given an artifact
 * directory, it will maintain a metadata json file that will maintain its state and description on the same path. The
 * artifact will be stored or unzipped inside this directory to handle its update and maintenance.
 */
class Requester
        : public acsdkCommunicationInterfaces::CommunicationPropertyValidatorInterface<int>
        , public acsdkCommunicationInterfaces::FunctionInvokerInterface<std::string>
        , public std::enable_shared_from_this<Requester> {
public:
    /**
     * steady_clock starting offset based off of the previous artifact times.
     * @note this is done to get around the issue with changing system_clock times and synchronizations.
     */
    static std::chrono::milliseconds START_TIME_OFFSET;

    /**
     * Ensures that the Communication Handler properties are all cleared away.
     */
    virtual ~Requester();

    /**
     * Issues a download request if not already in progress.
     *
     * @return true if the artifact is already downloaded or can download.
     */
    virtual bool download() = 0;

    /**
     * Deletes the artifact and deregisters Communication Handler properties accordingly.
     */
    inline size_t deleteAndCleanup() {
        std::unique_lock<std::mutex> lock(m_eventMutex);
        return deleteAndCleanupLocked(lock);
    }

    /**
     * Handles pendingUpdate resource according to this function call.
     * @param accept the pendingUpdate change or not.
     *               If accepted, release the old resource and set the new one.
     *               If rejected, then release the pendingUpdate.
     */
    void handleUpdate(bool accept);

    /**
     * @return name of this artifact based on the summary.
     */
    inline std::string name() const {
        return m_metadata->getRequest()->getSummary();
    }

    /**
     * @return original request which describes this artifact.
     */
    inline const std::shared_ptr<commonInterfaces::ArtifactRequest>& getArtifactRequest() const {
        return m_metadata->getRequest();
    }

    /**
     * @return the current state for this artifact;
     */
    inline commonInterfaces::State getState() const {
        if (m_communicationHandlerRegistered) {
            auto state = static_cast<commonInterfaces::State>(m_stateProperty->getValue());
            return state;
        }
        return commonInterfaces::State::INVALID;
    }

    /**
     * @return last time the artifact was created or used.
     */
    inline std::chrono::milliseconds getLastUsed() const {
        return m_metadata->getLastUsed();
    }

    /**
     * @return the current priority for this artifact.
     */
    inline commonInterfaces::Priority getPriority() const {
        if (m_communicationHandlerRegistered && m_priorityProperty != nullptr) {
            return static_cast<commonInterfaces::Priority>(m_priorityProperty->getValue());
        }
        return commonInterfaces::Priority::UNUSED;
    }

    /**
     * @return true if the artifact is downloaded on the system.
     */
    inline bool isDownloaded() {
        return m_resource != nullptr;
    }

    /**
     * Returns the path where the artifact is stored and updates the last used timestamp if the path exists.
     *
     * @return path of the artifact if exists, empty string otherwise.
     */
    std::string getArtifactPath();

    /**
     * Sets the priority of this artifact to a new value.
     */
    virtual void setPriority(commonInterfaces::Priority newPriority);

    /// Override of the CommunicationPropertyValidatorInterface
    bool validateWriteRequest(const std::string& name, int newValue) override;

    /// Override of the InvokeFunctionInterface
    std::string functionToBeInvoked(const std::string& Name) override;

protected:
    Requester(
            std::shared_ptr<StorageManager> storageManager,
            std::shared_ptr<commonInterfaces::AmdCommunicationInterface> communicationHandler,
            std::shared_ptr<RequesterMetadata> metadata,
            std::string metadataFilePath);

    /**
     * Attempts to fetch the resource from storage manager.
     *
     * @return true if resource is available, false otherwise.
     */
    bool initializeFromStorage();

    /**
     * Registers the Communication Handler properties, if not already registered, for state, priority, and path.
     *
     * @return true if the properties were registered successfully, false otherwise.
     */
    bool registerCommunicationHandlerPropsLocked();

    /**
     * Deletes the artifact and deregisters communication properties accordingly.
     */
    void deregisterCommunicationHandlerPropsLocked(std::unique_lock<std::mutex>& lock);

    /**
     * Sets the existing state and informs the manager of the new state.
     */
    inline void setStateLocked(commonInterfaces::State newState) {
        if (nullptr == m_metadata) {
            return;
        }

        if (m_communicationHandlerRegistered) {
            m_stateProperty->setValue(static_cast<int>(newState));
        }
    }

    /**
     * Sets the last used timestamp to the current time.
     */
    void updateLastUsedTimestampLocked();

    void notifyUpdateIsAvailableLocked(std::unique_lock<std::mutex>& lock);

    bool handleAcquiredResourceLocked(std::unique_lock<std::mutex>& lock, const std::shared_ptr<Resource>& newResource);
    void handleDownloadFailureLocked(std::unique_lock<std::mutex>& lock);
    void handleUpdateLocked(std::unique_lock<std::mutex>& lock, bool accept);

    /**
     * Deletes the artifact and deregisters from DAVS Client accordingly.
     */
    virtual size_t deleteAndCleanupLocked(std::unique_lock<std::mutex>& lock);

protected:
    // Manager used to free up space when needed.
    const std::shared_ptr<StorageManager> m_storageManager;
    // Communication Property Handler used for communicating with external processes.
    const std::shared_ptr<commonInterfaces::AmdCommunicationInterface> m_communicationHandler;

    // Artifact metadata containing request and other information for this artifact.
    const std::shared_ptr<RequesterMetadata> m_metadata;
    // Path to the directory where this artifact is stored.
    const std::string m_metadataFilePath;

    // A storage manager space reservation token that (while alive) holds a certain space in storage manager
    std::unique_ptr<StorageManager::ReservationToken> m_storageReservationToken;
    // Pointer to the actual resource used for this request.
    std::shared_ptr<Resource> m_resource;
    // Pointer to the resource that will be held for updating this request.
    std::shared_ptr<Resource> m_pendingUpdate;
    // Total number of update notifications sent for this request
    int m_updateNotificationsSent;
    // The Timer used to schedule updates.
    alexaClientSDK::avsCommon::utils::timing::Timer m_timer;

    // Mutex for synchronizing event states.
    std::mutex m_eventMutex;

    // Are our communication Handler properties registered or not?
    bool m_communicationHandlerRegistered;

    // Allow the requester factory to create requesters
    friend RequesterFactory;

    // Communication Property for state.
    std::shared_ptr<acsdkCommunicationInterfaces::CommunicationProperty<int>> m_stateProperty;
    // Communication Property for priority
    std::shared_ptr<acsdkCommunicationInterfaces::CommunicationProperty<int>> m_priorityProperty;
    // Communication Property for Updates
    std::shared_ptr<acsdkCommunicationInterfaces::CommunicationProperty<std::string>> m_updateProperty;
};

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGER_SRC_REQUESTER_H_
