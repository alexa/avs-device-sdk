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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINTCAPABILITIESREGISTRAR_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINTCAPABILITIESREGISTRAR_H_

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace endpoints {
namespace test {

/// Mocks an endpoint capabilities registrar.
class MockEndpointCapabilitiesRegistrar : public EndpointCapabilitiesRegistrarInterface {
public:
    /// @name @c MockEndpointCapabilitiesRegistrarInterface methods to be mocked.
    /// @{
    MOCK_METHOD2(
        withCapability,
        EndpointCapabilitiesRegistrarInterface&(
            const avs::CapabilityConfiguration& configuration,
            std::shared_ptr<DirectiveHandlerInterface> directiveHandler));
    MOCK_METHOD2(
        withCapability,
        EndpointCapabilitiesRegistrarInterface&(
            const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>& configurationInterface,
            std::shared_ptr<DirectiveHandlerInterface> directiveHandler));
    MOCK_METHOD1(
        withCapabilityConfiguration,
        EndpointCapabilitiesRegistrarInterface&(
            const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>& configurationInterface));
    /// @}
};

}  // namespace test
}  // namespace endpoints
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINTCAPABILITIESREGISTRAR_H_
