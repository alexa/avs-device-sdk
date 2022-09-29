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
#include "acsdk/PresentationOrchestratorStateTracker/private/PresentationOrchestratorStateTracker.h"

#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace presentationOrchestratorStateTracker {

static const std::string TAG{"PresentationOrchestratorStateTracker"};
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

using presentationOrchestratorInterfaces::PresentationOrchestratorStateObserverInterface;
using presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface;
using presentationOrchestratorInterfaces::PresentationOrchestratorWindowInfo;
using presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance;
using presentationOrchestratorInterfaces::PresentationOrchestratorWindowObserverInterface;

std::shared_ptr<PresentationOrchestratorStateTracker> PresentationOrchestratorStateTracker::create(
    const std::shared_ptr<alexaClientSDK::afml::ActivityTrackerInterface>& visualActivityTracker) {
    if (!visualActivityTracker) {
        ACSDK_ERROR(LX("createFailed").m("Null VisualActivityTracker"));
        return nullptr;
    }

    return std::shared_ptr<PresentationOrchestratorStateTracker>(
        new PresentationOrchestratorStateTracker(visualActivityTracker));
}

PresentationOrchestratorStateTracker::PresentationOrchestratorStateTracker(
    std::shared_ptr<afml::ActivityTrackerInterface> visualActivityTracker) :
        RequiresShutdown{TAG},
        m_visualActivityTracker{std::move(visualActivityTracker)} {
    m_executor = std::make_shared<avsCommon::utils::threading::Executor>();
}

void PresentationOrchestratorStateTracker::acquireWindow(
    const std::string& clientId,
    const std::string& windowId,
    presentationOrchestratorInterfaces::PresentationMetadata metadata) {
    ACSDK_DEBUG5(LX(__func__).d("clientId", clientId).d("windowId", windowId));
    m_executor->submit([this, clientId, windowId, metadata]() {
        auto windowIt = m_windows.find(windowId);
        if (windowIt == m_windows.end()) {
            ACSDK_ERROR(LX("acquireWindowFailed").d("reason", "Unknown windowId").d("windowId", windowId));
            return;
        }

        auto& window = windowIt->second;

        auto clientIt = window.clientIdToPresentation.find(clientId);
        if (clientIt == window.clientIdToPresentation.end()) {
            window.presentationStack.emplace_front(clientId, metadata);
            window.clientIdToPresentation[clientId] = window.presentationStack.begin();
        } else {
            // Move the element to the front of the stack without invalidating iterators
            window.presentationStack.splice(
                window.presentationStack.begin(), window.presentationStack, clientIt->second);
            clientIt->second->acquiredTime = std::chrono::steady_clock::now();
        }

        executeNotifyStateObservers(windowId, metadata);
        executeUpdateFocusedPresentation(window);
    });
}

void PresentationOrchestratorStateTracker::updatePresentationMetadata(
    const std::string& clientId,
    const std::string& windowId,
    presentationOrchestratorInterfaces::PresentationMetadata metadata) {
    m_executor->submit([this, clientId, windowId, metadata] {
        auto windowIt = m_windows.find(windowId);
        if (windowIt == m_windows.end()) {
            ACSDK_ERROR(LX("updatePresentationMetadataFailed").d("reason", "Unknown windowId").d("windowId", windowId));
            return;
        }

        auto& window = windowIt->second;

        auto clientIt = window.clientIdToPresentation.find(clientId);
        if (clientIt == window.clientIdToPresentation.end()) {
            ACSDK_ERROR(LX("updatePresentationMetadataFailed").d("reason", "Unknown client").d("clientId", clientId));
            return;
        }

        clientIt->second->metadata = metadata;

        if (clientIt->second == window.presentationStack.begin()) {
            executeNotifyStateObservers(windowId, metadata);
        }
    });
}

void PresentationOrchestratorStateTracker::releaseWindow(const std::string& clientId, const std::string& windowId) {
    ACSDK_DEBUG5(LX(__func__).d("clientId", clientId).d("windowId", windowId));
    m_executor->submit([this, clientId, windowId]() {
        auto windowIt = m_windows.find(windowId);
        if (windowIt == m_windows.end()) {
            ACSDK_WARN(LX("releaseWindowFailed").d("reason", "Unknown windowId").d("windowId", windowId));
            return;
        }

        auto& window = windowIt->second;

        auto clientIt = window.clientIdToPresentation.find(clientId);
        if (clientIt == window.clientIdToPresentation.end()) {
            ACSDK_WARN(LX("releaseWindowFailed").d("reason", "Unknown client").d("clientId", clientId));
            return;
        }

        bool wasForeground = window.presentationStack.begin() == clientIt->second;

        window.presentationStack.erase(clientIt->second);
        window.clientIdToPresentation.erase(clientIt);

        if (wasForeground) {
            if (window.presentationStack.empty()) {
                executeNotifyStateObservers(windowId, {});
            } else {
                executeNotifyStateObservers(windowId, window.presentationStack.front().metadata);
            }
            executeUpdateFocusedPresentation(window);
        }
    });
}

void PresentationOrchestratorStateTracker::setDeviceInterface(std::string interfaceName) {
    m_executor->submit([this, interfaceName]() {
        if (m_deviceInterface == interfaceName) {
            return;
        }

        m_deviceInterface = interfaceName;
        if (m_focusedWindowAndPresentation.first.empty()) {
            executeUpdateVisualActivityTracker();
        }
    });
}

void PresentationOrchestratorStateTracker::releaseDeviceInterface() {
    m_executor->submit([this]() {
        if (m_deviceInterface.empty()) {
            return;
        }

        m_deviceInterface.clear();
        if (m_focusedWindowAndPresentation.first.empty()) {
            executeUpdateVisualActivityTracker();
        }
    });
}

std::string PresentationOrchestratorStateTracker::getFocusedInterface() {
    return m_executor->submit([this]() { return executeGetFocusedInterface(); }).get();
}

std::string PresentationOrchestratorStateTracker::getFocusedWindowId() {
    return m_executor->submit([this]() { return executeGetFocusedWindowId(); }).get();
}

void PresentationOrchestratorStateTracker::setWindows(
    const std::vector<PresentationOrchestratorWindowInstance>& windows) {
    ACSDK_DEBUG5(LX(__func__).d("windowCount", windows.size()));
    m_executor->submit([this, windows] { executeSetWindows(windows); });
}

void PresentationOrchestratorStateTracker::executeSetWindows(
    const std::vector<PresentationOrchestratorWindowInstance>& windows) {
    if (m_windows.empty()) {
        for (const auto& window : windows) {
            executeAddWindow(window);
        }
    } else {
        std::vector<PresentationOrchestratorWindowInstance> windowsToAdd;
        std::vector<std::string> windowsIdsToRemove;
        std::vector<std::string> newWindowIds;

        // Identify windows to add or update
        for (const auto& window : windows) {
            newWindowIds.push_back(window.id);
            if (!m_windows.count(window.id)) {
                windowsToAdd.push_back(window);
            } else {
                // Update any existing window
                executeUpdateWindow(window);
            }
        }

        // Identify windows to remove
        for (const auto& existingWindow : m_windows) {
            if (!std::count(newWindowIds.begin(), newWindowIds.end(), existingWindow.first)) {
                windowsIdsToRemove.push_back(existingWindow.first);
            }
        }

        // Remove windows
        for (const auto& windowIdToRemove : windowsIdsToRemove) {
            executeRemoveWindow(windowIdToRemove);
        }

        // Add new windows
        for (const auto& windowToAdd : windowsToAdd) {
            executeAddWindow(windowToAdd);
        }
    }
}

bool PresentationOrchestratorStateTracker::addWindow(const PresentationOrchestratorWindowInstance& window) {
    ACSDK_DEBUG5(LX(__func__).d("windowId", window.id));
    return m_executor->submit([this, window]() { return executeAddWindow(window); }).get();
}

bool PresentationOrchestratorStateTracker::executeAddWindow(const PresentationOrchestratorWindowInstance& window) {
    if (window.id.empty()) {
        ACSDK_ERROR(LX("addWindowFailed").d("reason", "Empty window Id"));
        return false;
    }
    if (m_windows.count(window.id)) {
        ACSDK_ERROR(LX("addWindowFailed").d("reason", "Window Id already exists").d("windowId", window.id));
        return false;
    }

    m_windows.insert({window.id, Window(window)});

    notifier::Notifier<PresentationOrchestratorWindowObserverInterface>::notifyObservers(
        [window](const std::shared_ptr<PresentationOrchestratorWindowObserverInterface>& observer) {
            observer->onWindowAdded(window);
        });

    return true;
}

void PresentationOrchestratorStateTracker::removeWindow(const std::string& windowId) {
    ACSDK_DEBUG5(LX(__func__).d("windowId", windowId));
    m_executor->submit([this, windowId]() { executeRemoveWindow(windowId); });
}

void PresentationOrchestratorStateTracker::executeRemoveWindow(const std::string& windowId) {
    auto windowIt = m_windows.find(windowId);
    if (windowIt == m_windows.end()) {
        ACSDK_ERROR(LX("removeWindowFailed").d("reason", "Window Id does not exist").d("windowId", windowId));
        return;
    }

    m_windows.erase(windowIt);

    // handle the case if window being removed is focused
    if (windowId == m_focusedWindowAndPresentation.first) {
        executeUpdateFocusedPresentation(windowIt->second);
    }

    notifier::Notifier<PresentationOrchestratorWindowObserverInterface>::notifyObservers(
        [windowId](const std::shared_ptr<PresentationOrchestratorWindowObserverInterface>& observer) {
            observer->onWindowRemoved(windowId);
        });
}

void PresentationOrchestratorStateTracker::updateWindow(const PresentationOrchestratorWindowInstance& window) {
    ACSDK_DEBUG5(LX(__func__).d("windowId", window.id));
    m_executor->submit([this, window]() { executeUpdateWindow(window); });
}

void PresentationOrchestratorStateTracker::executeUpdateWindow(const PresentationOrchestratorWindowInstance& window) {
    auto windowIt = m_windows.find(window.id);
    if (windowIt == m_windows.end()) {
        ACSDK_ERROR(LX("updateWindowFailed").d("reason", "Window Id does not exist").d("windowId", window.id));
        return;
    }

    windowIt->second.configuration = window;

    // update focused presentation based on modified window configuration
    // zOrderIndex update, for example, may change focused window
    executeUpdateFocusedPresentation(windowIt->second);

    notifier::Notifier<PresentationOrchestratorWindowObserverInterface>::notifyObservers(
        [window](const std::shared_ptr<PresentationOrchestratorWindowObserverInterface>& observer) {
            observer->onWindowModified(window);
        });
}

void PresentationOrchestratorStateTracker::addWindowObserver(
    std::weak_ptr<PresentationOrchestratorWindowObserverInterface> observer) {
    notifier::Notifier<PresentationOrchestratorWindowObserverInterface>::addWeakPtrObserver(observer);
}

void PresentationOrchestratorStateTracker::removeWindowObserver(
    std::weak_ptr<PresentationOrchestratorWindowObserverInterface> observer) {
    notifier::Notifier<PresentationOrchestratorWindowObserverInterface>::removeWeakPtrObserver(observer);
}

void PresentationOrchestratorStateTracker::addStateObserver(
    std::weak_ptr<PresentationOrchestratorStateObserverInterface> observer) {
    notifier::Notifier<PresentationOrchestratorStateObserverInterface>::addWeakPtrObserver(observer);
}

void PresentationOrchestratorStateTracker::removeStateObserver(
    std::weak_ptr<PresentationOrchestratorStateObserverInterface> observer) {
    notifier::Notifier<PresentationOrchestratorStateObserverInterface>::removeWeakPtrObserver(observer);
}

void PresentationOrchestratorStateTracker::executeNotifyStateObservers(
    const std::string& windowId,
    const presentationOrchestratorInterfaces::PresentationMetadata& metadata) {
    notifier::Notifier<PresentationOrchestratorStateObserverInterface>::notifyObservers(
        [windowId, metadata](const std::shared_ptr<PresentationOrchestratorStateObserverInterface>& observer) {
            observer->onStateChanged(windowId, metadata);
        });
}

void PresentationOrchestratorStateTracker::executeUpdateFocusedPresentation(const Window& changedWindow) {
    ACSDK_DEBUG5(LX(__func__).d("changedWindowId", changedWindow.configuration.id));
    bool shouldUpdateVisualActivityTracker = false;
    if (m_focusedWindowAndPresentation.first.empty()) {
        if (changedWindow.presentationStack.empty()) {
            ACSDK_WARN(LX("updateFocusedPresentationFailed")
                           .d("reason", "Unexpected state change, expected active presentation"));
            return;
        }

        m_focusedWindowAndPresentation.first = changedWindow.configuration.id;
        m_focusedWindowAndPresentation.second = changedWindow.presentationStack.front();
        shouldUpdateVisualActivityTracker = true;
    } else {
        int focusedZOrder = -1;
        std::string focusedWindowId;
        const PresentationMetadata* focusedPresentation = nullptr;

        for (const auto& window : m_windows) {
            const int candidateZOrder = window.second.configuration.zOrderIndex;
            if (candidateZOrder >= focusedZOrder && !window.second.presentationStack.empty()) {
                // In the case where two zOrders are the same the latest request wins
                const PresentationMetadata* const candidate = &window.second.presentationStack.front();
                if (!focusedPresentation || candidate->acquiredTime > focusedPresentation->acquiredTime) {
                    focusedZOrder = candidateZOrder;
                    focusedWindowId = window.first;
                    focusedPresentation = candidate;
                }
            }
        }

        if (!focusedPresentation) {
            if (!m_focusedWindowAndPresentation.first.empty()) {
                shouldUpdateVisualActivityTracker = true;
            }
        } else if (
            m_focusedWindowAndPresentation.first.empty() ||
            m_focusedWindowAndPresentation.second.metadata.interfaceName !=
                focusedPresentation->metadata.interfaceName) {
            shouldUpdateVisualActivityTracker = true;
        }

        m_focusedWindowAndPresentation.first = focusedWindowId;
        if (focusedPresentation) {
            m_focusedWindowAndPresentation.second = *focusedPresentation;
        }
    }

    if (shouldUpdateVisualActivityTracker) {
        executeUpdateVisualActivityTracker();
    }
}

void PresentationOrchestratorStateTracker::executeUpdateVisualActivityTracker() {
    if (!m_visualActivityTracker) return;

    afml::Channel::State stateUpdate(avsCommon::sdkInterfaces::FocusManagerInterface::VISUAL_CHANNEL_NAME);

    stateUpdate.interfaceName = executeGetFocusedInterface();
    if (stateUpdate.interfaceName.empty()) {
        stateUpdate.focusState = avsCommon::avs::FocusState::NONE;
    } else {
        stateUpdate.focusState = avsCommon::avs::FocusState::FOREGROUND;
    }

    m_visualActivityTracker->notifyOfActivityUpdates({stateUpdate});
}

std::vector<PresentationOrchestratorWindowInfo> PresentationOrchestratorStateTracker::getWindowInformation() {
    return m_executor
        ->submit([this]() {
            std::vector<PresentationOrchestratorWindowInfo> instances;

            for (const auto& window : m_windows) {
                PresentationOrchestratorWindowInfo info;
                info.configuration = window.second.configuration;
                if (!window.second.presentationStack.empty()) {
                    info.state = window.second.presentationStack.front().metadata;
                }
                instances.push_back(info);
            }

            return instances;
        })
        .get();
}

void PresentationOrchestratorStateTracker::setExecutor(
    const std::shared_ptr<avsCommon::utils::threading::Executor>& executor) {
    ACSDK_WARN(LX(__func__).d("reason", "should be called in test only"));
    m_executor = executor;
}

void PresentationOrchestratorStateTracker::doShutdown() {
    m_executor->shutdown();
}

std::string PresentationOrchestratorStateTracker::executeGetFocusedInterface() {
    std::string interfaceName;
    if (!m_focusedWindowAndPresentation.first.empty()) {
        interfaceName = m_focusedWindowAndPresentation.second.metadata.interfaceName;
    } else if (!m_deviceInterface.empty()) {
        interfaceName = m_deviceInterface;
    }
    return interfaceName;
}

std::string PresentationOrchestratorStateTracker::executeGetFocusedWindowId() {
    std::string windowId;
    if (!m_focusedWindowAndPresentation.first.empty()) {
        windowId = m_focusedWindowAndPresentation.first;
    }
    return windowId;
}
}  // namespace presentationOrchestratorStateTracker
}  // namespace alexaClientSDK
