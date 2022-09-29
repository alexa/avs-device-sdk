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
#ifndef ACSDK_SAMPLE_ENDPOINT_INPUTCONTROLLERENDPOINTCAPABILITIESBUILDER_H_
#define ACSDK_SAMPLE_ENDPOINT_INPUTCONTROLLERENDPOINTCAPABILITIESBUILDER_H_

#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesBuilderInterface.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * An implementation of a @c EndpointCapabilitiesBuilderInterface.
 */
class InputControllerEndpointCapabilitiesBuilder
        : public alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesBuilderInterface {
public:
    /**
     * Constructor.
     */
    InputControllerEndpointCapabilitiesBuilder();
    /// @name @c EndpointCapabilitiesBuilderInterface methods.
    /// @{
    std::pair<std::list<Capability>, std::list<std::shared_ptr<alexaClientSDK::avsCommon::utils::RequiresShutdown>>>
    buildCapabilities(
        const alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>&
            responseSender,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
            exceptionSender) override;
    /// @}
private:
    /// Lock to protect concurrent calls to PeripheralEndpointCapabilitiesBuilder class
    std::mutex m_buildCapabilitiesMutex;
    /// Flag to check if capabilities have already been built
    bool m_capabilitiesBuilt;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_ENDPOINT_INPUTCONTROLLERENDPOINTCAPABILITIESBUILDER_H_
