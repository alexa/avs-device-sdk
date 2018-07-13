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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SETTINGS_INCLUDE_SETTINGS_SETTINGSUPDATEDEVENTSENDER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SETTINGS_INCLUDE_SETTINGS_SETTINGSUPDATEDEVENTSENDER_H_

#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/GlobalSettingsObserverInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace settings {

/**
 * This class sends the SettingsUpdated event to AVS when it receives a change in one or more settings.
 */
class SettingsUpdatedEventSender : public avsCommon::sdkInterfaces::GlobalSettingsObserverInterface {
public:
    /**
     * Creates a new @c SettingsUpdatedEventSender instance.
     * @param messageSender The object to send messages to AVS.
     */
    static std::unique_ptr<SettingsUpdatedEventSender> create(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    void onSettingChanged(const std::unordered_map<std::string, std::string>& mapOfSettings) override;

private:
    /**
     * Constructor for SettingsUpdatedEventSender.
     */
    SettingsUpdatedEventSender(std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /// Object to send messages to AVS.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;
};
}  // namespace settings
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SETTINGS_INCLUDE_SETTINGS_SETTINGSUPDATEDEVENTSENDER_H_
