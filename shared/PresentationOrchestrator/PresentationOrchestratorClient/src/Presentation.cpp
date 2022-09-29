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

#include "acsdk/PresentationOrchestratorClient/private/Presentation.h"

#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdk/PresentationOrchestratorClient/private/WindowManager.h"

namespace alexaClientSDK {
namespace presentationOrchestratorClient {

using namespace presentationOrchestratorInterfaces;
using namespace alexaClientSDK::avsCommon::utils;

/// String to identify log entries originating from this file.
#define TAG "Presentation"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

std::shared_ptr<Presentation> Presentation::create(
    std::shared_ptr<PresentationObserverInterface> observer,
    PresentationOptions options,
    PresentationRequestToken requestToken,
    PresentationState state,
    std::shared_ptr<VisualTimeoutManagerInterface> visualTimeoutManager,
    std::shared_ptr<WindowManager> windowManager) {
    if (!observer) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullPresentationObserver"));
        return nullptr;
    }

    if (!visualTimeoutManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullVisualTimeoutManager"));
        return nullptr;
    }

    if (!windowManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullWindowManager"));
        return nullptr;
    }

    auto customTimeout = options.timeout;
    if (!validateTimeout(options.timeout) || options.timeout == PresentationInterface::getTimeoutDefault()) {
        ACSDK_DEBUG5(LX(__func__)
                         .d("reason", "Defaulting to timeout associated with lifespan")
                         .d("presentationRequestToken", requestToken)
                         .d("timeout", options.timeout.count()));
        options.timeout = windowManager->getTimeoutDuration(options.presentationLifespan);
        customTimeout = PresentationInterface::getTimeoutDefault();
    } else if (options.presentationLifespan == PresentationLifespan::PERMANENT) {
        auto defaultTimeout = windowManager->getTimeoutDuration(PresentationLifespan::PERMANENT);
        if (defaultTimeout != options.timeout) {
            ACSDK_WARN(
                LX(__func__)
                    .d("reason", "Presentations with PERMANENT lifespan cannot have custom timeout. Defaulting timeout")
                    .d("presentationRequestToken", requestToken));
            options.timeout = defaultTimeout;
        }
    }

    return std::shared_ptr<Presentation>(new Presentation(
        std::move(observer),
        std::move(options),
        std::move(requestToken),
        std::move(state),
        std::move(visualTimeoutManager),
        std::move(windowManager),
        std::move(customTimeout)));
}

Presentation::Presentation(
    std::shared_ptr<PresentationObserverInterface> observer,
    PresentationOptions options,
    PresentationRequestToken requestToken,
    PresentationState state,
    std::shared_ptr<VisualTimeoutManagerInterface> visualTimeoutManager,
    std::shared_ptr<WindowManager> windowManager,
    std::chrono::milliseconds customTimeout) :
        m_observer(std::move(observer)),
        m_options(std::move(options)),
        m_state(std::move(state)),
        m_requestToken(std::move(requestToken)),
        m_visualTimeoutManager(std::move(visualTimeoutManager)),
        m_windowManager(std::move(windowManager)),
        m_customTimeout(customTimeout) {
}

void Presentation::dismiss() {
    ACSDK_DEBUG5(LX(__func__));
    m_windowManager->dismissPresentation(shared_from_this(), true);
}

void Presentation::foreground() {
    ACSDK_DEBUG5(LX(__func__));
    m_windowManager->foregroundPresentation(shared_from_this());
}

void Presentation::setMetadata(const std::string& metadata) {
    ACSDK_DEBUG5(LX(__func__));
    const std::lock_guard<std::mutex> lock(m_optionsMutex);
    m_options.metadata = metadata;
    m_windowManager->onPresentationMetadataUpdate(shared_from_this());
}

void Presentation::setLifespan(const presentationOrchestratorInterfaces::PresentationLifespan& newLifespan) {
    ACSDK_DEBUG5(LX(__func__));
    const std::lock_guard<std::mutex> lock(m_optionsMutex);
    m_options.presentationLifespan = newLifespan;

    setTimeoutLocked(m_customTimeout);

    m_windowManager->onPresentationLifespanUpdate(shared_from_this());
}

void Presentation::startTimeout() {
    ACSDK_DEBUG5(LX(__func__));
    const std::lock_guard<std::mutex> lock(m_optionsMutex);
    startTimeoutLocked();
}

void Presentation::startTimeoutLocked() {
    ACSDK_DEBUG5(LX(__func__));
    if (m_options.timeout == PresentationInterface::getTimeoutDisabled()) {
        ACSDK_DEBUG3(LX("startTimeoutLockedFailed")
                         .d("reason", "Timeout is disabled")
                         .d("presentationRequestToken", m_requestToken));
        return;
    }

    if (m_state != PresentationState::FOREGROUND) {
        ACSDK_DEBUG3(LX("startTimeoutLockedFailed")
                         .d("reason", "Presentation not in FOREGROUND state")
                         .d("presentationRequestToken", m_requestToken)
                         .d("presentationState", m_state));
        return;
    }

    m_lastTimeoutId = m_visualTimeoutManager->requestTimeout(
        m_options.timeout, [this]() { m_windowManager->dismissPresentation(shared_from_this()); });
}

void Presentation::stopTimeout() {
    ACSDK_DEBUG5(LX(__func__));

    m_visualTimeoutManager->stopTimeout(m_lastTimeoutId);
}

void Presentation::setTimeout(const std::chrono::milliseconds& timeout) {
    ACSDK_DEBUG5(LX(__func__));
    const std::lock_guard<std::mutex> lock(m_optionsMutex);
    setTimeoutLocked(timeout);
}

void Presentation::setTimeoutLocked(const std::chrono::milliseconds& timeout) {
    ACSDK_DEBUG5(LX(__func__));
    if (!validateTimeout(timeout)) {
        return;
    }

    if (timeout == PresentationInterface::getTimeoutDefault()) {
        ACSDK_DEBUG5(LX(__func__)
                         .d("reason", "Defaulting to timeout associated with lifespan")
                         .d("presentationRequestToken", m_requestToken));
        m_options.timeout = m_windowManager->getTimeoutDuration(m_options.presentationLifespan);
        if (m_options.timeout == PresentationInterface::getTimeoutDisabled()) {
            ACSDK_DEBUG5(LX(__func__).d("reason", "Timeout disabled").d("presentationRequestToken", m_requestToken));
            stopTimeout();
        }
        return;
    }

    if (m_options.presentationLifespan == PresentationLifespan::PERMANENT) {
        ACSDK_WARN(LX(__func__)
                       .d("reason", "Presentations with PERMANENT lifespan cannot have custom timeout")
                       .d("presentationRequestToken", m_requestToken));
        return;
    }

    m_options.timeout = timeout;
    if (timeout == PresentationInterface::getTimeoutDisabled()) {
        ACSDK_DEBUG5(LX(__func__).d("reason", "Timeout disabled").d("presentationRequestToken", m_requestToken));
        stopTimeout();
    }
}

bool Presentation::validateTimeout(const std::chrono::milliseconds& timeout) {
    if (timeout.count() <= 0 && timeout != PresentationInterface::getTimeoutDisabled() &&
        timeout != PresentationInterface::getTimeoutDefault()) {
        ACSDK_WARN(LX(__func__).d("reason", "Invalid timeout value").d("timeout", timeout.count()));
        return false;
    }

    return true;
}

PresentationState Presentation::getState() {
    ACSDK_DEBUG5(LX(__func__));
    return m_state;
}

PresentationRequestToken Presentation::getRequestToken() const {
    ACSDK_DEBUG5(LX(__func__));
    return m_requestToken;
}

PresentationLifespan Presentation::getLifespan() const {
    ACSDK_DEBUG5(LX(__func__));
    const std::lock_guard<std::mutex> lock(m_optionsMutex);

    return m_options.presentationLifespan;
}

std::string Presentation::getMetadata() const {
    ACSDK_DEBUG5(LX(__func__));
    const std::lock_guard<std::mutex> lock(m_optionsMutex);

    return m_options.metadata;
}

std::string Presentation::getInterface() const {
    ACSDK_DEBUG5(LX(__func__));
    const std::lock_guard<std::mutex> lock(m_optionsMutex);

    return m_options.interfaceName;
}

void Presentation::setState(PresentationState newState) {
    ACSDK_DEBUG5(LX(__func__).d("requestToken", m_requestToken).d("currentState", m_state).d("newState", newState));
    const std::lock_guard<std::mutex> lock(m_optionsMutex);

    if (newState != m_state) {
        PresentationState oldState = m_state;
        m_state = newState;

        // restart timeout if presentation state changes to foreground
        if (newState == PresentationState::FOREGROUND &&
            m_options.timeout != PresentationInterface::getTimeoutDisabled()) {
            startTimeoutLocked();
        }

        // stop timeout if presentation state changes to any state other than FOREGROUND
        // only a foregrounded presentation should have an active timeout
        if (oldState == PresentationState::FOREGROUND) {
            stopTimeout();
        }
        m_observer->onPresentationStateChanged(m_requestToken, m_state);
    }
}

bool Presentation::navigateBack() {
    ACSDK_DEBUG5(LX(__func__).d("requestToken", m_requestToken));
    return m_observer->onNavigateBack(m_requestToken);
}

}  // namespace presentationOrchestratorClient
}  // namespace alexaClientSDK
