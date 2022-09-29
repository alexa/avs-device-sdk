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

#include "IPCServerSampleApp/IPC/Components/SystemHandler.h"
#include "IPCServerSampleApp/IPC/Namespaces/SystemNamespace.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::json;

#define TAG "SystemHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Constant for alexaStateRequest event.
static const char ALEXA_STATE_REQUEST_EVENT[] = "alexaStateRequest";

/// Constant for authorizationInfoRequest event.
static const char AUTHORIZATION_INFO_REQUEST_EVENT[] = "authorizationInfoRequest";

/// Constant for authorizationStateRequest event.
static const char AUTHORIZATION_STATE_REQUEST_EVENT[] = "authorizationStateRequest";

/// Constant for localesRequest event.
static const char LOCALES_REQUEST_EVENT[] = "localesRequest";

std::shared_ptr<SystemHandler> SystemHandler::create(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<SystemHandlerInterface> systemComponent) {
    if (!ipcHandlerRegistrar) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIpcHandlerRegistrar"));
        return nullptr;
    }

    if (!systemComponent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullSystemComponent"));
        return nullptr;
    }

    auto systemHandler =
        std::shared_ptr<SystemHandler>(new SystemHandler(std::move(ipcHandlerRegistrar), std::move(systemComponent)));

    if (systemHandler) {
        systemHandler->registerHandlers();
    }

    return systemHandler;
}

SystemHandler::SystemHandler(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<SystemHandlerInterface> systemComponent) :
        IPCHandlerBase(IPCNamespaces::IPC_MSG_NAMESPACE_SYSTEM, std::make_shared<threading::Executor>()),
        RequiresShutdown{TAG},
        m_ipcHandlerRegistrar{std::move(ipcHandlerRegistrar)},
        m_systemComponent{std::move(systemComponent)} {
}

void SystemHandler::registerHandlers() {
    /// Register @c SystemHandler.
    m_ipcDispatcher =
        m_ipcHandlerRegistrar->registerHandler(IPCNamespaces::IPC_MSG_NAMESPACE_SYSTEM, shared_from_this());
    if (!m_ipcDispatcher) {
        ACSDK_ERROR(LX("registerHandlerFailed").d("namespace", IPCNamespaces::IPC_MSG_NAMESPACE_SYSTEM));
        return;
    }
    /// Register its methods to handle incoming events from the IPC client.
    if (!registerIPCMessageHandlerMethod(ALEXA_STATE_REQUEST_EVENT, [this](std::string message) {
            m_systemComponent->alexaStateRequest(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", ALEXA_STATE_REQUEST_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(AUTHORIZATION_INFO_REQUEST_EVENT, [this](std::string message) {
            m_systemComponent->authorizationInfoRequest(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", AUTHORIZATION_INFO_REQUEST_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(AUTHORIZATION_STATE_REQUEST_EVENT, [this](std::string message) {
            m_systemComponent->authorizationStateRequest(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", AUTHORIZATION_STATE_REQUEST_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(
            LOCALES_REQUEST_EVENT, [this](std::string message) { m_systemComponent->localesRequest(message); })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", LOCALES_REQUEST_EVENT));
        return;
    }
}

void SystemHandler::completeAuthorization(
    const std::string& url,
    const std::string& code,
    const std::string& clientId) {
    m_executor->submit([this, url, code, clientId]() {
        auto message = ipc::IPCNamespaces::CompleteAuthorizationMessage(url, code, clientId);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("executeCompleteAuthorization failed"));
        }
    });
}

void SystemHandler::setAlexaState(const std::string& state) {
    m_executor->submit([this, state]() {
        auto message = ipc::IPCNamespaces::SetAlexaStateMessage(state);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("executeSetAlexaState failed"));
        }
    });
}

void SystemHandler::setAuthorizationState(const std::string& state) {
    m_executor->submit([this, state]() {
        auto message = ipc::IPCNamespaces::SetAuthorizationStateMessage(state);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("executeSetAuthorizationState failed"));
        }
    });
}

void SystemHandler::setLocales(const std::string& localeStr) {
    m_executor->submit([this, localeStr]() {
        auto message = ipc::IPCNamespaces::SetLocalesMessage(localeStr);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("executeSetLocales failed"));
        }
    });
}

void SystemHandler::doShutdown() {
    /// Deregister @c SystemHandler.
    if (!m_ipcHandlerRegistrar->deregisterHandler(IPCNamespaces::IPC_MSG_NAMESPACE_SYSTEM)) {
        ACSDK_WARN(LX("deregisterHandlerFailed"));
    }

    m_ipcDispatcher.reset();
    m_ipcHandlerRegistrar.reset();
}

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
