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

#include "IPCServerSampleApp/IPC/Components/SessionSetupHandler.h"
#include "IPCServerSampleApp/IPC/Namespaces/SessionSetupNamespace.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::json;

#define TAG "SessionSetupHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Constant for namespaceVersionsReport event.
static const char NAMESPACE_VERSIONS_REPORT_EVENT[] = "namespaceVersionsReport";

/// Constant for clientInitialized event.
static const char CLIENT_INITIALIZED_EVENT[] = "clientInitialized";

/// Constant for clientConfigRequest event.
static const char CLIENT_CONFIG_REQUEST_EVENT[] = "clientConfigRequest";

/// The key in our config file to find the root of sample client configuration
static const std::string SAMPLE_CLIENT_CONFIGURATION_ROOT_KEY = "sampleClientConfig";

std::shared_ptr<SessionSetupHandler> SessionSetupHandler::create(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<SessionSetupHandlerInterface> sessionSetupComponent) {
    if (!ipcHandlerRegistrar) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIpcHandlerRegistrar"));
        return nullptr;
    }

    if (!sessionSetupComponent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullSessionSetupComponent"));
        return nullptr;
    }

    auto sessionSetupHandler = std::shared_ptr<SessionSetupHandler>(
        new SessionSetupHandler(std::move(ipcHandlerRegistrar), std::move(sessionSetupComponent)));

    if (sessionSetupHandler) {
        sessionSetupHandler->registerHandlers();
    }

    return sessionSetupHandler;
}

SessionSetupHandler::SessionSetupHandler(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<SessionSetupHandlerInterface> sessionSetupComponent) :
        IPCHandlerBase(IPCNamespaces::IPC_MSG_NAMESPACE_SESSION_SETUP, std::make_shared<threading::Executor>()),
        RequiresShutdown{TAG},
        m_ipcHandlerRegistrar{std::move(ipcHandlerRegistrar)},
        m_sessionSetupComponent{std::move(sessionSetupComponent)} {
}

void SessionSetupHandler::registerHandlers() {
    /// Register @c SessionSetupHandler.
    m_ipcDispatcher =
        m_ipcHandlerRegistrar->registerHandler(IPCNamespaces::IPC_MSG_NAMESPACE_SESSION_SETUP, shared_from_this());
    if (!m_ipcDispatcher) {
        ACSDK_ERROR(LX("registerHandlerFailed").d("namespace", IPCNamespaces::IPC_MSG_NAMESPACE_SESSION_SETUP));
        return;
    }
    /// Register its methods to handle incoming events from the IPC client.
    if (!registerIPCMessageHandlerMethod(NAMESPACE_VERSIONS_REPORT_EVENT, [this](std::string message) {
            m_sessionSetupComponent->namespaceVersionsReport(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", NAMESPACE_VERSIONS_REPORT_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(CLIENT_INITIALIZED_EVENT, [this](std::string message) {
            m_sessionSetupComponent->clientInitialized(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", CLIENT_INITIALIZED_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(CLIENT_CONFIG_REQUEST_EVENT, [this](std::string message) {
            m_sessionSetupComponent->clientConfigRequest(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", CLIENT_CONFIG_REQUEST_EVENT));
        return;
    }
}

void SessionSetupHandler::dispatchConfigureClient(const std::string& payload) {
    m_executor->submit([this, payload]() {
        /// Get the root ConfigurationNode.
        auto configurationRoot = avsCommon::utils::configuration::ConfigurationNode::getRoot();
        auto message =
            IPCNamespaces::ConfigureClientMessage(configurationRoot[SAMPLE_CLIENT_CONFIGURATION_ROOT_KEY].serialize());
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("executeDispatchConfigureClient failed"));
        }
    });
}

void SessionSetupHandler::dispatchInitializeClient(const std::string& ipcVersion) {
    m_executor->submit([this, ipcVersion]() {
        auto message = IPCNamespaces::InitClientMessage(ipcVersion);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("executeDispatchInitializeClient failed"));
        }
    });
}

void SessionSetupHandler::doShutdown() {
    /// Deregister @c SessionSetupHandler.
    if (!m_ipcHandlerRegistrar->deregisterHandler(IPCNamespaces::IPC_MSG_NAMESPACE_SESSION_SETUP)) {
        ACSDK_WARN(LX("deregisterHandlerFailed"));
    }
    m_ipcDispatcher.reset();
    m_ipcHandlerRegistrar.reset();
    m_sessionSetupComponent.reset();
}

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
