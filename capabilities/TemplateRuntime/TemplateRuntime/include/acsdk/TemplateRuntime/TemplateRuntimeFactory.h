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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIME_INCLUDE_ACSDK_TEMPLATERUNTIME_TEMPLATERUNTIMEFACTORY_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIME_INCLUDE_ACSDK_TEMPLATERUNTIME_TEMPLATERUNTIMEFACTORY_H_

#include <memory>

#include <acsdk/TemplateRuntimeInterfaces/TemplateRuntimeInterface.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderRegistrarInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace templateRuntime {

/**
 * This factory can be used to create a TemplateRuntimeFactory object which could be a parameter for Capability Agent
 * construction.
 */
class TemplateRuntimeFactory {
private:
    /**
     * This object contains the interfaces to interact with the TemplateRuntime Capability Agent.
     */
    struct TemplateRuntimeAgentData {
        /// An interface used to handle TemplateRuntime capability agent.
        std::shared_ptr<templateRuntimeInterfaces::TemplateRuntimeInterface> templateRuntime;
        /// Instance of @c RequiresShutdown used for cleaning up the capability agent during shutdown.
        std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown;
    };

public:
    /**
     * @brief Creates template runtime capability agent.
     *
     * Method creates a component that advertises template runtime capability. The component is responsible for
     * handling directives to render a template card and a music player.
     *
     * @see: https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/templateruntime.html
     *
     * @param renderPlayerInfoCardsProviderRegistrar The object with which to register this provider for playerInfo
     * cards.
     * @param endpointCapabilitiesRegistrar The @c EndpointCapabilitiesRegistrarInterface for the default endpoint.
     * @param exceptionSender Object used to send exceptions.
     *
     * @return An @c Optional object containing an instance of @c TemplateRuntimeAgentData object if successful.
     */
    static avsCommon::utils::Optional<TemplateRuntimeAgentData> create(
        const std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>&
            renderPlayerInfoCardsProviderRegistrar,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>&
            endpointCapabilitiesRegistrar);
};
}  // namespace templateRuntime
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIME_INCLUDE_ACSDK_TEMPLATERUNTIME_TEMPLATERUNTIMEFACTORY_H_
