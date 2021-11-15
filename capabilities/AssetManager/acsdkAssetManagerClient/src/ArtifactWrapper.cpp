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

#include "acsdkAssetManagerClient/ArtifactWrapper.h"
#include <AVSCommon/Utils/Logger/Logger.h>
#include <thread>

#include "acsdkAssetManagerClient/AMD.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace client {

using namespace std;
using namespace chrono;
using namespace commonInterfaces;
using namespace clientInterfaces;

static constexpr auto DOWNLOAD_ACK_TIMEOUT = seconds(5);

/// String to identify log entries originating from this file.
static const std::string TAG{"ArtifactWrapper"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<ArtifactWrapper> ArtifactWrapper::create(
        const shared_ptr<AmdCommunicationInterface>& amdComm,
        const shared_ptr<commonInterfaces::ArtifactRequest>& request,
        const shared_ptr<ArtifactUpdateValidator>& updateValidator) {
    if (nullptr == amdComm) {
        ACSDK_ERROR(LX("create").m("Null AmdCommunicationInterface"));
        return nullptr;
    }

    if (nullptr == request) {
        ACSDK_ERROR(LX("create").m("Null ArtifactRequest"));
        return nullptr;
    }

    auto wrapper = shared_ptr<ArtifactWrapper>(new ArtifactWrapper(amdComm, request, updateValidator));
    if (nullptr == wrapper) {
        ACSDK_ERROR(LX("create").m("ArtifactWrapper create failed"));
        return nullptr;
    }

    // subscribe to assetmgrd initialization event
    if (!amdComm->subscribeToPropertyChangeEvent(
                AMD::INITIALIZATION_PROP, static_pointer_cast<CommunicationPropertyChangeSubscriber<int>>(wrapper))) {
        ACSDK_ERROR(LX("create").m("Failed to register for Asset Manager initialization event"));
        return nullptr;
    }

    // subscribe to state changes
    if (!amdComm->subscribeToPropertyChangeEvent(
                request->getSummary() + AMD::STATE_SUFFIX,
                static_pointer_cast<CommunicationPropertyChangeSubscriber<int>>(wrapper))) {
        ACSDK_ERROR(LX("create").m("Failed to register for state changes"));
        return nullptr;
    }

    // subscribe to updates
    if (!amdComm->subscribeToPropertyChangeEvent(
                request->getSummary() + AMD::UPDATE_SUFFIX,
                static_pointer_cast<CommunicationPropertyChangeSubscriber<string>>(wrapper))) {
        ACSDK_ERROR(LX("create").m("Failed to register for update changes"));
        return nullptr;
    }

    // initialize current state
    auto currentState = static_cast<int>(State::INIT);
    amdComm->readProperty(request->getSummary() + AMD::STATE_SUFFIX, currentState);
    wrapper->m_state = static_cast<State>(currentState);

    // initialize current priority
    auto currentPriority = static_cast<int>(Priority::UNUSED);
    amdComm->readProperty(request->getSummary() + AMD::PRIORITY_SUFFIX, currentPriority);
    wrapper->m_desiredPriority = static_cast<Priority>(currentPriority);

    ACSDK_INFO(LX("create").d("Artifact registration succeeded for", request->getSummary()));
    return wrapper;
}

ArtifactWrapper::ArtifactWrapper(
        shared_ptr<AmdCommunicationInterface> amdComm,
        shared_ptr<ArtifactRequest> request,
        const shared_ptr<ArtifactUpdateValidator>& updateValidator) :
        m_amdComm(move(amdComm)),
        m_request(move(request)),
        m_updateValidator(updateValidator),
        m_state(State::INIT),
        m_desiredPriority(Priority::UNUSED) {
}

bool ArtifactWrapper::download() const {
    ACSDK_INFO(LX("download").m("Downloading").d("name", name()));
    auto result = m_amdComm->invoke(AMD::REGISTER_PROP, m_request->toJsonString());
    if (!result.isSucceeded() || !result.value()) {
        ACSDK_ERROR(LX("download").m("Failed to initiate download").d("name", name()));
        return false;
    }

    std::unique_lock<std::mutex> lock(m_stateMutex);
    return m_stateTrigger.wait_for(lock, DOWNLOAD_ACK_TIMEOUT, [this] { return m_state != State::INIT; });
}

string ArtifactWrapper::getPath() const {
    auto result = m_amdComm->invoke(m_request->getSummary() + AMD::PATH_SUFFIX);
    if (!result.isSucceeded()) {
        ACSDK_ERROR(LX("getPath").m("Could not read path property").d("name", name()));
    }
    return result.value();
}

Priority ArtifactWrapper::getPriority() const {
    int priority;
    if (!m_amdComm->readProperty(m_request->getSummary() + AMD::PRIORITY_SUFFIX, priority)) {
        ACSDK_ERROR(LX("getPriority").m("Could not read priority property").d("name", name()));
        return Priority::UNUSED;
    }
    return static_cast<Priority>(priority);
}

bool ArtifactWrapper::setPriority(Priority priority) {
    lock_guard<mutex> lock(m_stateMutex);
    m_desiredPriority = priority;

    if (!m_amdComm->writeProperty(m_request->getSummary() + AMD::PRIORITY_SUFFIX, static_cast<int>(priority))) {
        ACSDK_ERROR(LX("setPriority").m("Could not write priority property").d("priority", priority).d("name", name()));
        return false;
    }
    return true;
}

void ArtifactWrapper::erase() {
    ACSDK_INFO(LX("erase").d("Erasing", name()));

    auto result = m_amdComm->invoke(AMD::REMOVE_PROP, m_request->getSummary());
    if (!result.isSucceeded() || !result.value()) {
        ACSDK_ERROR(LX("erase").m("Could not write erase property"));
    }

    unique_lock<mutex> lock(m_stateMutex);
    auto erased = m_stateTrigger.wait_for(
            lock, seconds(1), [this] { return m_state == State::INIT || m_state == State::INVALID; });
    if (!erased) {
        ACSDK_DEBUG9(LX("erase").m("Timed out waiting to be erased").d("name", name()).d("state", m_state));
    }
}

void ArtifactWrapper::onAmdInit() {
    ACSDK_DEBUG(LX("onAmdInit").m("Asset Manager has been restarted"));

    auto currentState = static_cast<int>(State::INVALID);
    if (m_amdComm->readProperty(m_request->getSummary() + AMD::STATE_SUFFIX, currentState)) {
        onStateChange(static_cast<State>(currentState));
    } else {
        ACSDK_ERROR(LX("onAmdInit").m("Could not read state property upon initialization"));
    }

    setPriority(m_desiredPriority);
}

void ArtifactWrapper::onStateChange(commonInterfaces::State newState) {
    std::unique_lock<std::mutex> lock(m_stateMutex);
    ACSDK_INFO(LX("onStateChange")
                       .m("State changed")
                       .d("name", name())
                       .d("old state", toString(m_state))
                       .d("new state", toString(newState)));
    m_state = newState;
    lock.unlock();
    m_stateTrigger.notify_all();
    notifyObservers(
            [=](const shared_ptr<ArtifactChangeObserver>& observer) { observer->stateChanged(m_request, newState); });
}

void ArtifactWrapper::onUpdateAvailable(const string& newPath) {
    ACSDK_INFO(LX("onUpdateAvailable").m("New update is available").d("name", name()).sensitive("path", newPath));
    auto validator = m_updateValidator.lock();
    auto action = (validator == nullptr || validator->validateUpdate(m_request, newPath)) ? AMD::ACCEPT_UPDATE_PROP
                                                                                          : AMD::REJECT_UPDATE_PROP;
    m_amdComm->writeProperty(action, m_request->getSummary());
}
void ArtifactWrapper::onCommunicationPropertyChange(const string& propertyName, int newValue) {
    if (propertyName == AMD::INITIALIZATION_PROP) {
        if (newValue == 1) {
            onAmdInit();
        }
        return;
    }

    if (propertyName == m_request->getSummary() + AMD::STATE_SUFFIX) {
        onStateChange(static_cast<State>(newValue));
        return;
    }
}
void ArtifactWrapper::onCommunicationPropertyChange(const string& propertyName, string newValue) {
    if (propertyName == m_request->getSummary() + AMD::UPDATE_SUFFIX) {
        onUpdateAvailable(newValue);
    }
}

}  // namespace client
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
