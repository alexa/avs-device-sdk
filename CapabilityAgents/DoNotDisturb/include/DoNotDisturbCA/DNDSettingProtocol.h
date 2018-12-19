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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_DONOTDISTURB_INCLUDE_DONOTDISTURBCA_DNDSETTINGPROTOCOL_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_DONOTDISTURB_INCLUDE_DONOTDISTURBCA_DNDSETTINGPROTOCOL_H_

#include <memory>
#include <string>
#include <utility>

#include <AVSCommon/Utils/Threading/Executor.h>
#include <Settings/Storage/DeviceSettingStorageInterface.h>
#include <Settings/SetSettingResult.h>
#include <Settings/SettingEventMetadata.h>
#include <Settings/SettingEventSenderInterface.h>
#include <Settings/SettingObserverInterface.h>
#include <Settings/SettingProtocolInterface.h>
#include <Settings/SettingStatus.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace doNotDisturb {

/**
 * Implement the logic of DoNotDisturb protocol where AVS is the source of truth. This implementation corresponds to
 * Alexa.DoNotDisturb v1.0 AVS interface.
 */
class DNDSettingProtocol : public settings::SettingProtocolInterface {
public:
    /**
     * Create a protocol object.
     *
     * @param metadata The setting metadata used to generate a unique database key.
     * @param eventSender Object used to send events to avs in order to report changes to the device.
     * @param settingStorage The setting storage object.
     * @return A pointer to the new @c DNDSettingProtocol object if it succeeds; @c nullptr otherwise.
     */
    static std::unique_ptr<DNDSettingProtocol> create(
        const settings::SettingEventMetadata& metadata,
        std::shared_ptr<settings::SettingEventSenderInterface> eventSender,
        std::shared_ptr<settings::storage::DeviceSettingStorageInterface> settingStorage);

    /// @name SettingProtocolInterface methods.
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
    ///@}

    /**
     * Destructor.
     */
    ~DNDSettingProtocol() = default;

private:
    /**
     * Constructor.
     * @param settingKey The setting key used to access the setting storage.
     * @param eventSender Object used to send events to avs in order to report changes to the device.
     * @param settingStorage The setting storage object.
     */
    DNDSettingProtocol(
        const std::string& settingKey,
        std::shared_ptr<settings::SettingEventSenderInterface> eventSender,
        std::shared_ptr<settings::storage::DeviceSettingStorageInterface> settingStorage);

    /// The setting key used to access the setting storage.
    const std::string m_key;

    /// Object used to send events to avs in order to report changes to the device.
    std::shared_ptr<settings::SettingEventSenderInterface> m_eventSender;

    /// The setting storage object.
    std::shared_ptr<settings::storage::DeviceSettingStorageInterface> m_storage;

    /// Executor used to handle events in sequence.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace doNotDisturb
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_DONOTDISTURB_INCLUDE_DONOTDISTURBCA_DNDSETTINGPROTOCOL_H_
