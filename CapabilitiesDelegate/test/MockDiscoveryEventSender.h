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

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_MOCKDISCOVERYEVENTSENDER_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_MOCKDISCOVERYEVENTSENDER_H_

#include <CapabilitiesDelegate/DiscoveryEventSenderInterface.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

/**
 * Mock class for @c DiscoveryEventSenderInterface.
 */
class MockDiscoveryEventSender : public DiscoveryEventSenderInterface {
public:
    MOCK_METHOD0(stop, void());
    MOCK_METHOD1(sendDiscoveryEvents, bool(const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>&));
    MOCK_METHOD1(addDiscoveryStatusObserver, void(const std::shared_ptr<DiscoveryStatusObserverInterface>&));
    MOCK_METHOD1(removeDiscoveryStatusObserver, void(const std::shared_ptr<DiscoveryStatusObserverInterface>&));
    MOCK_METHOD1(onAlexaEventProcessedReceived, void(const std::string&));
    MOCK_METHOD2(onAuthStateChange, void(AuthObserverInterface::State, AuthObserverInterface::Error));
};

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_MOCKDISCOVERYEVENTSENDER_H_
