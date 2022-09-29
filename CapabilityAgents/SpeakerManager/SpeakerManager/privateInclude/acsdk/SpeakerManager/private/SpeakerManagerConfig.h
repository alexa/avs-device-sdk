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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_SPEAKERMANAGERCONFIG_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_SPEAKERMANAGERCONFIG_H_

#include <acsdk/SpeakerManager/SpeakerManagerConfigInterface.h>
#include <acsdk/SpeakerManager/SpeakerManagerStorageState.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace speakerManager {

/**
 * @brief Configuration interface for SpeakerManager.
 *
 * This class accesses configuration using @c ConfigurationNode facility. Internally class uses
 * "speakerManagerCapabilityAgent" child and looks up the following keys:
 * - "persistentStorage" -- Boolean flag that indicates if persistent storage is enabled.
 * - "minUnmuteVolume" -- Minimum volume level for unmuting the channel. This setting applies to all channel types.
 * - "defaultSpeakerVolume" -- Default speaker volume.
 * - "defaultAlertsVolume" -- Default alerts volume.
 * - "restoreMuteState" -- Boolean flag that indicates if mute state shall be preserved between device reboots.
 *
 * If AlexaClientSDKConfig.json configuration file is used, the example configuration may look like:
 * @code{.json}
 * {
 *    "speakerManagerCapabilityAgent": {
 *        "persistentStorage": true,
 *        "minUnmuteVolume": 10,
 *        "defaultSpeakerVolume": 40,
 *        "defaultAlertsVolume": 40,
 *        "restoreMuteState": true
 *    }
 * }
 * @endcode
 *
 * @ingroup Lib_acsdkSpeakerManager
 */
class SpeakerManagerConfig : public SpeakerManagerConfigInterface {
public:
    /**
     * @brief Construct object and load configuration.
     */
    SpeakerManagerConfig() noexcept;

    /// @name SpeakerManagerConfigInterface Functions
    /// @{
    bool getPersistentStorage(bool& persistentStorage) noexcept override;
    bool getMinUnmuteVolume(std::uint8_t& minUnmuteVolume) noexcept override;
    bool getRestoreMuteState(bool& restoreMuteState) noexcept override;
    bool getDefaultSpeakerVolume(std::uint8_t& defaultSpeakerVolume) noexcept override;
    bool getDefaultAlertsVolume(std::uint8_t& defaultAlertsVolume) noexcept override;
    /// @}
private:
    /**
     * @brief Load and validate values from platform configuration.
     */
    void loadPlatformConfig() noexcept;

    /// Flag if persistent storage is enabled for speaker settings.
    avsCommon::utils::Optional<bool> m_persistentStorage;

    /// Minimum volume for unmuting speakers. The value must be in range 0..100 inclusive.
    avsCommon::utils::Optional<std::uint8_t> m_minUnmuteVolume;

    /// Flag if the speaker mute state must be preserved between sessions.
    avsCommon::utils::Optional<bool> m_restoreMuteState;

    /// Default volume for speaker channel. The value must be in range 0..100 inclusive.
    avsCommon::utils::Optional<std::uint8_t> m_defaultSpeakerVolume;

    /// Default volume for alerts channel. The value must be in range 0..100 inclusive.
    avsCommon::utils::Optional<std::uint8_t> m_defaultAlertsVolume;
};

}  // namespace speakerManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_SPEAKERMANAGERCONFIG_H_
