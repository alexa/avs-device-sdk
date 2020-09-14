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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINTREGISTRATIONOBSERVER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINTREGISTRATIONOBSERVER_H_

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/Endpoints/EndpointRegistrationObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace endpoints {
namespace test {

/// Mocks an endpoint registration observer.
class MockEndpointRegistrationObserver : public EndpointRegistrationObserverInterface {
public:
    /// @name @c EndpointRegistrationObserverInterface methods to be mocked.
    /// @{
    MOCK_METHOD3(
        onEndpointRegistration,
        void(
            const EndpointIdentifier& endpointId,
            const avs::AVSDiscoveryEndpointAttributes& attributes,
            const RegistrationResult result));
    MOCK_METHOD2(
        onEndpointDeregistration,
        void(const EndpointIdentifier& endpointId, const DeregistrationResult result));
    /// @}
};

}  // namespace test
}  // namespace endpoints
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_ENDPOINTS_MOCKENDPOINTREGISTRATIONOBSERVER_H_
