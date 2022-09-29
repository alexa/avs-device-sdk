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

#include "IPCServerSampleApp/IPC/Components/APLClientHandler.h"
#include "IPCServerSampleApp/IPC/Namespaces/APLClientNamespace.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::json;

#define TAG "APLClientHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Constant for initializeRenderersRequest event.
static const char INITIALIZE_RENDERERS_REQUEST_EVENT[] = "initializeRenderersRequest";

/// Constant for metricsReport event.
static const char METRICS_REPORT_EVENT[] = "metricsReport";

/// Constant for viewhostEvent.
static const char VIEWHOST_EVENT[] = "viewhostEvent";

/// Constant for renderCompleted event.
static const char RENDER_COMPLETED_EVENT[] = "renderCompleted";

/// Constant for renderDocumentRequest event.
static const char RENDER_DOCUMENT_REQUEST_EVENT[] = "renderDocumentRequest";

/// Constant for executeCommandsRequest event.
static const char EXECUTE_COMMANDS_REQUEST_EVENT[] = "executeCommandsRequest";

/// Constant for clearDocumentRequest event.
static const char CLEAR_DOCUMENT_REQUEST[] = "clearDocumentRequest";

std::shared_ptr<APLClientHandler> APLClientHandler::create(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<APLClientHandlerInterface> aplClientComponent) {
    if (!ipcHandlerRegistrar) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIpcHandlerRegistrar"));
        return nullptr;
    }

    if (!aplClientComponent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAPLClientComponent"));
        return nullptr;
    }

    auto aplClientHandler = std::shared_ptr<APLClientHandler>(
        new APLClientHandler(std::move(ipcHandlerRegistrar), std::move(aplClientComponent)));

    if (aplClientHandler) {
        aplClientHandler->registerHandlers();
    }

    return aplClientHandler;
}

APLClientHandler::APLClientHandler(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<APLClientHandlerInterface> aplClientComponent) :
        IPCHandlerBase(IPCNamespaces::IPC_MSG_NAMESPACE_APL, std::make_shared<threading::Executor>()),
        RequiresShutdown{TAG},
        m_ipcHandlerRegistrar{std::move(ipcHandlerRegistrar)},
        m_aplClientComponent{std::move(aplClientComponent)} {
}

void APLClientHandler::registerHandlers() {
    /// Register @c APLClientHandler.
    m_ipcDispatcher = m_ipcHandlerRegistrar->registerHandler(IPCNamespaces::IPC_MSG_NAMESPACE_APL, shared_from_this());
    if (!m_ipcDispatcher) {
        ACSDK_ERROR(LX("registerHandlerFailed").d("namespace", IPCNamespaces::IPC_MSG_NAMESPACE_APL));
        return;
    }
    /// Register its methods to handle incoming events from the IPC client.
    if (!registerIPCMessageHandlerMethod(INITIALIZE_RENDERERS_REQUEST_EVENT, [this](std::string message) {
            m_aplClientComponent->initializeRenderersRequest(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", INITIALIZE_RENDERERS_REQUEST_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(
            METRICS_REPORT_EVENT, [this](std::string message) { m_aplClientComponent->metricsReport(message); })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", METRICS_REPORT_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(
            VIEWHOST_EVENT, [this](std::string message) { m_aplClientComponent->viewhostEvent(message); })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", VIEWHOST_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(
            RENDER_COMPLETED_EVENT, [this](std::string message) { m_aplClientComponent->renderCompleted(message); })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", RENDER_COMPLETED_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(RENDER_DOCUMENT_REQUEST_EVENT, [this](std::string message) {
            m_aplClientComponent->renderDocumentRequest(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", RENDER_DOCUMENT_REQUEST_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(EXECUTE_COMMANDS_REQUEST_EVENT, [this](std::string message) {
            m_aplClientComponent->executeCommandsRequest(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", EXECUTE_COMMANDS_REQUEST_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(CLEAR_DOCUMENT_REQUEST, [this](std::string message) {
            m_aplClientComponent->clearDocumentRequest(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", CLEAR_DOCUMENT_REQUEST));
        return;
    }
}

void APLClientHandler::dispatchCreateRenderer(const std::string& windowId, const std::string& token) {
    m_executor->submit([this, windowId, token]() {
        auto message = IPCNamespaces::AplCreateRendererMessage(windowId, token);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("executeDispatchCreateRenderer failed"));
        }
    });
}

void APLClientHandler::dispatchSendMessageToViewhost(const std::string& windowId, const std::string& payload) {
    m_executor->submit([this, windowId, payload]() {
        auto message = IPCNamespaces::AplViewhostMessage(windowId, payload);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("executeDispatchSendMessageToViewhost failed"));
        }
    });
}

void APLClientHandler::doShutdown() {
    /// Deregister @c APLClientHandler.
    if (!m_ipcHandlerRegistrar->deregisterHandler(IPCNamespaces::IPC_MSG_NAMESPACE_APL)) {
        ACSDK_WARN(LX("deregisterHandlerFailed"));
    }
    m_ipcDispatcher.reset();
    m_ipcHandlerRegistrar.reset();
    m_aplClientComponent.reset();
}

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
