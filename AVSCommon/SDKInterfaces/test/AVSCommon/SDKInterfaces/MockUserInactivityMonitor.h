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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKUSERINACTIVITYMONITOR_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKUSERINACTIVITYMONITOR_H_

#include "AVSCommon/SDKInterfaces/UserInactivityMonitorInterface.h"
#include "AVSCommon/SDKInterfaces/UserInactivityMonitorObserverInterface.h"
#include <gmock/gmock.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace test {

/// Mock class that implements @c UserInactivityMonitorInterface.
class MockUserInactivityMonitor : public UserInactivityMonitorInterface {
public:
    MOCK_METHOD0(onUserActive, void());
    MOCK_METHOD0(timeSinceUserActivity, std::chrono::seconds());
    MOCK_METHOD1(addObserver, void(std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorObserverInterface>));
    MOCK_METHOD1(
        removeObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorObserverInterface>));
};

}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKUSERINACTIVITYMONITOR_H_
