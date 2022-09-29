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

#include <acsdk/Sample/Endpoint/EndpointFocusAdapter.h>
#include <acsdk/Sample/Console/ConsolePrinter.h>

#include <AVSCommon/Utils/Logger/Logger.h>

/// String to identify log entries originating from this file.
#define TAG "EndpointFocusAdapter"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

using namespace presentationOrchestratorInterfaces;

/// The @c Channel name for use by the FocusManager.
static const char* const CHANNEL_NAME = avsCommon::sdkInterfaces::FocusManagerInterface::CONTENT_CHANNEL_NAME;

std::shared_ptr<EndpointFocusAdapter> EndpointFocusAdapter::create(
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
    std::shared_ptr<PresentationOrchestratorInterface> presentationOrchestrator,
    std::shared_ptr<PresentationOrchestratorStateTrackerInterface> presentationOrchestratorStateTracker) {
    if (!focusManager || !presentationOrchestrator || !presentationOrchestratorStateTracker) {
        ACSDK_WARN(LX(__func__).d("Endpoint Focus Adapter Creation Failed", "Null input"));
        return nullptr;
    }

    auto focusAdapter = std::shared_ptr<EndpointFocusAdapter>(new EndpointFocusAdapter(
        std::move(focusManager), std::move(presentationOrchestrator), std::move(presentationOrchestratorStateTracker)));
    return focusAdapter;
}

EndpointFocusAdapter::EndpointFocusAdapter(
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
    std::shared_ptr<PresentationOrchestratorInterface> presentationOrchestrator,
    std::shared_ptr<PresentationOrchestratorStateTrackerInterface> presentationOrchestratorStateTracker) :
        m_focusManager{std::move(focusManager)},
        m_presentationOrchestrator{std::move(presentationOrchestrator)},
        m_presentationOrchestratorStateTracker{std::move(presentationOrchestratorStateTracker)},
        m_executor(std::make_shared<alexaClientSDK::avsCommon::utils::threading::Executor>()) {
}

void EndpointFocusAdapter::executeAcquireWindow() {
    ACSDK_DEBUG9(LX("executeAcquireWindow"));
    if (m_focusAcquirePtr) {
        /// Clear all existing presentations.
        m_presentationOrchestrator->clearPresentations();
        /// Set the device interface.
        m_presentationOrchestratorStateTracker->setDeviceInterface(m_focusAcquirePtr->interfaceName);
    }
    executeAcquireContentChannel();
}

void EndpointFocusAdapter::executeReleaseWindow() {
    ACSDK_DEBUG9(LX("executeReleaseWindow"));
    m_presentationOrchestratorStateTracker->releaseDeviceInterface();
}

void EndpointFocusAdapter::executeAcquireContentChannel() {
    ACSDK_DEBUG9(LX("executeAcquireContentChannel"));
    if (m_focusAcquirePtr) {
        auto activity = avsCommon::sdkInterfaces::FocusManagerInterface::Activity::create(
            m_focusAcquirePtr->interfaceName, shared_from_this());
        m_focusManager->acquireChannel(CHANNEL_NAME, activity);
    }
}

void EndpointFocusAdapter::executeReleaseContentChannel() {
    ACSDK_DEBUG9(LX("executeReleaseContentChannel"));
    if (m_focusManager) {
        m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
    }
}

void EndpointFocusAdapter::onFocusChanged(
    avsCommon::avs::FocusState newFocus,
    avsCommon::avs::MixingBehavior behavior) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor->execute([this, newFocus] {
        switch (newFocus) {
            case avsCommon::avs::FocusState::FOREGROUND:
                /// Audio Focus has been acquired. Can Play content. After playback is complete, releaseContentChannel()
                /// must be called.
                /// Note the callback is invoked only once per acquireFocus request.
                if (m_focusAcquirePtr) {
                    m_focusAcquirePtr->callback();
                    m_focusAcquirePtr.reset();
                }
                break;
            case avsCommon::avs::FocusState::BACKGROUND:
                /// Audio Focus has been granted. Can Play content, subject to the Mixing Behavior. After playback is
                /// complete, releaseContentChannel() must be called.
                break;
            case avsCommon::avs::FocusState::NONE:
                /// All playback must immediately stop. Further playback would require focus to be acquired again.
                executeReleaseWindow();
                break;
        }
    });
}

void EndpointFocusAdapter::acquireFocus(const std::string& interfaceName, FocusAcquiredCallback callback) {
    m_executor->execute([this, interfaceName, callback] {
        /// Clear any existing acquireFocus request information.
        m_focusAcquirePtr.reset();
        if (callback) {
            m_focusAcquirePtr = std::make_shared<FocusAcquireInterfaceAndCallback>(std::move(interfaceName), callback);
        } else {
            ACSDK_WARN(LX("No callback registered for Focus acquisition"));
        }
        executeAcquireWindow();
    });
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK