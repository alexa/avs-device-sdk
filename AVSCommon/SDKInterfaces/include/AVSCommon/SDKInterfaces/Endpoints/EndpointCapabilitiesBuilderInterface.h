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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTCAPABILITIESBUILDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTCAPABILITIESBUILDERINTERFACE_H_

#include <list>
#include <memory>
#include <utility>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace endpoints {

/**
 * This class provides a mechanism through which Endpoint Capability Agents can be passed to the @c EndpointBuilder.
 *
 * @note The @c EndpointBuilder calls @c buildCapabilties() in withEndpointCapabilitiesBuilder() method with all
 * required dependencies to build the capability agents.
 *
 * @note The user must ensure thread safety of this class.
 */
class EndpointCapabilitiesBuilderInterface {
public:
    /**
     * Helper structure containing @c CapabilityConfiguration and @c DirectiveHandler.
     */
    struct Capability {
        avsCommon::avs::CapabilityConfiguration configuration;
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler;
    };

    /**
     * Default Destructor.
     */
    virtual ~EndpointCapabilitiesBuilderInterface() = default;

    /**
     * The method builds controller capabilities and returns a pair of lists containing @c Capability and lists
     * containing @c RequiresShutdown objects.
     * @param endpointId The endpoint ID
     * @param contextManager The @c ContextManager pointer.
     * @param responseSender The @c AlexaInterfaceMessageSender to send Alexa Interface messages.
     * @param exceptionSender The @c ExceptionEncounteredSender to send exception messages to AVS.
     * @return A pair of lists of @c Capability and @c RequiresShutdown objects
     */
    virtual std::pair<std::list<Capability>, std::list<std::shared_ptr<avsCommon::utils::RequiresShutdown>>>
    buildCapabilities(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender) = 0;
};

}  // namespace endpoints
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTCAPABILITIESBUILDERINTERFACE_H_
