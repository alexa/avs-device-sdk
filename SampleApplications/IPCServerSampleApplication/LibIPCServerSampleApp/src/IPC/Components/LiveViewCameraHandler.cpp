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

#include "IPCServerSampleApp/IPC/Components/LiveViewCameraHandler.h"
#include "IPCServerSampleApp/IPC/Namespaces/LiveViewCameraNamespace.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::json;

#define TAG "LiveViewCameraHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Constant for cameraMicrophoneStateChanged event.
static const char CAMERA_MICROPHONE_STATE_CHANGED_EVENT[] = "cameraMicrophoneStateChanged";

/// Constant for cameraFirstFrameRendered event.
static const char CAMERA_FIRST_FRAME_RENDERED_EVENT[] = "cameraFirstFrameRendered";

/// Constant for windowIdReport event.
static const char WINDOW_ID_REPORT_EVENT[] = "windowIdReport";

std::shared_ptr<LiveViewCameraHandler> LiveViewCameraHandler::create(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<LiveViewCameraHandlerInterface> liveViewCameraComponent) {
    if (!ipcHandlerRegistrar) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIpcHandlerRegistrar"));
        return nullptr;
    }

    if (!liveViewCameraComponent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullLiveViewCameraComponent"));
        return nullptr;
    }

    auto liveViewCameraHandler = std::shared_ptr<LiveViewCameraHandler>(
        new LiveViewCameraHandler(std::move(ipcHandlerRegistrar), std::move(liveViewCameraComponent)));

    if (liveViewCameraHandler) {
        liveViewCameraHandler->registerHandlers();
    }

    return liveViewCameraHandler;
}

LiveViewCameraHandler::LiveViewCameraHandler(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<LiveViewCameraHandlerInterface> liveViewCameraComponent) :
        IPCHandlerBase(IPCNamespaces::IPC_MSG_NAMESPACE_LIVE_VIEW_CAMERA, std::make_shared<threading::Executor>()),
        RequiresShutdown{TAG},
        m_ipcHandlerRegistrar{std::move(ipcHandlerRegistrar)},
        m_liveViewCameraComponent{std::move(liveViewCameraComponent)} {
}

void LiveViewCameraHandler::registerHandlers() {
    /// Register @c LiveViewCameraHandler.
    m_ipcDispatcher =
        m_ipcHandlerRegistrar->registerHandler(IPCNamespaces::IPC_MSG_NAMESPACE_LIVE_VIEW_CAMERA, shared_from_this());
    if (!m_ipcDispatcher) {
        ACSDK_ERROR(LX("registerHandlerFailed").d("namespace", IPCNamespaces::IPC_MSG_NAMESPACE_LIVE_VIEW_CAMERA));
        return;
    }
    /// Register its methods to handle incoming events from the IPC client.
    if (!registerIPCMessageHandlerMethod(CAMERA_MICROPHONE_STATE_CHANGED_EVENT, [this](std::string message) {
            m_liveViewCameraComponent->cameraMicrophoneStateChanged(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", CAMERA_MICROPHONE_STATE_CHANGED_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(CAMERA_FIRST_FRAME_RENDERED_EVENT, [this](std::string message) {
            m_liveViewCameraComponent->cameraFirstFrameRendered(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", CAMERA_FIRST_FRAME_RENDERED_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(WINDOW_ID_REPORT_EVENT, [this](std::string message) {
            m_liveViewCameraComponent->windowIdReport(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", WINDOW_ID_REPORT_EVENT));
        return;
    }
}

void LiveViewCameraHandler::renderCamera(const std::string& startLiveViewPayload) {
    m_executor->submit([this, startLiveViewPayload]() {
        auto message = IPCNamespaces::RenderCameraMessage(startLiveViewPayload);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("renderCamera failed"));
        }
    });
}

void LiveViewCameraHandler::setCameraState(const std::string& cameraState) {
    m_executor->submit([this, cameraState]() {
        auto message = IPCNamespaces::SetCameraStateMessage(cameraState);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("setCameraState failed"));
        }
    });
}

void LiveViewCameraHandler::clearCamera() {
    m_executor->submit([this]() {
        auto message = IPCNamespaces::ClearCameraMessage();
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("clearCamera failed"));
        }
    });
}

void LiveViewCameraHandler::doShutdown() {
    /// Deregister @c LiveViewCameraHandler.
    if (!m_ipcHandlerRegistrar->deregisterHandler(IPCNamespaces::IPC_MSG_NAMESPACE_LIVE_VIEW_CAMERA)) {
        ACSDK_WARN(LX("deregisterHandlerFailed"));
    }
    m_ipcDispatcher.reset();
    m_ipcHandlerRegistrar.reset();
    m_liveViewCameraComponent.reset();
}

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
