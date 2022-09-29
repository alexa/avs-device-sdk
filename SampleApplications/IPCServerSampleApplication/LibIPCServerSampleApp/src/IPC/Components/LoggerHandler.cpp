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

#include "IPCServerSampleApp/IPC/Components/LoggerHandler.h"
#include "IPCServerSampleApp/IPC/Namespaces/LoggerNamespace.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::json;

#define TAG "LoggerHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Constant for guiActivityEvent.
static const char LOG_EVENT[] = "logEvent";

std::shared_ptr<LoggerHandler> LoggerHandler::create(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<LoggerHandlerInterface> loggerComponent) {
    if (!ipcHandlerRegistrar) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIpcHandlerRegistrar"));
        return nullptr;
    }

    if (!loggerComponent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullLoggerComponent"));
        return nullptr;
    }

    auto loggerHandler =
        std::shared_ptr<LoggerHandler>(new LoggerHandler(std::move(ipcHandlerRegistrar), std::move(loggerComponent)));

    if (loggerHandler) {
        loggerHandler->registerHandlers();
    }

    return loggerHandler;
}

LoggerHandler::LoggerHandler(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<LoggerHandlerInterface> loggerComponent) :
        IPCHandlerBase(IPCNamespaces::IPC_MSG_NAMESPACE_LOGGER, std::make_shared<threading::Executor>()),
        RequiresShutdown{TAG},
        m_ipcHandlerRegistrar{std::move(ipcHandlerRegistrar)},
        m_loggerComponent{std::move(loggerComponent)} {
}

void LoggerHandler::registerHandlers() {
    /// Register @c LoggerHandler.
    m_ipcDispatcher =
        m_ipcHandlerRegistrar->registerHandler(IPCNamespaces::IPC_MSG_NAMESPACE_LOGGER, shared_from_this());
    if (!m_ipcDispatcher) {
        ACSDK_ERROR(LX("registerHandlerFailed").d("namespace", IPCNamespaces::IPC_MSG_NAMESPACE_LOGGER));
        return;
    }

    /// Register its methods to handle incoming events from the IPC client.
    if (!registerIPCMessageHandlerMethod(
            LOG_EVENT, [this](std::string message) { m_loggerComponent->logEvent(message); })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", LOG_EVENT));
        return;
    }
}

void LoggerHandler::doShutdown() {
    /// Deregister @c LoggerHandler.
    if (!m_ipcHandlerRegistrar->deregisterHandler(IPCNamespaces::IPC_MSG_NAMESPACE_LOGGER)) {
        ACSDK_WARN(LX("deregisterHandlerFailed"));
    }

    m_ipcDispatcher.reset();
    m_ipcHandlerRegistrar.reset();
    m_loggerComponent.reset();
}

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
