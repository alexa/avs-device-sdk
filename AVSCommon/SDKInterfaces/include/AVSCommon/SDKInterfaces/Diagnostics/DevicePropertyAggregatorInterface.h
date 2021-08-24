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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIAGNOSTICS_DEVICEPROPERTYAGGREGATORINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIAGNOSTICS_DEVICEPROPERTYAGGREGATORINTERFACE_H_

#include <string>
#include <unordered_map>

#include <acsdkAlertsInterfaces/AlertObserverInterface.h>
#include <acsdkAudioPlayerInterfaces/AudioPlayerObserverInterface.h>
#include <acsdkNotificationsInterfaces/NotificationsObserverInterface.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/PowerController/PowerControllerObserverInterface.h>
#include <AVSCommon/SDKInterfaces/RangeController/RangeControllerObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <Settings/DeviceSettingsManager.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace diagnostics {

/**
 * A class used to aggregate various states and properties of the device.
 */
class DevicePropertyAggregatorInterface
        : public acsdkAlertsInterfaces::AlertObserverInterface
        , public acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface
        , public avsCommon::sdkInterfaces::AuthObserverInterface
        , public avsCommon::sdkInterfaces::ConnectionStatusObserverInterface
        , public avsCommon::sdkInterfaces::ContextRequesterInterface
        , public acsdkNotificationsInterfaces::NotificationsObserverInterface
        , public avsCommon::sdkInterfaces::SpeakerManagerObserverInterface
        , public avsCommon::sdkInterfaces::DialogUXStateObserverInterface
        , public avsCommon::sdkInterfaces::rangeController::RangeControllerObserverInterface
        , public avsCommon::sdkInterfaces::powerController::PowerControllerObserverInterface {
public:
    /// Property Key to get Device Context. The Property Value is the json string containing the device context.
    static constexpr const char* DEVICE_CONTEXT = "DeviceContext";

    /// Property Key for Alerts state. The Property Value is a string of format "Alert:State". Ex: "ALARM:STARTED"
    static constexpr const char* ALERT_TYPE_AND_STATE = "AlertTypeAndState";

    /// Property Key for Audio Player State. The Property Value is a string representing audio player state. Ex:
    /// "PLAYING"
    static constexpr const char* AUDIO_PLAYER_STATE = "AudioPlayerState";

    /// Property Key for Audio Player Content ID. The Property Value is a content ID string.
    static constexpr const char* CONTENT_ID = "ContentId";

    /// Property Key for Connection State. The Property Value is a string representing connection state. Ex: "CONNECTED"
    static constexpr const char* CONNECTION_STATE = "ConnectionState";

    /// Property Key for Notification Indicator. The Property Value is a string representing notification state. Ex:
    /// "ON".
    static constexpr const char* NOTIFICATION_INDICATOR = "NotificationIndicator";

    /// Property Key for TTS Player State. The Property Value is a string representing the TTS state. Ex: "THINKING".
    static constexpr const char* TTS_PLAYER_STATE = "TTSPlayerState";

    /// Property Key for AVS Speaker Volume. The Property Value is a string representing the speaker volume. Ex: "25".
    static constexpr const char* AVS_SPEAKER_VOLUME = "AVSSpeakerVolume";

    /// Property Key for AVS Speaker Mute. The Property Value is a string representing if the speaker is muted. Ex:
    /// "true"
    static constexpr const char* AVS_SPEAKER_MUTE = "AVSSpeakerMute";

    /// Property Key for AVS Alerts Volume. The Property Value is a string representing the alerts volume. Ex: "50".
    static constexpr const char* AVS_ALERTS_VOLUME = "AVSAlertsVolume";

    /// Property Key for AVS Alerts Mute. The Property Value is a string representing if the alerts is muted. Ex:
    /// "false".
    static constexpr const char* AVS_ALERTS_MUTE = "AVSAlertsMute";

    /// Property Key for Do not Disturb. The Property Value is a string representing if do not disturb is on. Ex:
    /// "false".
    static constexpr const char* DO_NOT_DISTURB = "DoNotDisturb";

    /// Property Key for Locale. The Property Value is a string representing the locale of the device. Ex:
    /// "[en-US]".
    static constexpr const char* LOCALE = "Locale";

    /// Property Key for Wake Workds. The Property Value is a string representing the wake words configured on the
    /// device. Ex: "[ALEXA]"
    static constexpr const char* WAKE_WORDS = "WakeWords";

    /// Property Key for Registration status. The Property Value is a string representing the registration status. Ex:
    /// "true".
    static constexpr const char* REGISTRATION_STATUS = "RegistrationStatus";

    /// Property Key for Range Controller status. The Property Value is a string representing the range value of an
    /// instance. Ex: Instance FanSpeed: "7".
    static constexpr const char* RANGE_CONTROLLER_STATUS = "RangeControllerStatus";

    /// Property Key for Power Controller status. The Property Value is a string representing if power status is ON. Ex:
    /// "true".
    static constexpr const char* POWER_CONTROLLER_STATUS = "PowerControllerStatus";

    /**
     * Gets the property for the given property key.
     *
     * @param propertyKey The property key string.
     * @return The property value as an Optional.
     */
    virtual avsCommon::utils::Optional<std::string> getDeviceProperty(const std::string& propertyKey) = 0;

    /**
     * This method returns a list of all properties.
     *
     * @return A map of all properties.
     */
    virtual std::unordered_map<std::string, std::string> getAllDeviceProperties() = 0;

    /**
     * Set the @c ContextManagerInterface.
     *
     * @param contextManager The @c ContextManager.
     */
    virtual void setContextManager(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) = 0;

    /**
     * Set the @c DeviceSettingsManager
     *
     * @param settingManager the @c DeviceSettingsManager
     */
    virtual void setDeviceSettingsManager(std::shared_ptr<settings::DeviceSettingsManager> settingManager) = 0;
    /**
     * @note This API should only be used to initialize the volume values. Subsequent updates to volume values should
     * come from the @c SpeakerManagerInterface.
     *
     * @param speakerManager The @c SpeakerManagerInterface to initialize volume values.
     */
    virtual void initializeVolume(
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager) = 0;
};

}  // namespace diagnostics
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIAGNOSTICS_DEVICEPROPERTYAGGREGATORINTERFACE_H_
