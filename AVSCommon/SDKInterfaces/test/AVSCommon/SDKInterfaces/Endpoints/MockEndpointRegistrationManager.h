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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINTREGISTRATIONMANAGER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINTREGISTRATIONMANAGER_H_

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/Endpoints/EndpointRegistrationManagerInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace endpoints {
namespace test {

/// Mocks an endpoint registration manager.
class MockEndpointRegistrationManager : public EndpointRegistrationManagerInterface {
public:
    /// @name @c EndpointRegistrationManagerInterface methods to be mocked.
    /// @{
    MOCK_METHOD1(registerEndpoint, std::future<RegistrationResult>(std::shared_ptr<EndpointInterface> endpoint));
    MOCK_METHOD1(deregisterEndpoint, std::future<DeregistrationResult>(const EndpointIdentifier& endpointId));
    MOCK_METHOD2(
        updateEndpoint,
        std::future<UpdateResult>(
            const EndpointIdentifier& endpointId,
            const std::shared_ptr<EndpointModificationData>& endpointModificationData));
    MOCK_METHOD1(addObserver, void(std::shared_ptr<EndpointRegistrationObserverInterface> observer));
    MOCK_METHOD1(removeObserver, void(const std::shared_ptr<EndpointRegistrationObserverInterface>& observer));
    /// @}
};

}  // namespace test
}  // namespace endpoints
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINTREGISTRATIONMANAGER_H_
