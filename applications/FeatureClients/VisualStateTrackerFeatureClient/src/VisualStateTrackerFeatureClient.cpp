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

#include <acsdk/PresentationOrchestratorStateTracker/PresentationOrchestratorStateTrackerFactory.h>
#include <acsdk/VisualCharacteristics/VisualCharacteristicsFactory.h>
#include <acsdk/VisualCharacteristics/VisualCharacteristicsSerializerFactory.h>
#include <acsdkShutdownManager/ShutdownManager.h>
#include <acsdkShutdownManager/ShutdownNotifier.h>
#include <AFML/VisualActivityTracker.h>

#include "acsdk/VisualStateTrackerFeatureClient/VisualStateTrackerFeatureClient.h"

namespace alexaClientSDK {
namespace featureClient {
/// String to identify log entries originating from this file.
#define TAG "VisualStateTrackerFeatureClient"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// String used to identify this feature client builder
static const char VISUAL_STATE_TRACKER_FEATURE_CLIENT[] = "VisualStateTrackerFeatureClient";

VisualStateTrackerFeatureClient::VisualStateTrackerFeatureClient(
    std::shared_ptr<alexaClientSDK::presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>
        presentationOrchestratorStateTracker,
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> shutdownManager) :
        FeatureClientInterface(VISUAL_STATE_TRACKER_FEATURE_CLIENT),
        m_presentationOrchestratorStateTracker(std::move(presentationOrchestratorStateTracker)),
        m_shutdownManager(std::move(shutdownManager)) {
}

std::unique_ptr<VisualStateTrackerFeatureClient> VisualStateTrackerFeatureClient::create(
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>&
        endpointBuilder,
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "contextManager null"));
        return nullptr;
    }

    if (!endpointBuilder) {
        ACSDK_ERROR(LX("createFailed").d("reason", "endpointBuilder null"));
        return nullptr;
    }

    auto shutdownNotifier = acsdkShutdownManager::ShutdownNotifier::createShutdownNotifierInterface();
    auto shutdownManager = acsdkShutdownManager::ShutdownManager::createShutdownManagerInterface(shutdownNotifier);
    if (!shutdownNotifier || !shutdownManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "null shutdownManager"));
        return nullptr;
    }

    auto visualActivityTracker = afml::VisualActivityTracker::create(contextManager);
    if (!visualActivityTracker) {
        ACSDK_ERROR(LX("createFailed").d("reason", "null visualActivityTracker"));
        return nullptr;
    }

    endpointBuilder->withCapabilityConfiguration(visualActivityTracker);
    shutdownNotifier->addObserver(visualActivityTracker);

    auto poStateTrackerExports =
        presentationOrchestratorStateTracker::PresentationOrchestratorStateTrackerFactory::create(
            visualActivityTracker);
    if (!poStateTrackerExports.hasValue()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "unableToCreatePresentationOrchestratorStateTracker"));
        return nullptr;
    }
    auto poStateTrackerInterfaces = poStateTrackerExports.value();

    shutdownNotifier->addObserver(poStateTrackerInterfaces.requiresShutdown);

    if (sdkClientRegistry) {
        sdkClientRegistry->registerComponent(poStateTrackerInterfaces.presentationOrchestratorStateTrackerInterface);
        sdkClientRegistry->registerComponent(visualActivityTracker);
    }

    auto client = std::unique_ptr<VisualStateTrackerFeatureClient>(new VisualStateTrackerFeatureClient(
        std::move(poStateTrackerInterfaces.presentationOrchestratorStateTrackerInterface), std::move(shutdownManager)));

    return client;
}

VisualStateTrackerFeatureClient::~VisualStateTrackerFeatureClient() {
    shutdown();
}

std::shared_ptr<alexaClientSDK::presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>
VisualStateTrackerFeatureClient::getPresentationOrchestratorStateTracker() const {
    return m_presentationOrchestratorStateTracker;
}

bool VisualStateTrackerFeatureClient::configure(
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    /// No-op
    return true;
}

void VisualStateTrackerFeatureClient::doShutdown() {
    if (m_shutdownManager) {
        m_shutdownManager->shutdown();
    }

    m_shutdownManager.reset();
    m_presentationOrchestratorStateTracker.reset();
}

}  // namespace featureClient
}  // namespace alexaClientSDK
