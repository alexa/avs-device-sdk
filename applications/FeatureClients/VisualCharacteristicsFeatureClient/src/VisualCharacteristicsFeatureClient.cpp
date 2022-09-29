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

#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateTrackerInterface.h>
#include <acsdk/VisualCharacteristics/VisualCharacteristicsFactory.h>
#include <acsdk/VisualCharacteristics/VisualCharacteristicsSerializerFactory.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <acsdkShutdownManager/ShutdownManager.h>
#include <acsdkShutdownManager/ShutdownNotifier.h>

#include "acsdk/VisualCharacteristicsFeatureClient/VisualCharacteristicsFeatureClient.h"

namespace alexaClientSDK {
namespace featureClient {
/// String to identify log entries originating from this file.
#define TAG "VisualCharacteristicsFeatureClient"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// String used to identify this feature client
static const char VISUAL_CHARACTERISTICS_FEATURE_CLIENT[] = "VisualCharacteristicsFeatureClient";

VisualCharacteristicsFeatureClient::VisualCharacteristicsFeatureClient(
    std::shared_ptr<visualCharacteristicsInterfaces::VisualCharacteristicsInterface> visualCharacteristics,
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateObserverInterface>
        visualCharacteristicsPOStateObserver,
    std::shared_ptr<visualCharacteristicsInterfaces::VisualCharacteristicsSerializerInterface>
        visualCharacteristicsSerializer,
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> shutdownManager) :
        FeatureClientInterface(VISUAL_CHARACTERISTICS_FEATURE_CLIENT),
        m_visualCharacteristics(std::move(visualCharacteristics)),
        m_visualCharacteristicsPOStateObserver(std::move(visualCharacteristicsPOStateObserver)),
        m_visualCharacteristicsSerializer(std::move(visualCharacteristicsSerializer)),
        m_shutdownManager(std::move(shutdownManager)) {
}

std::unique_ptr<VisualCharacteristicsFeatureClient> VisualCharacteristicsFeatureClient::create(
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
        exceptionSender,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>&
        endpointBuilder,
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    if (!exceptionSender) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "exceptionSender null"));
        return nullptr;
    }

    if (!contextManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "contextManager null"));
        return nullptr;
    }

    if (!endpointBuilder) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "defaultEndpointBuilder null"));
        return nullptr;
    }

    auto shutdownNotifier = acsdkShutdownManager::ShutdownNotifier::createShutdownNotifierInterface();
    auto shutdownManager = acsdkShutdownManager::ShutdownManager::createShutdownManagerInterface(shutdownNotifier);
    if (!shutdownNotifier || !shutdownManager) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "null shutdownManager"));
        return nullptr;
    }

    /*
     * Creating the VisualCharacteristics Capability Agent - This component is the Capability Agent that
     * publish Alexa.Display, Alexa.Display.Window, Alexa.InteractionMode interfaces.
     */
    auto visualCharacteristicsExports =
        visualCharacteristics::VisualCharacteristicsFactory::create(contextManager, exceptionSender);
    if (!visualCharacteristicsExports.hasValue()) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "null VisualCharacteristics"));
        return nullptr;
    }
    auto visualCharacteristicsInterfaces = visualCharacteristicsExports.value();

    endpointBuilder->withCapabilityConfiguration(visualCharacteristicsInterfaces.capabilityConfigurationInterface);

    shutdownNotifier->addObserver(visualCharacteristicsInterfaces.requiresShutdown);

    /**
     * Creating the VisualCharacteristics serializer
     */
    auto visualCharacteristicsSerializer = visualCharacteristics::VisualCharacteristicsSerializerFactory::create();
    if (!visualCharacteristicsSerializer) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToFetchVisualCharacteristicsSerializer"));
        return nullptr;
    }

    if (sdkClientRegistry) {
        sdkClientRegistry->registerComponent(visualCharacteristicsInterfaces.visualCharacteristicsInterface);
        sdkClientRegistry->registerComponent(visualCharacteristicsSerializer);
    }

    auto client = std::unique_ptr<VisualCharacteristicsFeatureClient>(new VisualCharacteristicsFeatureClient(
        std::move(visualCharacteristicsInterfaces.visualCharacteristicsInterface),
        std::move(visualCharacteristicsInterfaces.presentationOrchestratorStateObserverInterface),
        std::move(visualCharacteristicsSerializer),
        std::move(shutdownManager)));

    return client;
}

VisualCharacteristicsFeatureClient::~VisualCharacteristicsFeatureClient() {
    shutdown();
}

std::shared_ptr<visualCharacteristicsInterfaces::VisualCharacteristicsInterface> VisualCharacteristicsFeatureClient::
    getVisualCharacteristics() const {
    return m_visualCharacteristics;
}

std::shared_ptr<visualCharacteristicsInterfaces::VisualCharacteristicsSerializerInterface>
VisualCharacteristicsFeatureClient::getVisualCharacteristicsSerializer() const {
    return m_visualCharacteristicsSerializer;
}

bool VisualCharacteristicsFeatureClient::configure(
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    if (!sdkClientRegistry) {
        ACSDK_ERROR(LX("configureFailed").d("reason", "null client registry"));
        return false;
    }

    auto presentationOrchestratorStateTracker =
        sdkClientRegistry
            ->getComponent<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>();
    if (!presentationOrchestratorStateTracker) {
        ACSDK_DEBUG0(LX("initialize").m("Building without Presentation Orchestrator support"));
    }

    if (presentationOrchestratorStateTracker) {
        presentationOrchestratorStateTracker->addStateObserver(m_visualCharacteristicsPOStateObserver);
    }
    return true;
}

void VisualCharacteristicsFeatureClient::doShutdown() {
    if (m_shutdownManager) {
        m_shutdownManager->shutdown();
        m_shutdownManager.reset();
    }
}

}  // namespace featureClient
}  // namespace alexaClientSDK
