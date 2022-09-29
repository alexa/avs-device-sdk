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

#include "IPCServerSampleApp/IPC/Components/DoNotDisturbHandler.h"
#include "IPCServerSampleApp/IPC/Namespaces/DoNotDisturbNamespace.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::json;

#define TAG "DoNotDisturbHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Constant for doNotDisturbStateChanged event.
static const char DO_NOT_DISTURB_STATE_CHANGED_EVENT[] = "doNotDisturbStateChanged";

/// Constant for doNotDisturbStateRequest event.
static const char DO_NOT_DISTURB_STATE_REQUEST_EVENT[] = "doNotDisturbStateRequest";

std::shared_ptr<DoNotDisturbHandler> DoNotDisturbHandler::create(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<DoNotDisturbHandlerInterface> doNotDisturbComponent) {
    if (!ipcHandlerRegistrar) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIpcHandlerRegistrar"));
        return nullptr;
    }

    if (!doNotDisturbComponent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullDoNotDisturbComponent"));
        return nullptr;
    }

    auto doNotDisturbHandler = std::shared_ptr<DoNotDisturbHandler>(
        new DoNotDisturbHandler(std::move(ipcHandlerRegistrar), std::move(doNotDisturbComponent)));

    if (doNotDisturbHandler) {
        doNotDisturbHandler->registerHandlers();
    }

    return doNotDisturbHandler;
}

DoNotDisturbHandler::DoNotDisturbHandler(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<DoNotDisturbHandlerInterface> doNotDisturbComponent) :
        IPCHandlerBase(IPCNamespaces::IPC_MSG_NAMESPACE_DO_NOT_DISTURB, std::make_shared<threading::Executor>()),
        RequiresShutdown{TAG},
        m_ipcHandlerRegistrar{std::move(ipcHandlerRegistrar)},
        m_doNotDisturbComponent{std::move(doNotDisturbComponent)} {
}

void DoNotDisturbHandler::registerHandlers() {
    /// Register @c DoNotDisturbHandler.
    m_ipcDispatcher =
        m_ipcHandlerRegistrar->registerHandler(IPCNamespaces::IPC_MSG_NAMESPACE_DO_NOT_DISTURB, shared_from_this());
    if (!m_ipcDispatcher) {
        ACSDK_ERROR(LX("registerHandlerFailed").d("namespace", IPCNamespaces::IPC_MSG_NAMESPACE_DO_NOT_DISTURB));
        return;
    }
    /// Register its methods to handle incoming events from the IPC client.
    if (!registerIPCMessageHandlerMethod(DO_NOT_DISTURB_STATE_CHANGED_EVENT, [this](std::string message) {
            m_doNotDisturbComponent->doNotDisturbStateChanged(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", DO_NOT_DISTURB_STATE_CHANGED_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(DO_NOT_DISTURB_STATE_REQUEST_EVENT, [this](std::string message) {
            m_doNotDisturbComponent->doNotDisturbStateRequest(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", DO_NOT_DISTURB_STATE_REQUEST_EVENT));
        return;
    }
}

void DoNotDisturbHandler::dispatchSetDoNotDisturbState(bool enabled) {
    m_executor->submit([this, enabled]() {
        auto message = IPCNamespaces::SetDoNotDisturbStateMessage(enabled);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("executeDispatchSetDoNotDisturbState failed"));
        }
    });
}

void DoNotDisturbHandler::doShutdown() {
    /// Deregister @c DoNotDisturbHandler.
    if (!m_ipcHandlerRegistrar->deregisterHandler(IPCNamespaces::IPC_MSG_NAMESPACE_DO_NOT_DISTURB)) {
        ACSDK_WARN(LX("deregisterHandlerFailed"));
    }
    m_ipcDispatcher.reset();
    m_ipcHandlerRegistrar.reset();
    m_doNotDisturbComponent.reset();
}

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
