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

#include "acsdk/PresentationOrchestratorClient/private/PresentationOrchestratorClient.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace presentationOrchestratorClient {

using namespace presentationOrchestratorInterfaces;
using namespace alexaClientSDK::avsCommon::utils;

/// String to identify log entries originating from this file.
#define TAG "PresentationOrchestratorClient"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

std::shared_ptr<PresentationOrchestratorClient> PresentationOrchestratorClient::create(
    const std::string& clientId,
    std::shared_ptr<PresentationOrchestratorStateTrackerInterface> stateTracker,
    std::shared_ptr<VisualTimeoutManagerInterface> visualTimeoutManager) {
    if (!stateTracker) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullStateTracker"));
        return nullptr;
    }

    if (!visualTimeoutManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullVisualTimeoutManager"));
        return nullptr;
    }

    auto client = std::shared_ptr<PresentationOrchestratorClient>(
        new PresentationOrchestratorClient(clientId, stateTracker, visualTimeoutManager));

    stateTracker->addWindowObserver(client);

    return client;
}

PresentationOrchestratorClient::PresentationOrchestratorClient(
    const std::string& clientId,
    std::shared_ptr<PresentationOrchestratorStateTrackerInterface> stateTracker,
    std::shared_ptr<VisualTimeoutManagerInterface> visualTimeoutManager) :
        RequiresShutdown{TAG},
        m_requestTokenCounter{0},
        m_stateTracker{std::move(stateTracker)},
        m_visualTimeoutManager{std::move(visualTimeoutManager)},
        m_lifespanToTimeoutMapper{PresentationLifespanToTimeoutMapper::create()},
        m_clientId{clientId} {
    m_executor = std::make_shared<threading::Executor>();
}

PresentationRequestToken PresentationOrchestratorClient::requestWindow(
    const std::string& windowId,
    PresentationOptions options,
    std::shared_ptr<PresentationObserverInterface> observer) {
    ACSDK_DEBUG5(LX(__func__));

    const PresentationRequestToken requestToken = m_requestTokenCounter++;

    m_executor->submit([this, requestToken, windowId, options, observer]() {
        executeRequestWindow(requestToken, windowId, options, observer);
    });

    return requestToken;
}

void PresentationOrchestratorClient::clearPresentations() {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this]() {
        ACSDK_DEBUG5(LX("executeClearPresentations"));
        for (auto const& windowManagerIt : m_windowIdToManager) {
            windowManagerIt.second->clearPresentations();
        }
    });
}

bool PresentationOrchestratorClient::navigateBack() {
    ACSDK_DEBUG5(LX(__func__));
    return m_executor
        ->submit([this]() {
            ACSDK_DEBUG5(LX("executeNavigateBack"));
            if (auto windowManager = executeGetFocusedWindowManager()) {
                return windowManager->navigateBack();
            }

            return false;
        })
        .get();
}

void PresentationOrchestratorClient::onWindowAdded(const PresentationOrchestratorWindowInstance& windowInstance) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this, windowInstance]() {
        ACSDK_DEBUG5(LX("executeOnWindowAdded").d("windowId", windowInstance.id));
        auto windowManager = WindowManager::create(
            m_clientId,
            windowInstance,
            m_stateTracker,
            m_visualTimeoutManager,
            shared_from_this(),
            m_lifespanToTimeoutMapper);
        m_windowIdToManager.insert({windowInstance.id, windowManager});
    });
}

void PresentationOrchestratorClient::onWindowModified(const PresentationOrchestratorWindowInstance& windowInstance) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this, windowInstance]() { executeOnWindowModified(windowInstance); });
}

void PresentationOrchestratorClient::onWindowRemoved(const std::string& windowId) {
    ACSDK_DEBUG5(LX(__func__).d("windowId", windowId));

    m_executor->submit([this, windowId]() { executeOnWindowRemoved(windowId); });
}

void PresentationOrchestratorClient::prepareToForegroundWindow(
    const PresentationOrchestratorWindowInstance& windowInstance,
    ForegroundWindowCallback foregroundWindowCallback) {
    m_executor->submit([this, windowInstance, foregroundWindowCallback]() {
        executePrepareToForegroundWindow(windowInstance, foregroundWindowCallback);
    });
}

void PresentationOrchestratorClient::doShutdown() {
    m_executor->shutdown();
    auto windowManagerIt = m_windowIdToManager.begin();
    while (windowManagerIt != m_windowIdToManager.end()) {
        windowManagerIt->second->shutdown();
        windowManagerIt++;
    }
    m_windowIdToManager.clear();
}

void PresentationOrchestratorClient::setExecutor(const std::shared_ptr<threading::Executor>& executor) {
    ACSDK_WARN(LX(__func__).d("reason", "should only be called in tests"));
    m_executor = executor;
}

void PresentationOrchestratorClient::executeRequestWindow(
    const PresentationRequestToken& requestToken,
    const std::string& windowId,
    PresentationOptions options,
    std::shared_ptr<PresentationObserverInterface> observer) {
    ACSDK_DEBUG5(LX(__func__).d("windowId", windowId));

    auto windowManagerIt = m_windowIdToManager.find(windowId);
    if (windowManagerIt == m_windowIdToManager.end()) {
        ACSDK_ERROR(LX("executeRequestWindowFailed")
                        .d("reason", "Window Manager does not exist for specified window Id")
                        .d("windowId", windowId));
        return;
    }

    auto windowInstance = windowManagerIt->second->getWindowInstance();
    auto supportedInterfaces = windowInstance.supportedInterfaces;
    auto findIt = std::find(supportedInterfaces.begin(), supportedInterfaces.end(), options.interfaceName);
    if (findIt == supportedInterfaces.end()) {
        ACSDK_ERROR(LX("executeRequestWindowFailed")
                        .d("reason", "Interface not supported in specified window")
                        .d("windowId", windowId)
                        .d("interface", options.interfaceName));
        return;
    }

    windowManagerIt->second->acquire(requestToken, options, observer);
}

void PresentationOrchestratorClient::executePrepareToForegroundWindow(
    const PresentationOrchestratorWindowInstance& windowInstanceToForeground,
    ForegroundWindowCallback foregroundWindowCallback) {
    ACSDK_DEBUG5(LX(__func__));

    for (auto windowManagerIt = m_windowIdToManager.begin(); windowManagerIt != m_windowIdToManager.end();
         windowManagerIt++) {
        if (windowManagerIt->first == windowInstanceToForeground.id) {
            // ensure that no blocking calls are made on the window being foregrounded leading to a cycle
            continue;
        }

        auto windowManager = windowManagerIt->second;
        auto windowInstance = windowManager->getWindowInstance();
        // presentations in windows with higher zOrderIndex must be cleared before foregrounding a window
        if (windowInstance.zOrderIndex > windowInstanceToForeground.zOrderIndex) {
            windowManager->clearPresentations();
            continue;
        }

        // unfocus any previously foreground focused window
        if (windowManager->isForegroundFocused()) {
            windowManager->unfocus();
            continue;
        }
    }

    foregroundWindowCallback();
}

void PresentationOrchestratorClient::executeOnWindowModified(
    const PresentationOrchestratorWindowInstance& windowInstance) {
    ACSDK_DEBUG5(LX(__func__).d("windowId", windowInstance.id));

    auto windowManagerIt = m_windowIdToManager.find(windowInstance.id);
    if (windowManagerIt == m_windowIdToManager.end()) {
        ACSDK_ERROR(LX("executeOnWindowModifiedFailed")
                        .d("reason", "Window Manager does not exist")
                        .d("windowId", windowInstance.id));
        return;
    }

    windowManagerIt->second->setWindowInstance(windowInstance);

    if (auto windowManager = executeGetFocusedWindowManager()) {
        if (windowManager->isForegroundFocused()) {
            ACSDK_DEBUG7(LX("executeOnWindowModified").d("reason", "Window already in foreground"));
            return;
        }

        windowManager->foregroundWindow();
    }
}

void PresentationOrchestratorClient::executeOnWindowRemoved(const std::string& windowId) {
    ACSDK_DEBUG5(LX(__func__).d("windowId", windowId));

    auto windowManagerIt = m_windowIdToManager.find(windowId);
    if (windowManagerIt == m_windowIdToManager.end()) {
        ACSDK_ERROR(
            LX("executeOnWindowRemovedFailed").d("reason", "Window Manager does not exist").d("windowId", windowId));
        return;
    }

    windowManagerIt->second->shutdown();
    m_windowIdToManager.erase(windowId);

    if (auto windowManager = executeGetFocusedWindowManager()) {
        windowManager->foregroundWindow();
    }
}

void PresentationOrchestratorClient::updateForegroundWindow() {
    m_executor->submit([this]() {
        ACSDK_DEBUG5(LX("updateForegroundWindow"));
        if (auto windowManager = executeGetFocusedWindowManager()) {
            windowManager->foregroundWindow();
        }
    });
}

std::shared_ptr<WindowManager> PresentationOrchestratorClient::executeGetFocusedWindowManager() {
    auto focusedWindowId = m_stateTracker->getFocusedWindowId();
    if (focusedWindowId.empty()) {
        ACSDK_DEBUG5(LX(__func__).m("No window in focus"));
        return nullptr;
    }

    auto focusedWindowManagerIt = m_windowIdToManager.find(focusedWindowId);
    if (focusedWindowManagerIt == m_windowIdToManager.end()) {
        ACSDK_DEBUG5(LX(__func__).d("reason", "Window Manager does not exist").d("windowId", focusedWindowId));
        return nullptr;
    }

    return focusedWindowManagerIt->second;
}

}  // namespace presentationOrchestratorClient
}  // namespace alexaClientSDK
