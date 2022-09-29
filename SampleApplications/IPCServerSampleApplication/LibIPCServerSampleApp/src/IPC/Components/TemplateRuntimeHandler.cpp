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
#include <acsdk/TemplateRuntimeInterfaces/TemplateRuntimeObserverInterface.h>

#include "IPCServerSampleApp/IPC/Components/TemplateRuntimeHandler.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/TemplateRuntimeHandlerInterface.h"
#include "IPCServerSampleApp/IPC/Namespaces/TemplateRuntimeNamespace.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::json;

#define TAG "TemplateRuntimeHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Constant for windowIdReport event.
static const char TEMPLATE_RUNTIME_WINDOW_ID_REPORT_EVENT[] = "windowIdReport";

std::shared_ptr<TemplateRuntimeHandler> TemplateRuntimeHandler::create(
    const std::shared_ptr<IPCHandlerRegistrationInterface>& ipcHandlerRegistrar,
    const std::shared_ptr<TemplateRuntimeHandlerInterface>& templateRuntimeComponent) {
    if (!ipcHandlerRegistrar) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIpcHandlerRegistrar"));
        return nullptr;
    }

    if (!templateRuntimeComponent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullTemplateRuntimeComponent"));
        return nullptr;
    }

    auto templateRuntimeHandler = std::shared_ptr<TemplateRuntimeHandler>(
        new TemplateRuntimeHandler(std::move(ipcHandlerRegistrar), std::move(templateRuntimeComponent)));

    if (templateRuntimeHandler) {
        templateRuntimeHandler->registerHandlers();
    }

    return templateRuntimeHandler;
}

TemplateRuntimeHandler::TemplateRuntimeHandler(
    std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
    std::shared_ptr<TemplateRuntimeHandlerInterface> templateRuntimeComponent) :
        IPCHandlerBase(IPCNamespaces::IPC_MSG_NAMESPACE_TEMPLATE_RUNTIME, std::make_shared<threading::Executor>()),
        RequiresShutdown{TAG},
        m_ipcHandlerRegistrar{std::move(ipcHandlerRegistrar)},
        m_templateRuntimeComponent{std::move(templateRuntimeComponent)} {
}

void TemplateRuntimeHandler::registerHandlers() {
    /// Register @c TemplateRuntimeHandler.
    m_ipcDispatcher =
        m_ipcHandlerRegistrar->registerHandler(IPCNamespaces::IPC_MSG_NAMESPACE_TEMPLATE_RUNTIME, shared_from_this());
    if (!m_ipcDispatcher) {
        ACSDK_ERROR(LX("registerHandlerFailed").d("namespace", IPCNamespaces::IPC_MSG_NAMESPACE_TEMPLATE_RUNTIME));
        return;
    }

    /// Register its methods to handle incoming events from the IPC client.
    if (!registerIPCMessageHandlerMethod(TEMPLATE_RUNTIME_WINDOW_ID_REPORT_EVENT, [this](const std::string& message) {
            m_templateRuntimeComponent->windowIdReport(message);
        })) {
        ACSDK_ERROR(LX("registerIPCMessageHandlerMethod").d("methodName", TEMPLATE_RUNTIME_WINDOW_ID_REPORT_EVENT));
        return;
    }
}

void TemplateRuntimeHandler::renderPlayerInfoCard(
    const std::string& payload,
    templateRuntimeInterfaces::TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo) {
    m_executor->submit([this, payload, audioPlayerInfo]() {
        auto message = ipc::IPCNamespaces::RenderPlayerInfoMessage(payload, audioPlayerInfo);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("renderPlayerInfoCard failed"));
        }
    });
}

void TemplateRuntimeHandler::renderTemplateCard(const std::string& payload) {
    m_executor->submit([this, payload]() {
        auto message = ipc::IPCNamespaces::RenderTemplateMessage(payload);
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("renderTemplateCard failed"));
        }
    });
}

void TemplateRuntimeHandler::clearPlayerInfoCard() {
    m_executor->submit([this]() {
        auto message = ipc::IPCNamespaces::ClearPlayerInfoCardMessage();
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("clearPlayerInfoCard failed"));
        }
    });
}

void TemplateRuntimeHandler::clearRenderTemplateCard() {
    m_executor->submit([this]() {
        auto message = ipc::IPCNamespaces::ClearRenderTemplateCardMessage();
        if (!m_ipcDispatcher || !m_ipcDispatcher->dispatch(message.get())) {
            ACSDK_ERROR(LX("clearRenderTemplateCard failed"));
        }
    });
}

void TemplateRuntimeHandler::doShutdown() {
    /// Deregister @c TemplateRuntimeHandler.
    if (!m_ipcHandlerRegistrar->deregisterHandler(IPCNamespaces::IPC_MSG_NAMESPACE_TEMPLATE_RUNTIME)) {
        ACSDK_WARN(LX("deregisterHandlerFailed"));
    }

    m_ipcDispatcher.reset();
    m_ipcHandlerRegistrar.reset();
    m_templateRuntimeComponent.reset();
}

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
