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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKSETTING_H_
#define ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKSETTING_H_

#include <functional>
#include <string>

#include <gmock/gmock.h>

#include <Settings/SettingInterface.h>

namespace alexaClientSDK {
namespace settings {
namespace test {

/**
 * Mock class that implements @c SettingInterface.
 */
template <typename ValueT>
class MockSetting : public SettingInterface<ValueT> {
public:
    /// @name Mock SettingInterface methods
    /// @{
    MOCK_METHOD1_T(setLocalChange, SetSettingResult(const ValueT& value));
    /// @}

    /**
     * Constructor.
     *
     * @param value Initial value of this setting.
     */
    MockSetting(const ValueT& value);
};

template <typename ValueT>
MockSetting<ValueT>::MockSetting(const ValueT& value) : SettingInterface<ValueT>(value) {
}

}  // namespace test
}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKSETTING_H_
