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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINTBUILDER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINTBUILDER_H_

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace endpoints {
namespace test {

/// Mocks an endpoint builder.
class MockEndpointBuilder : public EndpointBuilderInterface {
public:
    /// @name @c EndpointBuilderInterface methods to be mocked.
    /// @{
    MOCK_METHOD1(withDerivedEndpointId, EndpointBuilderInterface&(const std::string& suffix));
    MOCK_METHOD1(withEndpointId, EndpointBuilderInterface&(const EndpointIdentifier& endpointId));
    MOCK_METHOD1(withFriendlyName, EndpointBuilderInterface&(const std::string& friendlyName));
    MOCK_METHOD1(withDescription, EndpointBuilderInterface&(const std::string& description));
    MOCK_METHOD1(withManufacturerName, EndpointBuilderInterface&(const std::string& manufacturerName));
    MOCK_METHOD1(withDisplayCategory, EndpointBuilderInterface&(const std::vector<std::string>& displayCategories));
    MOCK_METHOD6(
        withAdditionalAttributes,
        EndpointBuilderInterface&(
            const std::string& manufacturer,
            const std::string& model,
            const std::string& serialNumber,
            const std::string& firmwareVersion,
            const std::string& softwareVersion,
            const std::string& customIdentifier));
    MOCK_METHOD1(
        withConnections,
        EndpointBuilderInterface&(const std::vector<std::map<std::string, std::string>>& connections));
    MOCK_METHOD1(withCookies, EndpointBuilderInterface&(const std::map<std::string, std::string>& cookies));
    MOCK_METHOD0(build, utils::Optional<EndpointIdentifier>());
    /// @}
};

}  // namespace test
}  // namespace endpoints
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINTBUILDER_H_
