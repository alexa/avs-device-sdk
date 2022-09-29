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

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "IPCServerSampleApp/IPC/Components/WindowManagerHandler.h"
#include "IPCServerSampleApp/IPC/Namespaces/WindowManagerNamespace.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::json;

#define TAG "WindowManagerHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Constant for visualCharacteristicsRequest event.
static const char VISUAL_CHARACTERISTICS_REQUEST_EVENT[] = "visualCharacteristicsRequest";

/// Constant for defaultWindowInstanceChanged event.
static const char DEFAULT_WINDOW_INSTANCE_CHANGED_EVENT[] = "defaultWindowInstanceChanged";

/// Constant for windowInstancesReport event.
static const char WINDOW_INSTANCES_REPORT_EVENT[] = "windowInstancesReport";

/// Constant for windowInstancesAdded event.
static const char WINDOW_INSTANCES_ADDED_EVENT[] = "windowInstancesAdded";

/// Constant for windowInstancesRemoved event.
static const char WINDOW_INSTANCES_REMOVED_EVENT[] = "windowInstancesRemoved";

/// Constant for windowInstancesUpdated event.
static const char WINDOW_INSTANCES_UPDATED_EVENT[] = "windowInstancesUpdated";

std::shared_ptr<WindowManagerHandler> WindowManagerHandler::create(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<WindowManagerHandlerInterface> windowManagerComponent) {
    if (!ipcHandlerRegistrar) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIpcHandlerRegistrar"));
        return nullptr;
    }

    if (!windowManagerComponent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullWindowManagerComponent"));
        return nullptr;
    }

    auto windowManagerHandler = std::shared_ptr<WindowManagerHandler>(
        new WindowManagerHandler(std::move(ipcHandlerRegistrar), std::move(windowManagerComponent)));

    if (windowManagerHandler) {
        windowManagerHandler->registerHandlers();
    }

    return windowManagerHandler;
}

WindowManagerHandler::WindowManagerHandler(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<WindowManagerHandlerInterface> windowManagerComponent) :
        IPCHandlerBase(IPCNamespaces::IPC_MSG_NAMESPACE_WINDOW_MANAGER, std::make_shared<threading::Executor>()),
        RequiresShutdown{TAG},
        m_ipcHandlerRegistrar{std::move(ipcHandlerRegistrar)},
        m_windowManagerComponent{std::move(windowManagerComponent)} {
}

void WindowManagerHandler::registerHandlers() {
    /// Register @c WindowManagerHandler.
    m_ipcDispatcher =
        m_ipcHandlerRegistrar->registerHandler(IPCNamespaces::IPC_MSG_NAMESPACE_WINDOW_MANAGER, shared_from_this());
    if (!m_ipcDispatcher) {
        ACSDK_ERROR(LX("registerHandlerFailed").d("namespace", IPCNamespaces::IPC_MSG_NAMESPACE_WINDOW_MANAGER));
        return;
    }

    /// Register its methods to handle incoming events from the IPC client.
    if (!registerIPCMessageHandlerMethod(VISUAL_CHARACTERISTICS_REQUEST_EVENT, [this](std::string message) {
            m_windowManagerComponent->visualCharacteristicsRequest(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", VISUAL_CHARACTERISTICS_REQUEST_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(DEFAULT_WINDOW_INSTANCE_CHANGED_EVENT, [this](std::string message) {
            m_windowManagerComponent->defaultWindowInstanceChanged(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", DEFAULT_WINDOW_INSTANCE_CHANGED_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(WINDOW_INSTANCES_REPORT_EVENT, [this](std::string message) {
            m_windowManagerComponent->windowInstancesReport(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", WINDOW_INSTANCES_REPORT_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(WINDOW_INSTANCES_ADDED_EVENT, [this](std::string message) {
            m_windowManagerComponent->windowInstancesAdded(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", WINDOW_INSTANCES_ADDED_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(WINDOW_INSTANCES_REMOVED_EVENT, [this](std::string message) {
            m_windowManagerComponent->windowInstancesRemoved(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", WINDOW_INSTANCES_REMOVED_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(WINDOW_INSTANCES_UPDATED_EVENT, [this](std::string message) {
            m_windowManagerComponent->windowInstancesUpdated(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", WINDOW_INSTANCES_UPDATED_EVENT));
        return;
    }
}

void WindowManagerHandler::dispatchSetVisualCharacteristics(
    const avsCommon::utils::Optional<std::string>& displayCharacteristicOpt,
    const avsCommon::utils::Optional<std::string>& interactionModesOpt,
    const avsCommon::utils::Optional<std::string>& windowTemplatesOpt) {
    m_executor->submit([this, displayCharacteristicOpt, interactionModesOpt, windowTemplatesOpt]() {
        auto message = IPCNamespaces::SetVisualCharacteristicsMessage(
            displayCharacteristicOpt, interactionModesOpt, windowTemplatesOpt);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("executeDispatchSetVisualCharacteristics failed"));
        }
    });
}

void WindowManagerHandler::dispatchClearWindow(const std::string& windowId) {
    m_executor->submit([this, windowId]() {
        auto message = IPCNamespaces::ClearWindowMessage(windowId);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("executeDispatchClearWindow failed"));
        }
    });
}

void WindowManagerHandler::doShutdown() {
    /// Deregister @c WindowManagerHandler.
    if (!m_ipcHandlerRegistrar->deregisterHandler(IPCNamespaces::IPC_MSG_NAMESPACE_WINDOW_MANAGER)) {
        ACSDK_WARN(LX("deregisterHandlerFailed"));
    }

    m_ipcDispatcher.reset();
    m_ipcHandlerRegistrar.reset();
    m_windowManagerComponent.reset();
}

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
