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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINT_H_

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/Endpoints/EndpointInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace endpoints {
namespace test {

/// Mocks an endpoint object.
class MockEndpoint : public EndpointInterface {
public:
    /// @name @c EndpointInterface methods to be mocked.
    /// @{
    MOCK_CONST_METHOD0(getEndpointId, EndpointIdentifier());
    MOCK_CONST_METHOD0(getAttributes, avs::AVSDiscoveryEndpointAttributes());
    MOCK_CONST_METHOD0(getCapabilityConfigurations, std::vector<avsCommon::avs::CapabilityConfiguration>());
    MOCK_CONST_METHOD0(
        getCapabilities,
        std::unordered_map<
            avsCommon::avs::CapabilityConfiguration,
            std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface>>());
    MOCK_METHOD1(update, bool(const std::shared_ptr<EndpointModificationData>& endpointModificationData));
    /// @}
};

}  // namespace test
}  // namespace endpoints
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINT_H_
