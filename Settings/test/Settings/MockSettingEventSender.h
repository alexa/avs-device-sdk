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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKSETTINGEVENTSENDER_H_
#define ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKSETTINGEVENTSENDER_H_

#include <future>
#include <string>

#include <gmock/gmock.h>

#include <Settings/SettingEventSenderInterface.h>

namespace alexaClientSDK {
namespace settings {
namespace test {

/*
 * A utility class used to send events to AVS in the goal of synchronizing the value of an associated @c
 * SettingInterface value.
 *
 */
class MockSettingEventSender : public SettingEventSenderInterface {
public:
    /// @name SettingEventSenderInterface methods.
    /// @{
    MOCK_METHOD1(sendChangedEvent, std::shared_future<bool>(const std::string& value));
    MOCK_METHOD1(sendReportEvent, std::shared_future<bool>(const std::string& value));
    MOCK_METHOD1(sendStateReportEvent, std::shared_future<bool>(const std::string& payload));
    MOCK_METHOD0(cancel, void());
    /// @}
};

}  // namespace test
}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKSETTINGEVENTSENDER_H_
