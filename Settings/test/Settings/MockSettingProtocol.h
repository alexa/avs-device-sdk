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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKSETTINGPROTOCOL_H_
#define ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKSETTINGPROTOCOL_H_

#include <functional>
#include <string>

#include <gmock/gmock.h>

#include <Settings/SettingProtocolInterface.h>

namespace alexaClientSDK {
namespace settings {
namespace test {

/**
 * Mock class that implements @c SettingProtocolInterface.
 *
 * @note This is not thread safe and it runs the protocol synchronously to keep tests simple.
 */
class MockSettingProtocol : public SettingProtocolInterface {
public:
    MockSettingProtocol(const std::string& initialValue, bool applyChange, bool revertChange);

    /// @name SettingProtocolInterface methods
    /// @{
    SetSettingResult localChange(
        ApplyChangeFunction applyChange,
        RevertChangeFunction revertChange,
        SettingNotificationFunction notifyObservers) override;

    bool avsChange(
        ApplyChangeFunction applyChange,
        RevertChangeFunction revertChange,
        SettingNotificationFunction notifyObservers) override;

    bool restoreValue(ApplyDbChangeFunction applyChange, SettingNotificationFunction notifyObservers) override;
    /// @}

private:
    /// Stores the initial value used during restoreValue call.
    std::string m_initialValue;

    /// Flag that can be used to configure whether the protocol should call applyChange or not.
    bool m_applyChange;

    /// Flag that can be used to configure whether the protocol should call revert change or not.
    bool m_revertChange;
};

MockSettingProtocol::MockSettingProtocol(const std::string& initialValue, bool applyChange, bool revertChange) :
        m_initialValue{initialValue},
        m_applyChange{applyChange},
        m_revertChange{revertChange} {
}

SetSettingResult MockSettingProtocol::localChange(
    ApplyChangeFunction applyChange,
    RevertChangeFunction revertChange,
    SettingNotificationFunction notifyObservers) {
    if (m_applyChange) {
        applyChange();
    }

    if (m_revertChange) {
        revertChange();
    }

    notifyObservers(SettingNotifications::LOCAL_CHANGE);
    return SetSettingResult::ENQUEUED;
}

bool MockSettingProtocol::avsChange(
    ApplyChangeFunction applyChange,
    RevertChangeFunction revertChange,
    SettingNotificationFunction notifyObservers) {
    if (m_applyChange) {
        applyChange();
    }

    if (m_revertChange) {
        revertChange();
    }

    notifyObservers(SettingNotifications::AVS_CHANGE);
    return true;
}

bool MockSettingProtocol::restoreValue(ApplyDbChangeFunction applyChange, SettingNotificationFunction notifyObservers) {
    if (m_applyChange) {
        applyChange(m_initialValue);
    }
    return true;
}

}  // namespace test
}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_TEST_SETTINGS_MOCKSETTINGPROTOCOL_H_
