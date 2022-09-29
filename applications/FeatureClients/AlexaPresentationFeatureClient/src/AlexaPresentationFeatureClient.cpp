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

#include <acsdk/AlexaPresentation/AlexaPresentationFactory.h>
#include <acsdk/AlexaPresentationAPL/AlexaPresentationAPLFactory.h>
#include <acsdkShutdownManager/ShutdownNotifier.h>
#include <acsdkShutdownManager/ShutdownManager.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>

#include "acsdk/AlexaPresentationFeatureClient/AlexaPresentationFeatureClient.h"

namespace alexaClientSDK {
namespace featureClient {
/// String to identify log entries originating from this file.
#define TAG "AlexaPresentationFeatureClient"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Name to identify this feature client
static const char ALEXA_PRESENTATION_FEATURE_CLIENT[] = "AlexaPresentationFeatureClient";

AlexaPresentationFeatureClient::AlexaPresentationFeatureClient(
    std::shared_ptr<alexaPresentationInterfaces::AlexaPresentationCapabilityAgentInterface> alexaPresentationCA,
    std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> aplCapabilityAgent,
    std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentNotifierInterface> aplCapabilityAgentNotifier,
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> shutdownManager) :
        FeatureClientInterface(ALEXA_PRESENTATION_FEATURE_CLIENT),
        m_alexaPresentationCA(std::move(alexaPresentationCA)),
        m_aplCapabilityAgent(std::move(aplCapabilityAgent)),
        m_aplCapabilityAgentNotifier(std::move(aplCapabilityAgentNotifier)),
        m_shutdownManager(std::move(shutdownManager)) {
}

std::unique_ptr<AlexaPresentationFeatureClient> AlexaPresentationFeatureClient::create(
    std::string aplVersion,
    std::shared_ptr<aplCapabilityCommonInterfaces::VisualStateProviderInterface> stateProviderInterface,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
        exceptionSender,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AVSConnectionManagerInterface>& connectionManager,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>&
        endpointBuilder,
    const std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "exceptionSender null"));
        return nullptr;
    }

    if (!connectionManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "connectionManager null"));
        return nullptr;
    }

    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "contextManager null"));
        return nullptr;
    }

    if (!endpointBuilder) {
        ACSDK_ERROR(LX("createFailed").d("reason", "defaultEndpointBuilder null"));
        return nullptr;
    }

    if (!stateProviderInterface) {
        ACSDK_ERROR(LX("createFailed").d("reason", "stateProviderInterface null"));
        return nullptr;
    }

    if (!metricRecorder) {
        ACSDK_DEBUG0(LX(__func__).m("metricRecorder null"));
    }

    auto shutdownNotifier = acsdkShutdownManager::ShutdownNotifier::createShutdownNotifierInterface();
    auto shutdownManager = acsdkShutdownManager::ShutdownManager::createShutdownManagerInterface(shutdownNotifier);
    if (!shutdownNotifier || !shutdownManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "null shutdownManager"));
        return nullptr;
    }

    /*
     * Creating the AlexaPresentation Capability Agent - This component is the Capability Agent that
     * implements the Alexa.Presentation AVS interface.
     */
    auto alexaPresentationCAExports =
        alexaPresentation::AlexaPresentationFactory::create(exceptionSender, connectionManager, contextManager);
    if (!alexaPresentationCAExports.hasValue()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "unableToCreateAlexaPresentationCapabilityAgent"));
        return nullptr;
    }

    auto alexaPresentationCA = alexaPresentationCAExports.value();

    endpointBuilder->withCapabilityConfiguration(alexaPresentationCA.capabilityConfiguration);
    shutdownNotifier->addObserver(alexaPresentationCA.requiresShutdown);

    /*
     * Creating the AlexaPresentationAPL Capability Agent - This component is the Capability Agent that
     * implements the Alexa.Presentation.APL AVS interface.
     */
    auto alexaPresentationAPLCAExports = aplCapabilityAgent::AlexaPresentationAPLFactory::create(
        exceptionSender, metricRecorder, connectionManager, contextManager, aplVersion, stateProviderInterface);
    if (!alexaPresentationAPLCAExports.hasValue()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "unableToCreateAlexaPresentationAPLCapabilityAgent"));
        return nullptr;
    }

    auto alexaPresentationAPLCA = alexaPresentationAPLCAExports.value();

    endpointBuilder->withCapability(
        alexaPresentationAPLCA.capabilityConfiguration, alexaPresentationAPLCA.capabilityAgent);
    shutdownNotifier->addObserver(alexaPresentationAPLCA.requiresShutdown);

    if (sdkClientRegistry) {
        sdkClientRegistry->registerComponent(alexaPresentationCA.alexaPresentation);
        sdkClientRegistry->registerComponent(alexaPresentationAPLCA.aplCapabilityAgent);
    } else {
        ACSDK_DEBUG0(LX(__func__).m("Null SDKClientRegistry, skipping component registration"));
    }

    auto client = std::unique_ptr<AlexaPresentationFeatureClient>(new AlexaPresentationFeatureClient(
        std::move(alexaPresentationCA.alexaPresentation),
        std::move(alexaPresentationAPLCA.aplCapabilityAgent),
        std::move(alexaPresentationAPLCA.capabilityAgentNotifier),
        std::move(shutdownManager)));

    return client;
}

std::shared_ptr<alexaPresentationInterfaces::AlexaPresentationCapabilityAgentInterface> AlexaPresentationFeatureClient::
    getAlexaPresentationCapabilityAgent() const {
    return m_alexaPresentationCA;
}

std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> AlexaPresentationFeatureClient::
    getAPLCapabilityAgent() const {
    return m_aplCapabilityAgent;
}

void AlexaPresentationFeatureClient::addAPLCapabilityAgentObserver(
    std::weak_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentObserverInterface> observer) {
    if (!m_aplCapabilityAgentNotifier) {
        ACSDK_ERROR(LX("addAlexaPresentationObserverFailed").d("reason", "null alexaPresentationAPLNotifier"));
        return;
    }
    m_aplCapabilityAgentNotifier->addWeakPtrObserver(observer);
}

void AlexaPresentationFeatureClient::removeAPLCapabilityAgentObserver(
    std::weak_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentObserverInterface> observer) {
    if (!m_aplCapabilityAgentNotifier) {
        ACSDK_ERROR(LX("removeAlexaPresentationObserverFailed").d("reason", "null alexaPresentationAPLNotifier"));
        return;
    }
    m_aplCapabilityAgentNotifier->removeWeakPtrObserver(observer);
}

AlexaPresentationFeatureClient::~AlexaPresentationFeatureClient() {
    shutdown();
}

bool AlexaPresentationFeatureClient::configure(
    const std::shared_ptr<alexaClientSDK::sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    // No-op
    return true;
}

void AlexaPresentationFeatureClient::doShutdown() {
    if (m_shutdownManager) {
        m_shutdownManager->shutdown();
        m_shutdownManager.reset();
    }
}

}  // namespace featureClient
}  // namespace alexaClientSDK