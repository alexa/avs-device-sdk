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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_TIMING_MOCKTIMERDELEGATEFACTORY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_TIMING_MOCKTIMERDELEGATEFACTORY_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/Timing/TimerDelegateFactoryInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace test {

/// A mock of @c TimerDelegateFactoryInterface.
class MockTimerDelegateFactory : public timing::TimerDelegateFactoryInterface {
public:
    MOCK_METHOD0(supportsLowPowerMode, bool());
    MOCK_METHOD0(getTimerDelegate, std::unique_ptr<timing::TimerDelegateInterface>());
};

}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_TIMING_MOCKTIMERDELEGATEFACTORY_H_