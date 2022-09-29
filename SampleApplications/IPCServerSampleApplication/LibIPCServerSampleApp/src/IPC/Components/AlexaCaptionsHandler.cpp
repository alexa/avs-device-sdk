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

#include "IPCServerSampleApp/IPC/Components/AlexaCaptionsHandler.h"

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "IPCServerSampleApp/IPC/Namespaces/AlexaCaptionsNamespace.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::json;

/// Logger tag.
#define TAG "AlexaCaptionsHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The message name for captions state changed.
static const std::string NAME_CAPTIONS_STATE_CHANGED("captionsStateChanged");

/// The message name for captions state request.
static const std::string NAME_CAPTIONS_STATE_REQUEST("captionsStateRequest");

/// The enabled json key in the message.
static const char ENABLED_TAG[] = "enabled";

std::shared_ptr<AlexaCaptionsHandler> AlexaCaptionsHandler::create(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage) {
    if (!ipcHandlerRegistrar) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIpcHandlerRegistrar"));
        return nullptr;
    }

    if (!miscStorage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullCaptionManager"));
        return nullptr;
    }

    auto alexaCaptionsHandler = std::shared_ptr<AlexaCaptionsHandler>(
        new AlexaCaptionsHandler(std::move(ipcHandlerRegistrar), std::move(miscStorage)));

    if (alexaCaptionsHandler) {
        alexaCaptionsHandler->registerHandlers();
    }

    return alexaCaptionsHandler;
}

AlexaCaptionsHandler::AlexaCaptionsHandler(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage) :
        IPCHandlerBase(IPCNamespaces::IPC_MSG_NAMESPACE_CAPTIONS, std::make_shared<threading::Executor>()),
        RequiresShutdown{TAG},
        m_ipcHandlerRegistrar{std::move(ipcHandlerRegistrar)},
        m_captionManager{SmartScreenCaptionStateManager(miscStorage)} {
}

void AlexaCaptionsHandler::registerHandlers() {
    /// Register @c AlexaCaptionsHandler.
    m_ipcDispatcher =
        m_ipcHandlerRegistrar->registerHandler(IPCNamespaces::IPC_MSG_NAMESPACE_CAPTIONS, shared_from_this());
    if (!m_ipcDispatcher) {
        ACSDK_ERROR(LX("registerHandlerFailed").d("namespace", IPCNamespaces::IPC_MSG_NAMESPACE_CAPTIONS));
        return;
    }

    /// Register its methods to handle incoming events from the IPC client.
    if (!registerIPCMessageHandlerMethod(NAME_CAPTIONS_STATE_CHANGED, [this](std::string message) {
            bool enabled = false;

            if (!jsonUtils::retrieveValue(message, ENABLED_TAG, &enabled)) {
                ACSDK_ERROR(LX("executeHandleCaptionsStateChangedFailed").d("reason", "enabledNotFound"));
                return;
            }

            m_captionManager.setCaptionsState(enabled);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", NAME_CAPTIONS_STATE_CHANGED));
        return;
    }

    if (!registerIPCMessageHandlerMethod(NAME_CAPTIONS_STATE_REQUEST, [this](std::string message) {
            auto response = ipc::IPCNamespaces::SetCaptionsStateMessage(m_captionManager.areCaptionsEnabled());
            m_ipcDispatcher->dispatch(response.get());
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", NAME_CAPTIONS_STATE_REQUEST));
        return;
    }
}

void AlexaCaptionsHandler::renderCaptions(const std::string& payload) {
    if (m_captionManager.areCaptionsEnabled()) {
        ACSDK_DEBUG5(LX("renderCaptions"));

        auto message = ipc::IPCNamespaces::RenderCaptionsMessage(payload);
        m_ipcDispatcher->dispatch(message.get());
    }
}

void AlexaCaptionsHandler::doShutdown() {
    /// Deregister @c AlexaCaptionsHandler.
    if (!m_ipcHandlerRegistrar->deregisterHandler(IPCNamespaces::IPC_MSG_NAMESPACE_CAPTIONS)) {
        ACSDK_WARN(LX("deregisterHandlerFailed"));
    }
    m_ipcDispatcher.reset();
    m_ipcHandlerRegistrar.reset();
}

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
