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

#include <acsdk/AlexaLiveViewController/AlexaLiveViewControllerFactory.h>
#include <acsdkShutdownManager/ShutdownNotifier.h>
#include <acsdkShutdownManager/ShutdownManager.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <RTCSessionController/RtcscCapabilityAgent.h>

#include "acsdk/LiveViewControllerFeatureClient/LiveViewControllerFeatureClient.h"

namespace alexaClientSDK {
namespace featureClient {

/// String to identify log entries originating from this file.
#define TAG "LiveViewControllerFeatureClient"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Name to identify this feature client
static const char LIVE_VIEW_CONTROLLER_FEATURE_CLIENT[] = "LiveViewControllerFeatureClient";

LiveViewControllerFeatureClient::LiveViewControllerFeatureClient(
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> shutdownManager) :
        FeatureClientInterface(LIVE_VIEW_CONTROLLER_FEATURE_CLIENT),
        m_shutdownManager(std::move(shutdownManager)) {
}

std::unique_ptr<LiveViewControllerFeatureClient> LiveViewControllerFeatureClient::create(
    avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
    const std::shared_ptr<alexaClientSDK::alexaLiveViewControllerInterfaces::LiveViewControllerInterface>&
        liveViewController,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AVSConnectionManagerInterface>& connectionManager,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<alexaClientSDK::capabilityAgents::alexa::AlexaInterfaceMessageSender>& responseSender,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
        exceptionSender,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>&
        endpointBuilder) {
    if (!liveViewController) {
        ACSDK_ERROR(LX("createFailed").d("reason", "liveViewController null"));
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

    if (!responseSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "responseSender null"));
        return nullptr;
    }

    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "exceptionSender null"));
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

    /*
     * Creating the RTCSC Capability Agent - This component is the Capability Agent that implements the
     * media plane of the live view camera experience.
     */
    auto rtcscCapabilityAgent = capabilityAgents::rtcscCapabilityAgent::RtcscCapabilityAgent::create(
        connectionManager, contextManager, exceptionSender);
    if (!rtcscCapabilityAgent) {
        ACSDK_ERROR(LX("createFailed").m("Unable to create RTCSCCapabilityAgent"));
        return nullptr;
    }

    endpointBuilder->withCapability(rtcscCapabilityAgent, rtcscCapabilityAgent);
    shutdownNotifier->addObserver(rtcscCapabilityAgent);

    /*
     * Creating the LiveViewController Capability Agent - This component is the Capability Agent that
     * implements the Alexa.Camera.LiveViewController AVS interface.
     */
    auto liveViewControllerCAExports = alexaLiveViewController::AlexaLiveViewControllerFactory::create(
        endpointId, liveViewController, connectionManager, contextManager, responseSender, exceptionSender);
    if (!liveViewControllerCAExports.hasValue()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "unableToCreateLiveViewControllerCapabilityAgent"));
        return nullptr;
    }

    auto liveViewControllerCA = liveViewControllerCAExports.value();

    endpointBuilder->withCapability(
        liveViewControllerCA.capabilityConfiguration, liveViewControllerCA.directiveHandler);
    shutdownNotifier->addObserver(liveViewControllerCA.requiresShutdown);
    liveViewController->addObserver(liveViewControllerCA.liveViewControllerObserver);

    auto client = std::unique_ptr<LiveViewControllerFeatureClient>(
        new LiveViewControllerFeatureClient(std::move(shutdownManager)));

    return client;
}

LiveViewControllerFeatureClient::~LiveViewControllerFeatureClient() {
    shutdown();
}

bool LiveViewControllerFeatureClient::configure(
    const std::shared_ptr<alexaClientSDK::sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    // No-op
    return true;
}

void LiveViewControllerFeatureClient::doShutdown() {
    if (m_shutdownManager) {
        m_shutdownManager->shutdown();
        m_shutdownManager.reset();
    }
}

}  // namespace featureClient
}  // namespace alexaClientSDK
