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

#include <acsdk/PresentationOrchestratorClient/PresentationOrchestratorClientFactory.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateTrackerInterface.h>
#include <acsdk/VisualTimeoutManager/VisualTimeoutManagerFactory.h>
#include <acsdkShutdownManager/ShutdownNotifier.h>
#include <acsdkShutdownManager/ShutdownManager.h>

#include "acsdk/PresentationOrchestratorFeatureClient/PresentationOrchestratorFeatureClient.h"

namespace alexaClientSDK {
namespace featureClient {
/// String to identify log entries originating from this file.
#define TAG "PresentationOrchestratorFeatureClient"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// String used to identify this feature client
static const char PRESENTATION_ORCHESTRATOR_FEATURE_CLIENT[] = "PresentationOrchestratorFeatureClient";

/// String used as a client ID for the Presentation Orchestrator
static const char DEFAULT_CLIENT_ID[] = "default";

PresentationOrchestratorFeatureClient::PresentationOrchestratorFeatureClient(
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorInterface> presentationOrchestrator,
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorClientInterface>
        presentationOrchestratorClient,
    std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface> visualTimeoutManager,
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> shutdownManager) :
        FeatureClientInterface(PRESENTATION_ORCHESTRATOR_FEATURE_CLIENT),
        m_presentationOrchestrator{std::move(presentationOrchestrator)},
        m_presentationOrchestratorClient{std::move(presentationOrchestratorClient)},
        m_visualTimeoutManager{std::move(visualTimeoutManager)},
        m_shutdownManager{std::move(shutdownManager)} {
}

std::unique_ptr<PresentationOrchestratorFeatureClient> PresentationOrchestratorFeatureClient::create(
    const std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>&
        presentationOrchestratorStateTracker,
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    if (!presentationOrchestratorStateTracker) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "null presentationOrchestratorStateTracker"));
        return nullptr;
    }

    auto shutdownNotifier = acsdkShutdownManager::ShutdownNotifier::createShutdownNotifierInterface();
    auto shutdownManager = acsdkShutdownManager::ShutdownManager::createShutdownManagerInterface(shutdownNotifier);
    if (!shutdownNotifier || !shutdownManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "null shutdownManager"));
        return nullptr;
    }

    auto visualTimeoutManagerExports = visualTimeoutManager::VisualTimeoutManagerFactory::create();
    if (!visualTimeoutManagerExports.hasValue()) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateVisualTimeoutManager"));
        return nullptr;
    }

    auto visualTimeoutManagerInterfaces = visualTimeoutManagerExports.value();
    shutdownNotifier->addObserver(visualTimeoutManagerInterfaces.requiresShutdown);

    auto presentationOrchestratorClientExports =
        presentationOrchestratorClient::PresentationOrchestratorClientFactory::create(
            presentationOrchestratorStateTracker,
            visualTimeoutManagerInterfaces.visualTimeoutManagerInterface,
            DEFAULT_CLIENT_ID);
    if (!presentationOrchestratorClientExports.hasValue()) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreatePresentationOrchestratorClient"));
        return nullptr;
    }
    auto presentationOrchestratorClientInterfaces = presentationOrchestratorClientExports.value();

    if (sdkClientRegistry) {
        sdkClientRegistry->registerComponent(visualTimeoutManagerInterfaces.visualTimeoutManagerInterface);
        sdkClientRegistry->registerComponent(
            presentationOrchestratorClientInterfaces.presentationOrchestratorInterface);
        sdkClientRegistry->registerComponent(
            presentationOrchestratorClientInterfaces.presentationOrchestratorClientInterface);
    }

    auto client = std::unique_ptr<PresentationOrchestratorFeatureClient>(new PresentationOrchestratorFeatureClient(
        std::move(presentationOrchestratorClientInterfaces.presentationOrchestratorInterface),
        std::move(presentationOrchestratorClientInterfaces.presentationOrchestratorClientInterface),
        std::move(visualTimeoutManagerInterfaces.visualTimeoutManagerInterface),
        std::move(shutdownManager)));

    return client;
}

PresentationOrchestratorFeatureClient::~PresentationOrchestratorFeatureClient() {
    shutdown();
}

std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorClientInterface>
PresentationOrchestratorFeatureClient::getPresentationOrchestratorClient() const {
    return m_presentationOrchestratorClient;
}

std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorInterface>
PresentationOrchestratorFeatureClient::getPresentationOrchestrator() const {
    return m_presentationOrchestrator;
}

std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface>
PresentationOrchestratorFeatureClient::getVisualTimeoutManager() const {
    return m_visualTimeoutManager;
}

bool PresentationOrchestratorFeatureClient::configure(
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    // no-op
    return true;
}

void PresentationOrchestratorFeatureClient::doShutdown() {
    if (m_shutdownManager) {
        m_shutdownManager->shutdown();
        m_shutdownManager.reset();
    }
}

}  // namespace featureClient
}  // namespace alexaClientSDK
