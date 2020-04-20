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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKCAPABILITIESDELEGATE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKCAPABILITIESDELEGATE_H_

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace test {

/// Mock class that implements the @c CapabilitiesDelegateInterface.
class MockCapabilitiesDelegate : public CapabilitiesDelegateInterface {
public:
    MOCK_METHOD2(
        registerEndpoint,
        bool(
            const avsCommon::avs::AVSDiscoveryEndpointAttributes& endpointAttributes,
            const std::vector<avsCommon::avs::CapabilityConfiguration>& capabilities));
    MOCK_METHOD1(
        addCapabilitiesObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesObserverInterface> observer));
    MOCK_METHOD1(
        removeCapabilitiesObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesObserverInterface> observer));
    MOCK_METHOD0(invalidateCapabilities, void());
    MOCK_METHOD1(onAlexaEventProcessedReceived, void(const std::string& eventCorrelationToken));
    MOCK_METHOD1(onAVSGatewayChanged, void(const std::string& avsGateway));
};

}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKCAPABILITIESDELEGATE_H_
