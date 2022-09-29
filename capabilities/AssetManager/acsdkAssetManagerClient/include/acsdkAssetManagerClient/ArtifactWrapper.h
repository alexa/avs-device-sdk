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

#ifndef ACSDKASSETMANAGERCLIENT_ARTIFACTWRAPPER_H_
#define ACSDKASSETMANAGERCLIENT_ARTIFACTWRAPPER_H_

#include <acsdk/Notifier/Notifier.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

#include "acsdkAssetManagerClientInterfaces/ArtifactUpdateValidator.h"
#include "acsdkAssetManagerClientInterfaces/ArtifactWrapperInterface.h"
#include "acsdkAssetsInterfaces/ArtifactRequest.h"
#include "acsdkAssetsInterfaces/Communication/AmdCommunicationInterface.h"
#include "acsdkAssetsInterfaces/Priority.h"
#include "acsdkAssetsInterfaces/State.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace client {

/**
 * This class provides a mechanism for controlling artifacts in asset manager through aipc.
 */
class ArtifactWrapper
        : public clientInterfaces::ArtifactWrapperInterface
        , public notifier::Notifier<clientInterfaces::ArtifactChangeObserver>
        , public acsdkCommunicationInterfaces::CommunicationPropertyChangeSubscriber<int>
        , public acsdkCommunicationInterfaces::CommunicationPropertyChangeSubscriber<std::string> {
public:
    /**
     * Creates an artifact wrapper and request a download and creation on asset manager side. If the creation was
     * successful or if the artifact already existed on asset manager, then this will return a valid artifact wrapper to
     * manage that artifact.
     *
     * @param aipcWrapper REQUIRED, for aipc communication with asset manager
     * @param request REQUIRED, to uniquely identify the artifact to download or use.
     * @param updateValidator OPTIONAL, if none is provided, then the update will always be applied. Otherwise, the
     * validator will be used to confirm that the new artifact is valid and should be applied.
     * @return NULLABLE, a smart pointer to an artifact wrapper if successfully registered, null otherwise.
     */
    static std::shared_ptr<ArtifactWrapper> create(
            const std::shared_ptr<commonInterfaces::AmdCommunicationInterface>& amdComm,
            const std::shared_ptr<commonInterfaces::ArtifactRequest>& request,
            const std::shared_ptr<clientInterfaces::ArtifactUpdateValidator>& updateValidator = nullptr);

    ~ArtifactWrapper() override = default;

    inline bool operator==(const ArtifactWrapper& rhs) const {
        return m_request->getSummary() == rhs.m_request->getSummary();
    }

    inline bool operator!=(const ArtifactWrapper& rhs) const {
        return !(rhs == *this);
    }

    inline std::string name() const override {
        return m_request->getSummary();
    }

    /**
     * Requests the download of the artifact referenced by this wrapper if not already downloaded or downloading.
     *
     * @return true if the request was submitted successfully, false otherwise.
     */
    bool download() const override;

    /**
     * @return true if the artifact is already downloaded and ready.
     */
    inline bool isAvailable() const override {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_state == commonInterfaces::State::LOADED;
    }

    /**
     * @return if the artifact is being created, requested, or downloading.
     */
    inline bool isPending() const override {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_state == commonInterfaces::State::INIT || m_state == commonInterfaces::State::REQUESTING ||
               m_state == commonInterfaces::State::DOWNLOADING;
    }

    inline std::shared_ptr<commonInterfaces::ArtifactRequest> getRequest() const override {
        return m_request;
    }

    /**
     * @return the path where to find the artifact on disk using aipc.
     */
    std::string getPath() const override;

    /**
     * @return gets the current artifact priority using aipc.
     */
    commonInterfaces::Priority getPriority() const override;

    /**
     * Sets the priority accordingly.
     *
     * @return true if successful, false otherwise.
     */
    bool setPriority(commonInterfaces::Priority priority) override;

    /**
     * Requests the removal and cleanup of the given artifact.
     */
    void erase() override;

    void addWeakPtrObserver(const std::weak_ptr<clientInterfaces::ArtifactChangeObserver>& observer) override {
        return notifier::Notifier<clientInterfaces::ArtifactChangeObserver>::addWeakPtrObserver(observer);
    }

    void removeWeakPtrObserver(const std::weak_ptr<clientInterfaces::ArtifactChangeObserver>& observer) override {
        return notifier::Notifier<clientInterfaces::ArtifactChangeObserver>::removeWeakPtrObserver(observer);
    }

private:
    /// Constructor
    ArtifactWrapper(
            std::shared_ptr<commonInterfaces::AmdCommunicationInterface> amdComm,
            std::shared_ptr<commonInterfaces::ArtifactRequest> request,
            const std::shared_ptr<clientInterfaces::ArtifactUpdateValidator>& updateValidator = nullptr);

    /// @name CommunicationPropertyChangeSubscriber Functions
    /// @{
    void onCommunicationPropertyChange(const std::string& PropertyName, int newValue) override;
    void onCommunicationPropertyChange(const std::string& PropertyName, std::string newValue) override;
    /// @}

    /**
     * The event when asset manager restarts or is brought up for the first time.
     * We need to sync with the daemon to ensure that we have the right state and that it has the right priority.
     */
    void onAmdInit();

    /**
     * The event when the artifact that we are managing changes state.
     */
    void onStateChange(commonInterfaces::State newState);

    /**
     * The event when the artifact that we are managing has updated with a new path.
     */
    void onUpdateAvailable(const std::string& newPath);

private:
    const std::shared_ptr<commonInterfaces::AmdCommunicationInterface> m_amdComm;
    const std::shared_ptr<commonInterfaces::ArtifactRequest> m_request;
    const std::weak_ptr<clientInterfaces::ArtifactUpdateValidator> m_updateValidator;

    commonInterfaces::State m_state;
    commonInterfaces::Priority m_desiredPriority;
    mutable std::mutex m_stateMutex;
    mutable std::condition_variable m_stateTrigger;
};

}  // namespace client
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGERCLIENT_ARTIFACTWRAPPER_H_
