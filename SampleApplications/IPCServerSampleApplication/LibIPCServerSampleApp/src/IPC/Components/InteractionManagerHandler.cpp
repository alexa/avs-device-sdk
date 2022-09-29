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

#include "IPCServerSampleApp/IPC/Components/InteractionManagerHandler.h"
#include "IPCServerSampleApp/IPC/Namespaces/InteractionManagerNamespace.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::json;

#define TAG "InteractionManagerHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Constant for guiActivityEvent.
static const char GUI_ACTIVITY_EVENT[] = "guiActivityEvent";

/// Constant for navigationEvent.
static const char NAVIGATION_EVENT[] = "navigationEvent";

/// Constant for recognizeSpeechRequest event.
static const char RECOGNIZE_SPEECH_REQUEST_EVENT[] = "recognizeSpeechRequest";

std::shared_ptr<InteractionManagerHandler> InteractionManagerHandler::create(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<InteractionManagerHandlerInterface> interactionManagerComponent) {
    if (!ipcHandlerRegistrar) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIpcHandlerRegistrar"));
        return nullptr;
    }

    if (!interactionManagerComponent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullInteractionManagerComponent"));
        return nullptr;
    }

    auto interactionManagerHandler = std::shared_ptr<InteractionManagerHandler>(
        new InteractionManagerHandler(std::move(ipcHandlerRegistrar), std::move(interactionManagerComponent)));

    if (interactionManagerHandler) {
        interactionManagerHandler->registerHandlers();
    }

    return interactionManagerHandler;
}

InteractionManagerHandler::InteractionManagerHandler(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<InteractionManagerHandlerInterface> interactionManagerComponent) :
        IPCHandlerBase(IPCNamespaces::IPC_MSG_NAMESPACE_INTERACTION_MANAGER, std::make_shared<threading::Executor>()),
        RequiresShutdown{TAG},
        m_ipcHandlerRegistrar{std::move(ipcHandlerRegistrar)},
        m_interactionManagerComponent{std::move(interactionManagerComponent)} {
}

void InteractionManagerHandler::registerHandlers() {
    /// Register @c InteractionManagerHandler.
    m_ipcDispatcher = m_ipcHandlerRegistrar->registerHandler(
        IPCNamespaces::IPC_MSG_NAMESPACE_INTERACTION_MANAGER, shared_from_this());
    if (!m_ipcDispatcher) {
        ACSDK_ERROR(LX("registerHandlerFailed").d("namespace", IPCNamespaces::IPC_MSG_NAMESPACE_INTERACTION_MANAGER));
        return;
    }

    /// Register its methods to handle incoming events from the IPC client.
    if (!registerIPCMessageHandlerMethod(GUI_ACTIVITY_EVENT, [this](std::string message) {
            m_interactionManagerComponent->guiActivityEvent(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", GUI_ACTIVITY_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(NAVIGATION_EVENT, [this](std::string message) {
            m_interactionManagerComponent->navigationEvent(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", NAVIGATION_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(RECOGNIZE_SPEECH_REQUEST_EVENT, [this](std::string message) {
            m_interactionManagerComponent->recognizeSpeechRequest(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", RECOGNIZE_SPEECH_REQUEST_EVENT));
        return;
    }
}

void InteractionManagerHandler::doShutdown() {
    /// Deregister @c InteractionManagerHandler.
    if (!m_ipcHandlerRegistrar->deregisterHandler(IPCNamespaces::IPC_MSG_NAMESPACE_INTERACTION_MANAGER)) {
        ACSDK_WARN(LX("deregisterHandlerFailed"));
    }

    m_ipcDispatcher.reset();
    m_ipcHandlerRegistrar.reset();
    m_interactionManagerComponent.reset();
}

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
