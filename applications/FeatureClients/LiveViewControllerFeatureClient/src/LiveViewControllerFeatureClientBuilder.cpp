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

#include "acsdk/LiveViewControllerFeatureClient/LiveViewControllerFeatureClientBuilder.h"

#include <Alexa/AlexaInterfaceMessageSender.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>

namespace alexaClientSDK {
namespace featureClient {

/// String to identify log entries originating from this file.
#define TAG "LiveViewControllerFeatureClientBuilder"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Name to identify this feature client builder.
static const char LIVE_VIEW_CONTROLLER_FEATURE_CLIENT_BUILDER[] = "LiveViewControllerFeatureClientBuilder";

LiveViewControllerFeatureClientBuilder::LiveViewControllerFeatureClientBuilder(
    avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
    const std::shared_ptr<alexaClientSDK::alexaLiveViewControllerInterfaces::LiveViewControllerInterface>&
        liveViewController) :
        m_endpointId(std::move(endpointId)),
        m_liveViewController(liveViewController) {
    addRequiredType<alexaClientSDK::avsCommon::sdkInterfaces::AVSConnectionManagerInterface>();
    addRequiredType<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>();
    addRequiredType<alexaClientSDK::capabilityAgents::alexa::AlexaInterfaceMessageSender>();
    addRequiredType<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>();
    addRequiredType<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>();
}

std::shared_ptr<LiveViewControllerFeatureClient> LiveViewControllerFeatureClientBuilder::construct(
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    if (!sdkClientRegistry) {
        ACSDK_CRITICAL(LX("constructFailed").d("reason", "null SDKClientRegistry"));
        return nullptr;
    }

    auto connectionManager =
        sdkClientRegistry->getComponent<alexaClientSDK::avsCommon::sdkInterfaces::AVSConnectionManagerInterface>();
    auto contextManager =
        sdkClientRegistry->getComponent<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>();
    auto responseSender =
        sdkClientRegistry->getComponent<alexaClientSDK::capabilityAgents::alexa::AlexaInterfaceMessageSender>();
    auto exceptionSender =
        sdkClientRegistry
            ->getComponent<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>();
    auto endpointBuilder =
        sdkClientRegistry
            ->getComponent<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>();

    return LiveViewControllerFeatureClient::create(
        m_endpointId,
        m_liveViewController,
        connectionManager,
        contextManager,
        responseSender,
        exceptionSender,
        endpointBuilder);
}

std::unique_ptr<LiveViewControllerFeatureClientBuilder> LiveViewControllerFeatureClientBuilder::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
    const std::shared_ptr<alexaClientSDK::alexaLiveViewControllerInterfaces::LiveViewControllerInterface>&
        liveViewController) {
    return std::unique_ptr<LiveViewControllerFeatureClientBuilder>(
        new LiveViewControllerFeatureClientBuilder(endpointId, liveViewController));
}

std::string LiveViewControllerFeatureClientBuilder::name() {
    return LIVE_VIEW_CONTROLLER_FEATURE_CLIENT_BUILDER;
}
}  // namespace featureClient
}  // namespace alexaClientSDK
