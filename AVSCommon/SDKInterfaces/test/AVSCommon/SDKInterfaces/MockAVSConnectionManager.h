/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKAVSCONNECTIONMANAGER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKAVSCONNECTIONMANAGER_H_

#include <gmock/gmock.h>

#include "AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace test {

/// Mock class that implements AVSCOnnectionManagerInterface.
class MockAVSConnectionManager : public AVSConnectionManagerInterface {
public:
    MOCK_METHOD0(enable, void());
    MOCK_METHOD0(disable, void());
    MOCK_METHOD0(isEnabled, bool());
    MOCK_METHOD0(reconnect, void());
    MOCK_CONST_METHOD0(isConnected, bool());
    MOCK_METHOD1(
        addMessageObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer));
    MOCK_METHOD1(
        removeMessageObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer));
    MOCK_METHOD1(addConnectionStatusObserver, void(std::shared_ptr<ConnectionStatusObserverInterface> observer));
    MOCK_METHOD1(removeConnectionStatusObserver, void(std::shared_ptr<ConnectionStatusObserverInterface> observer));
};
}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKAVSCONNECTIONMANAGER_H_
