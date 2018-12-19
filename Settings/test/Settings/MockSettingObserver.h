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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKSETTINGOBSERVER_H_
#define ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKSETTINGOBSERVER_H_

#include <functional>
#include <memory>

#include <gmock/gmock.h>

#include <Settings/SettingObserverInterface.h>

namespace alexaClientSDK {
namespace settings {
namespace test {

/**
 * Mock setting observer.
 *
 * @tparam SettingT The setting type to be observed.
 */
template <typename SettingT>
class MockSettingObserver : public SettingObserverInterface<SettingT> {
public:
    /**
     * Function called when the observed setting is called.
     *
     * @param value The current value of the setting. For @c LOCAL_CHANGE and @c AVS_CHANGE, the value should be the
     * same as the one requested.
     * @param notification The notification type.
     */
    MOCK_METHOD2_T(
        onSettingNotification,
        void(const typename SettingT::ValueType& value, SettingNotifications notification));
};

}  // namespace test
}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKSETTINGOBSERVER_H_
