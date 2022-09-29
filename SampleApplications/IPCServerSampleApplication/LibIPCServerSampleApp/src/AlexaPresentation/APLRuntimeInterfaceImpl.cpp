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

#include "IPCServerSampleApp/AlexaPresentation/APLRuntimeInterfaceImpl.h"

#include <memory>

#include "IPCServerSampleApp/AlexaPresentation/APLViewhostInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

using presentationOrchestratorInterfaces::PresentationLifespan;

static const std::string TAG{"APLRuntimeInterfaceImpl"};
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

APLRuntimeInterfaceImpl::APLRuntimeInterfaceImpl(APLViewhostInterfacePtr viewhost) : m_viewhost{viewhost} {
}

APLRuntimeInterfaceImpl::~APLRuntimeInterfaceImpl() {
}

std::shared_ptr<APLRuntimeInterfaceImpl> APLRuntimeInterfaceImpl::create(const APLViewhostInterfacePtr& viewhost) {
    if (!viewhost) {
        ACSDK_ERROR(LX(__func__).m("viewhost null"));
        return nullptr;
    }

    return std::shared_ptr<APLRuntimeInterfaceImpl>(new APLRuntimeInterfaceImpl(viewhost));
}
void APLRuntimeInterfaceImpl::renderDocument(
    const std::string& document,
    const std::string& data,
    const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
    const aplCapabilityCommonInterfaces::PresentationOptions& presentationOptions,
    std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentObserverInterface> observer) {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_presentationOrchestratorClient) {
        ACSDK_CRITICAL(
            LX("renderDocumentFailed").d("reason", "Presentation Orchestrator Client has not been configured"));
        return;
    }

    aplCapabilityCommonInterfaces::PresentationOptions options = presentationOptions;
    if (options.windowId.empty()) {
        std::lock_guard<std::mutex> guard(m_mutex);
        options.windowId = m_defaultWindowId;
    }

    auto documentSession =
        createDocumentSession(document, data, presentationSession, presentationOptions, observer, true);

    presentationOrchestratorInterfaces::PresentationOptions poOptions;
    poOptions.presentationLifespan = options.lifespan;
    poOptions.metadata = options.token;
    poOptions.interfaceName = options.interfaceName;
    poOptions.timeout = options.timeout;

    m_presentationOrchestratorClient->requestWindow(options.windowId, poOptions, documentSession);
}

std::shared_ptr<APLDocumentSession> APLRuntimeInterfaceImpl::createDocumentSession(
    const std::string& document,
    const std::string& data,
    const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
    const aplCapabilityCommonInterfaces::PresentationOptions& presentationOptions,
    std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentObserverInterface> observer,
    bool hasPresentationAssociation) {
    ACSDK_DEBUG5(LX(__func__));

    aplCapabilityCommonInterfaces::PresentationOptions options = presentationOptions;
    if (options.windowId.empty()) {
        std::lock_guard<std::mutex> guard(m_mutex);
        options.windowId = m_defaultWindowId;
    }

    return std::shared_ptr<APLDocumentSession>(new APLDocumentSession(
        document,
        data,
        presentationOptions.supportedViewports,
        presentationSession,
        options,
        std::move(observer),
        m_viewhost,
        hasPresentationAssociation));
}

void APLRuntimeInterfaceImpl::setDefaultWindowId(const std::string& windowId) {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_defaultWindowId = windowId;
}

std::string APLRuntimeInterfaceImpl::getMaxAPLVersion() const {
    return m_viewhost->getMaxAPLVersion();
}

void APLRuntimeInterfaceImpl::setPresentationOrchestrator(
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorClientInterface> poClient) {
    m_presentationOrchestratorClient = std::move(poClient);
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
