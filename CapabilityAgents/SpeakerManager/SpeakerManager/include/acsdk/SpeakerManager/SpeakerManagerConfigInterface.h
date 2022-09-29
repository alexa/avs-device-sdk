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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_INCLUDE_ACSDK_SPEAKERMANAGER_SPEAKERMANAGERCONFIGINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_INCLUDE_ACSDK_SPEAKERMANAGER_SPEAKERMANAGERCONFIGINTERFACE_H_

#include <cstdint>

namespace alexaClientSDK {
namespace speakerManager {

struct SpeakerManagerStorageState;

/**
 * @brief Speaker manager configuration interface.
 *
 * This interface provides initial configuration for speaker manager capability agent.
 *
 * @see createSpeakerManagerConfig()
 *
 * @ingroup Lib_acsdkSpeakerManager
 */
struct SpeakerManagerConfigInterface {
    /**
     * @brief Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~SpeakerManagerConfigInterface() noexcept = default;

    /**
     * @brief Load persistent storage setting from platform configuration.
     *
     * @param[out] persistentStorage Flag if persistent storage is enabled for speaker settings.
     * @return True if value is loaded, false is value is not present, is out of range, or operation failed.
     */
    virtual bool getPersistentStorage(bool& persistentStorage) noexcept = 0;

    /**
     * @brief Load minimum unmute volume from platform configuration.
     *
     * @param[out] minUnmuteVolume Minimum volume for unmuting speakers. The value must be in range 0..100 inclusive.
     * @return True if value is loaded, false if value is not present, is out of range, or operation failed.
     */
    virtual bool getMinUnmuteVolume(std::uint8_t& minUnmuteVolume) noexcept = 0;

    /**
     * @brief Load minimum unmute volume from platform configuration.
     *
     * @param[out] restoreMuteState Flag if the speaker mute state must be preserved between sessions.
     * @return True if value is loaded, false if value is not present, is out of range, or operation failed.
     */
    virtual bool getRestoreMuteState(bool& restoreMuteState) noexcept = 0;

    /**
     * @brief Load minimum unmute volume from platform configuration.
     *
     * @param[out] defaultSpeakerVolume Default volume for speaker channel. The value must be in range 0..100 inclusive.
     * @return True if value is loaded, false if value is not present, is out of range, or operation failed.
     */
    virtual bool getDefaultSpeakerVolume(std::uint8_t& defaultSpeakerVolume) noexcept = 0;

    /**
     * @brief Load minimum unmute volume from platform configuration.
     *
     * @param[out] defaultAlertsVolume Default volume for alerts channel. The value must be in range 0..100 inclusive.
     * @return True if value is loaded, false if value is not present, is out of range, or operation failed.
     */
    virtual bool getDefaultAlertsVolume(std::uint8_t& defaultAlertsVolume) noexcept = 0;
};

}  // namespace speakerManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_INCLUDE_ACSDK_SPEAKERMANAGER_SPEAKERMANAGERCONFIGINTERFACE_H_
