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

#include "acsdk/Sample/Endpoint/InputControllerEndpointCapabilitiesBuilder.h"

#include <acsdk/AlexaInputController/InputControllerFactory.h>
#include "acsdk/Sample/Endpoint/EndpointInputControllerHandler.h"
#include <AVSCommon/Utils/Logger/Logger.h>

/// String to identify log entries originating from this file.
#define TAG "InputControllerEndpointCapabilitiesBuilder"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

InputControllerEndpointCapabilitiesBuilder::InputControllerEndpointCapabilitiesBuilder() : m_capabilitiesBuilt{false} {
    ACSDK_DEBUG5(LX(__func__));
}

std::pair<
    std::list<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesBuilderInterface::Capability>,
    std::list<std::shared_ptr<alexaClientSDK::avsCommon::utils::RequiresShutdown>>>
InputControllerEndpointCapabilitiesBuilder::buildCapabilities(
    const alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>&
        responseSender,
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
        exceptionSender) {
    ACSDK_DEBUG5(LX(__func__));
    std::pair<
        std::list<
            alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesBuilderInterface::Capability>,
        std::list<std::shared_ptr<alexaClientSDK::avsCommon::utils::RequiresShutdown>>>
        retValue;
    std::list<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesBuilderInterface::Capability>
        capabilities;
    std::list<std::shared_ptr<alexaClientSDK::avsCommon::utils::RequiresShutdown>> requireShutdownObjects;
    std::lock_guard<std::mutex> lock(m_buildCapabilitiesMutex);
    if (m_capabilitiesBuilt) {
        ACSDK_ERROR(LX(__FUNCTION__).d("buildCapabilitiesFailed reason", "Capabilities already built!"));
        return retValue;
    }

    auto inputControllerHandler = EndpointInputControllerHandler::create(endpointId);
    if (inputControllerHandler) {
        auto inputControllerCapabilityAgent = alexaInputController::AlexaInputControllerFactory::create(
            endpointId, inputControllerHandler, contextManager, responseSender, exceptionSender, true, true);
        if (inputControllerCapabilityAgent.hasValue()) {
            auto configurations =
                inputControllerCapabilityAgent.value().capabilityConfigurationInterface->getCapabilityConfigurations();
            if (configurations.empty() || configurations.find(nullptr) != configurations.end()) {
                ACSDK_ERROR(LX("buildCapabilitiesFailed").d("reason", "invalidRemoteVideoPlayerConfiguration"));
            }
            for (auto& configurationPtr : configurations) {
                auto& configuration = *configurationPtr;
                capabilities.push_back({configuration, inputControllerCapabilityAgent.value().directiveHandler});
            }
            requireShutdownObjects.push_back(inputControllerCapabilityAgent.value().requiresShutdown);
        } else {
            ACSDK_ERROR(LX("Failed to create InputController CA!"));
        }
    } else {
        ACSDK_ERROR(LX("Failed to create InputController handler!"));
    }

    retValue.first.swap(capabilities);
    retValue.second.swap(requireShutdownObjects);
    m_capabilitiesBuilt = true;
    return retValue;
}
}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
