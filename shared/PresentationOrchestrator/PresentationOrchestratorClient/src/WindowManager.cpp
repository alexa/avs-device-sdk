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

#include "acsdk/PresentationOrchestratorClient/private/WindowManager.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace presentationOrchestratorClient {

using namespace presentationOrchestratorInterfaces;
using namespace alexaClientSDK::avsCommon::utils;

/// String to identify log entries originating from this file.
#define TAG "WindowManager"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

std::shared_ptr<WindowManager> WindowManager::create(
    const std::string& clientId,
    const PresentationOrchestratorWindowInstance& windowInstance,
    std::shared_ptr<PresentationOrchestratorStateTrackerInterface> stateTracker,
    std::shared_ptr<VisualTimeoutManagerInterface> visualTimeoutManager,
    std::shared_ptr<MultiWindowManagerInterface> multiWindowManager,
    std::shared_ptr<PresentationLifespanToTimeoutMapper> lifespanToTimeoutMapper) {
    if (!stateTracker) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullStateTracker"));
        return nullptr;
    }

    if (!visualTimeoutManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullVisualTimeoutManager"));
        return nullptr;
    }

    if (!multiWindowManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMultiWindowManager"));
        return nullptr;
    }

    if (!lifespanToTimeoutMapper) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullLifespanToTimeoutMapper"));
        return nullptr;
    }

    return std::shared_ptr<WindowManager>(new WindowManager(
        clientId, windowInstance, stateTracker, visualTimeoutManager, multiWindowManager, lifespanToTimeoutMapper));
}

WindowManager::WindowManager(
    const std::string& clientId,
    const PresentationOrchestratorWindowInstance& windowInstance,
    std::shared_ptr<PresentationOrchestratorStateTrackerInterface> stateTracker,
    std::shared_ptr<VisualTimeoutManagerInterface> visualTimeoutManager,
    std::shared_ptr<MultiWindowManagerInterface> multiWindowManager,
    std::shared_ptr<PresentationLifespanToTimeoutMapper> lifespanToTimeoutMapper) :
        RequiresShutdown{TAG},
        m_stateTracker{std::move(stateTracker)},
        m_clientId{clientId},
        m_windowInstance{windowInstance},
        m_visualTimeoutManager{std::move(visualTimeoutManager)},
        m_multiWindowManager{std::move(multiWindowManager)},
        m_lifespanToTimeoutMapper{std::move(lifespanToTimeoutMapper)} {
    m_executor = std::make_shared<threading::Executor>();
}

PresentationMetadata getPresentationMetadata(const std::string& interfaceName, const std::string& metadata) {
    PresentationMetadata presentationMetadata;
    presentationMetadata.interfaceName = interfaceName;
    presentationMetadata.metadata = metadata;
    return presentationMetadata;
}

void WindowManager::acquire(
    const PresentationRequestToken& requestToken,
    PresentationOptions options,
    std::shared_ptr<PresentationObserverInterface> observer) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this, requestToken, options, observer]() {
        if (executeIsForegroundFocused()) {
            executeAcquire(requestToken, options, observer);
            return;
        }

        m_multiWindowManager->prepareToForegroundWindow(m_windowInstance, [this, requestToken, options, observer]() {
            executeAcquire(requestToken, options, observer);
        });
    });
}

void WindowManager::dismissPresentation(std::shared_ptr<Presentation> presentation, bool isSelfDismiss) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit(
        [this, presentation, isSelfDismiss]() { executeDismissPresentation(presentation, isSelfDismiss); });
}

void WindowManager::foregroundPresentation(std::shared_ptr<Presentation> presentation) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this, presentation]() {
        if (executeIsForegroundFocused()) {
            executeForegroundPresentation(presentation);
            return;
        }

        m_multiWindowManager->prepareToForegroundWindow(m_windowInstance, [this, presentation]() {
            m_executor->submit([this, presentation]() { executeForegroundPresentation(presentation); });
        });
    });
}

void WindowManager::foregroundWindow() {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this]() { executeForegroundWindow(); });
}

bool WindowManager::navigateBack() {
    ACSDK_DEBUG5(LX(__func__));
    return m_executor->submit([this]() { return executeNavigateBack(); }).get();
}

void WindowManager::onPresentationMetadataUpdate(std::shared_ptr<Presentation> presentation) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this, presentation]() { executeOnPresentationMetadataUpdate(presentation); });
}

void WindowManager::onPresentationLifespanUpdate(std::shared_ptr<Presentation> presentation) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this, presentation]() { executeOnPresentationLifespanUpdate(presentation); });
}

void WindowManager::setWindowInstance(const PresentationOrchestratorWindowInstance& windowInstance) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this, windowInstance]() { m_windowInstance = windowInstance; });
}

PresentationOrchestratorWindowInstance WindowManager::getWindowInstance() {
    ACSDK_DEBUG5(LX(__func__));
    return m_executor->submit([this]() { return m_windowInstance; }).get();
}

std::chrono::milliseconds WindowManager::getTimeoutDuration(const PresentationLifespan& lifespan) {
    ACSDK_DEBUG5(LX(__func__));
    return m_lifespanToTimeoutMapper->getTimeoutDuration(lifespan);
}

void WindowManager::clearPresentations() {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this]() { executeClearPresentations(); }).get();
}

bool WindowManager::isForegroundFocused() {
    ACSDK_DEBUG5(LX(__func__));
    return m_executor->submit([this]() { return executeIsForegroundFocused(); }).get();
}

void WindowManager::unfocus() {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this]() { executeUnfocus(); }).get();
}

void WindowManager::doShutdown() {
    clearPresentations();
    m_executor->shutdown();
}

void WindowManager::executeTopPresentationStateChange(const PresentationLifespan& nextPresentationLifespan) {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_presentationStack.top().hasValue()) {
        ACSDK_ERROR(LX("executeTopPresentationStateChangeFailed").d("reason", "Presentation stack is empty"));
        return;
    }

    auto topPresentation = m_presentationStack.top().value();
    auto topPresentationLifespan = topPresentation->getLifespan();

    switch (topPresentationLifespan) {
        case PresentationLifespan::TRANSIENT:
            m_presentationStack.pop();
            topPresentation->setState(PresentationState::NONE);
            break;
        case PresentationLifespan::SHORT:
            if (nextPresentationLifespan == PresentationLifespan::TRANSIENT) {
                // A short lived presentation can be backgrounded if a transient presentation is displayed
                topPresentation->setState(PresentationState::BACKGROUND);
            } else {
                m_presentationStack.pop();
                topPresentation->setState(PresentationState::NONE);
            }
            break;
        case PresentationLifespan::LONG:
        case PresentationLifespan::PERMANENT:
            topPresentation->setState(PresentationState::BACKGROUND);
            break;
    }
}

void WindowManager::executeAcquire(
    const PresentationRequestToken& requestToken,
    PresentationOptions options,
    std::shared_ptr<PresentationObserverInterface> observer) {
    ACSDK_DEBUG5(LX(__func__).d("windowId", m_windowInstance.id).d("presentationRequestToken", requestToken));

    PresentationMetadata presentationMetadata = getPresentationMetadata(options.interfaceName, options.metadata);
    if (!executeIsForegroundFocused()) {
        m_stateTracker->acquireWindow(m_clientId, m_windowInstance.id, presentationMetadata);
    } else {
        m_stateTracker->updatePresentationMetadata(m_clientId, m_windowInstance.id, presentationMetadata);
        executeTopPresentationStateChange(options.presentationLifespan);
    }

    auto nextPresentation = Presentation::create(
        observer, options, requestToken, PresentationState::FOREGROUND, m_visualTimeoutManager, shared_from_this());
    m_presentationStack.push(nextPresentation);
    nextPresentation->startTimeout();
    observer->onPresentationAvailable(requestToken, nextPresentation);
}

void WindowManager::executeDismissPresentation(std::shared_ptr<Presentation> presentation, bool isSelfDismiss) {
    ACSDK_DEBUG5(
        LX(__func__).d("windowId", m_windowInstance.id).d("presentationRequestToken", presentation->getRequestToken()));

    if (presentation->getState() == PresentationState::NONE) {
        ACSDK_WARN(LX("executeDismissPresentationFailed")
                       .d("reason", "Presentation has already been dismissed")
                       .d("presentationRequestToken", presentation->getRequestToken())
                       .d("windowId", m_windowInstance.id));
        return;
    }

    if (!m_presentationStack.contains(presentation)) {
        ACSDK_ERROR(LX("executeDismissPresentationFailed")
                        .d("reason", "Presentation absent from the stack")
                        .d("windowId", m_windowInstance.id)
                        .d("presentationRequestToken", presentation->getRequestToken()));
        return;
    }

    if (presentation->getLifespan() == PresentationLifespan::PERMANENT && !isSelfDismiss) {
        ACSDK_ERROR(LX("executeDismissPresentationFailed")
                        .d("reason", "Presentation with lifespan PERMANENT cannot be dismissed")
                        .d("presentationRequestToken", presentation->getRequestToken())
                        .d("windowId", m_windowInstance.id));
        return;
    }

    PresentationState prevState = presentation->getState();
    // remove presentation from the stack, not necessarily the top
    m_presentationStack.erase(presentation);

    if (!m_presentationStack.top().hasValue()) {
        presentation->setState(PresentationState::NONE);
        // window must release focus if all presentations in stack are dismissed
        m_stateTracker->releaseWindow(m_clientId, m_windowInstance.id);
        m_multiWindowManager->updateForegroundWindow();
        return;
    }

    presentation->setState(PresentationState::NONE);
    if (prevState == PresentationState::FOREGROUND || prevState == PresentationState::FOREGROUND_UNFOCUSED) {
        // top must be foregrounded if the dismissed presentation was in foreground
        m_presentationStack.top().value()->setState(prevState);
    }
}

void WindowManager::executeForegroundPresentation(std::shared_ptr<Presentation> presentation) {
    ACSDK_DEBUG5(
        LX(__func__).d("windowId", m_windowInstance.id).d("presentationRequestToken", presentation->getRequestToken()));

    if (presentation->getState() == PresentationState::FOREGROUND) {
        ACSDK_WARN(LX("executeForegroundPresentationFailed")
                       .d("reason", "Presentation already in foreground")
                       .d("windowId", m_windowInstance.id)
                       .d("presentationRequestToken", presentation->getRequestToken()));
        return;
    }

    if (!m_presentationStack.top().hasValue()) {
        ACSDK_ERROR(LX("executeForegroundPresentationFailed")
                        .d("reason", "Presentation stack is empty")
                        .d("windowId", m_windowInstance.id));
        return;
    }

    if (!m_presentationStack.contains(presentation)) {
        ACSDK_ERROR(LX("executeForegroundPresentationFailed")
                        .d("reason", "Presentation absent from the stack")
                        .d("windowId", m_windowInstance.id)
                        .d("presentationRequestToken", presentation->getRequestToken()));
        return;
    }

    auto topPresentation = m_presentationStack.top().value();
    auto topPresentationLifespan = presentation->getLifespan();

    executeTopPresentationStateChange(topPresentationLifespan);
    // move presentation to be foregrounded on top of stack
    m_presentationStack.moveToTop(presentation);

    auto interfaceName = presentation->getInterface();
    auto metadata = presentation->getMetadata();
    auto presentationMetadata = getPresentationMetadata(interfaceName, metadata);
    if (executeIsForegroundFocused()) {
        m_stateTracker->updatePresentationMetadata(m_clientId, m_windowInstance.id, presentationMetadata);
    } else {
        m_stateTracker->acquireWindow(m_clientId, m_windowInstance.id, presentationMetadata);
    }
    presentation->setState(PresentationState::FOREGROUND);
}

void WindowManager::executeForegroundWindow() {
    ACSDK_DEBUG5(LX(__func__).d("windowId", m_windowInstance.id));

    if (!m_presentationStack.top().hasValue()) {
        ACSDK_WARN(LX("executeForegroundWindowFailed").d("reason", "Window in FOREGROUND without any presentation"));
        return;
    }

    auto topPresentation = m_presentationStack.top().value();
    topPresentation->setState(PresentationState::FOREGROUND);
}

bool WindowManager::executeNavigateBack() {
    ACSDK_DEBUG5(LX(__func__).d("windowId", m_windowInstance.id));

    if (!m_presentationStack.top().hasValue()) {
        ACSDK_ERROR(LX("executeNavigateBackFailed").d("reason", "Presentation stack is empty"));
        return false;
    }

    auto topPresentation = m_presentationStack.top().value();
    auto topPresentationState = topPresentation->getState();
    auto topPresentationLifespan = topPresentation->getLifespan();

    // check that the top presentation is in the foreground
    if (topPresentationState != PresentationState::FOREGROUND) {
        ACSDK_ERROR(LX("executeNavigateBackFailed")
                        .d("reason", "Presentation on top of stack is not in the Foreground")
                        .d("presentationRequestToken", topPresentation->getRequestToken())
                        .d("presentationLifespan", topPresentationLifespan));
        return false;
    }

    if (topPresentation->navigateBack()) {
        // skip the default navigation behavior
        return false;
    }

    if (topPresentationLifespan == PresentationLifespan::PERMANENT) {
        ACSDK_DEBUG3(LX("executeNavigateBackFailed")
                         .d("reason", "Presentation on top of stack cannot be dismissed")
                         .d("presentationRequestToken", topPresentation->getRequestToken())
                         .d("presentationLifespan", topPresentationLifespan));
        return false;
    }

    // remove top presentation
    m_presentationStack.pop();
    topPresentation->setState(PresentationState::NONE);

    if (!m_presentationStack.top().hasValue()) {
        // window must release focus if all presentations in stack are dismissed
        m_stateTracker->releaseWindow(m_clientId, m_windowInstance.id);
        m_multiWindowManager->updateForegroundWindow();
    } else {
        m_presentationStack.top().value()->setState(topPresentationState);
    }
    return true;
}

void WindowManager::executeOnPresentationLifespanUpdate(std::shared_ptr<Presentation> presentation) {
    ACSDK_DEBUG5(
        LX(__func__).d("windowId", m_windowInstance.id).d("presentationRequestToken", presentation->getRequestToken()));

    auto presentationState = presentation->getState();
    auto presentationLifespan = presentation->getLifespan();

    if (presentationState == PresentationState::FOREGROUND ||
        presentationState == PresentationState::FOREGROUND_UNFOCUSED) {
        // nothing to do if the presentation is in foreground
        return;
    }

    if (presentationState == PresentationState::NONE) {
        ACSDK_WARN(LX("executeOnPresentationLifespanUpdateFailed")
                       .d("reason", "A dismissed presentation present in stack")
                       .d("presentationRequestToken", presentation->getRequestToken()));
        return;
    }

    if (presentationLifespan == PresentationLifespan::TRANSIENT) {
        // A transient presentation cannot be backgrounded
        presentation->setState(PresentationState::NONE);
        m_presentationStack.erase(presentation);
        return;
    }

    if (presentationLifespan == PresentationLifespan::SHORT) {
        if (!m_presentationStack.above(presentation).hasValue()) {
            ACSDK_WARN(LX("executeOnPresentationLifespanUpdateFailed")
                           .d("reason", "Backgrounded presentation must have a presentation above it in stack")
                           .d("presentationRequestToken", presentation->getRequestToken()));
            return;
        }

        auto presentationAbove = m_presentationStack.above(presentation).value();
        auto presentationAboveLifespan = presentationAbove->getLifespan();

        // short presentation should only be in background if transient presentation is on top
        if (presentationAboveLifespan != PresentationLifespan::TRANSIENT) {
            presentation->setState(PresentationState::NONE);
            m_presentationStack.erase(presentation);
            return;
        }
    }
}

void WindowManager::executeOnPresentationMetadataUpdate(std::shared_ptr<Presentation> presentation) {
    ACSDK_DEBUG5(LX(__func__).d("presentationRequestToken", presentation->getRequestToken()));
    auto presentationState = presentation->getState();
    if (presentationState == PresentationState::FOREGROUND ||
        presentationState == PresentationState::FOREGROUND_UNFOCUSED) {
        auto interfaceName = presentation->getInterface();
        auto metadata = presentation->getMetadata();
        auto presentationMetadata = getPresentationMetadata(interfaceName, metadata);
        m_stateTracker->updatePresentationMetadata(m_clientId, m_windowInstance.id, presentationMetadata);
    }
}

bool WindowManager::executeIsForegroundFocused() {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_presentationStack.top().hasValue()) {
        ACSDK_DEBUG5(LX("executeIsForegroundFocusedFailed").d("reason", "Presentation stack is empty"));
        return false;
    }

    auto topPresentation = m_presentationStack.top().value();
    auto topPresentationState = topPresentation->getState();
    return topPresentationState == PresentationState::FOREGROUND;
}

void WindowManager::executeClearPresentations() {
    ACSDK_DEBUG5(LX(__func__).d("windowId", m_windowInstance.id));

    if (!m_presentationStack.top().hasValue()) {
        ACSDK_DEBUG9(LX("executeClearPresentations")
                         .d("reason", "Presentation stack is empty")
                         .d("windowId", m_windowInstance.id));
        return;
    }

    // Dismiss all presentations without checking for visual focus behavior
    while (m_presentationStack.top().hasValue()) {
        m_presentationStack.top().value()->setState(PresentationState::NONE);
        m_presentationStack.pop();
    }

    // Always release the window when clearing presentations
    m_stateTracker->releaseWindow(m_clientId, m_windowInstance.id);
}

void WindowManager::executeUnfocus() {
    ACSDK_DEBUG5(LX(__func__).d("windowId", m_windowInstance.id));

    if (!executeIsForegroundFocused()) {
        ACSDK_ERROR(
            LX("executeUnfocusFailed").d("reason", "Window not foreground focused").d("windowId", m_windowInstance.id));
        return;
    }

    auto topPresentation = m_presentationStack.top().value();
    topPresentation->setState(PresentationState::FOREGROUND_UNFOCUSED);
}

}  // namespace presentationOrchestratorClient
}  // namespace alexaClientSDK
