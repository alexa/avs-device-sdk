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

#include "IPCServerSampleApp/IPC/Components/AudioFocusManagerHandler.h"
#include "IPCServerSampleApp/IPC/Namespaces/AudioFocusManagerNamespace.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::json;

#define TAG "AudioFocusManagerHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Constant for acquireChannelRequest event.
static const char ACQUIRE_CHANNEL_REQUEST_EVENT[] = "acquireChannelRequest";

/// Constant for releaseChannelRequest event.
static const char RELEASE_CHANNEL_REQUEST_EVENT[] = "releaseChannelRequest";

/// Constant for focusChangedReport event.
static const char FOCUS_CHANGED_REPORT_EVENT[] = "focusChangedReport";

std::shared_ptr<AudioFocusManagerHandler> AudioFocusManagerHandler::create(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<AudioFocusManagerHandlerInterface> audioFocusManagerComponent) {
    if (!ipcHandlerRegistrar) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIpcHandlerRegistrar"));
        return nullptr;
    }

    if (!audioFocusManagerComponent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAudioFocusManagerComponent"));
        return nullptr;
    }

    auto audioFocusManagerHandler = std::shared_ptr<AudioFocusManagerHandler>(
        new AudioFocusManagerHandler(std::move(ipcHandlerRegistrar), std::move(audioFocusManagerComponent)));

    if (audioFocusManagerHandler) {
        audioFocusManagerHandler->registerHandlers();
    }

    return audioFocusManagerHandler;
}

AudioFocusManagerHandler::AudioFocusManagerHandler(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<AudioFocusManagerHandlerInterface> audioFocusManagerComponent) :
        IPCHandlerBase(IPCNamespaces::IPC_MSG_NAMESPACE_AUDIO_FOCUS_MANAGER, std::make_shared<threading::Executor>()),
        RequiresShutdown{TAG},
        m_ipcHandlerRegistrar{std::move(ipcHandlerRegistrar)},
        m_audioFocusManagerComponent{std::move(audioFocusManagerComponent)} {
}

void AudioFocusManagerHandler::registerHandlers() {
    /// Register @c AudioFocusManagerHandler.
    m_ipcDispatcher = m_ipcHandlerRegistrar->registerHandler(
        IPCNamespaces::IPC_MSG_NAMESPACE_AUDIO_FOCUS_MANAGER, shared_from_this());
    if (!m_ipcDispatcher) {
        ACSDK_ERROR(LX("registerHandlerFailed").d("namespace", IPCNamespaces::IPC_MSG_NAMESPACE_AUDIO_FOCUS_MANAGER));
        return;
    }
    /// Register its methods to handle incoming events from the IPC client.
    if (!registerIPCMessageHandlerMethod(ACQUIRE_CHANNEL_REQUEST_EVENT, [this](std::string message) {
            m_audioFocusManagerComponent->acquireChannelRequest(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", ACQUIRE_CHANNEL_REQUEST_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(RELEASE_CHANNEL_REQUEST_EVENT, [this](std::string message) {
            m_audioFocusManagerComponent->releaseChannelRequest(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", RELEASE_CHANNEL_REQUEST_EVENT));
        return;
    }
    if (!registerIPCMessageHandlerMethod(FOCUS_CHANGED_REPORT_EVENT, [this](std::string message) {
            m_audioFocusManagerComponent->focusChangedReport(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", FOCUS_CHANGED_REPORT_EVENT));
        return;
    }
}

void AudioFocusManagerHandler::processChannelResult(unsigned token, bool result) {
    m_executor->submit([this, token, result]() {
        auto message = IPCNamespaces::ProcessChannelResultMessage(token, result);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("executeProcessChannelResult failed"));
        }
    });
}

void AudioFocusManagerHandler::processFocusChanged(unsigned token, avsCommon::avs::FocusState focusState) {
    m_executor->submit([this, token, focusState]() {
        auto message = IPCNamespaces::ProcessFocusChangedMessage(token, focusState);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("executeProcessFocusChanged failed"));
        }
    });
}

void AudioFocusManagerHandler::doShutdown() {
    /// Deregister @c AudioFocusManagerHandler.
    if (!m_ipcHandlerRegistrar->deregisterHandler(IPCNamespaces::IPC_MSG_NAMESPACE_AUDIO_FOCUS_MANAGER)) {
        ACSDK_WARN(LX("deregisterHandlerFailed"));
    }

    m_ipcDispatcher.reset();
    m_ipcHandlerRegistrar.reset();
    m_audioFocusManagerComponent.reset();
}

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
