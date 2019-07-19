/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKDEVICESETTINGSTORAGE_H_
#define ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKDEVICESETTINGSTORAGE_H_

#include <gmock/gmock.h>

#include <Settings/Storage/DeviceSettingStorageInterface.h>

namespace alexaClientSDK {
namespace settings {
namespace storage {
namespace test {

/**
 * Mock class of @c DeviceSettingStorageInterface.
 */
class MockDeviceSettingStorage : public DeviceSettingStorageInterface {
public:
    MOCK_METHOD0(open, bool());
    MOCK_METHOD0(close, void());
    MOCK_METHOD3(storeSetting, bool(const std::string& key, const std::string& value, SettingStatus status));
    MOCK_METHOD1(loadSetting, SettingStatusAndValue(const std::string& key));
    MOCK_METHOD1(deleteSetting, bool(const std::string& key));
    MOCK_METHOD2(updateSettingStatus, bool(const std::string& key, SettingStatus status));
};

}  // namespace test
}  // namespace storage
}  // namespace settings
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKDEVICESETTINGSTORAGE_H_
