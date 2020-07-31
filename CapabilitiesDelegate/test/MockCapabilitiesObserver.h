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

#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_MOCKCAPABILITIESOBSERVER_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_MOCKCAPABILITIESOBSERVER_H_

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

/**
 * Mock class for @c CapabilitiesObserverInterface.
 */
class MockCapabilitiesObserver : public avsCommon::sdkInterfaces::CapabilitiesObserverInterface {
public:
    MOCK_METHOD4(
        onCapabilitiesStateChange,
        void(
            CapabilitiesObserverInterface::State,
            CapabilitiesObserverInterface::Error,
            const std::vector<std::string>&,
            const std::vector<std::string>&));
};

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_MOCKCAPABILITIESOBSERVER_H_
