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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_SPEAKERMANAGERCONFIGHELPER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_SPEAKERMANAGERCONFIGHELPER_H_

#include <acsdk/SpeakerManager/SpeakerManagerStorageInterface.h>
#include <acsdk/SpeakerManager/SpeakerManagerConfigInterface.h>

namespace alexaClientSDK {
namespace speakerManager {

/**
 * @brief Helper class to manage configuration operations for SpeakerManager CA.
 *
 * This class implements all configuration operations and merges logic of accessing different configuration sources.
 * SpeakerManager get configuration values from three sources: hardcoded values, platform configuration, and persistent
 * storage.
 *
 * @ingroup Lib_acsdkSpeakerManager
 */
class SpeakerManagerConfigHelper {
public:
    /**
     * Creates object.
     * @param[in] storage Storage interface.
     */
    SpeakerManagerConfigHelper(
        std::shared_ptr<SpeakerManagerConfigInterface> config,
        std::shared_ptr<SpeakerManagerStorageInterface> storage) noexcept;

    /**
     * Load configuration.
     *
     * This method always succeeds (assuming @a state is not @a nullptr), as it first tries to load configuration
     * from config storage, then from platform configuration files, and falls back to hardcoded values.
     *
     * @param[out] state Pointer to configuration container to fill with config values.
     */
    void loadState(SpeakerManagerStorageState& state) noexcept;

    /**
     * Saves configuration to to config storage.
     *
     * @param[in] state Configuration data to persist.
     *
     * @return Boolean that indicates operation success.
     */
    bool saveState(const SpeakerManagerStorageState& state) noexcept;

    /**
     * Loads if persistent storage is enabled from configuration. By default persistent storage is not enabled, but it
     * can be overridden by configuration.
     *
     * @return Returns configured value, where true indicates that the persistent storage flag has been retrieved, and
     * false indicates that a default shall be kept.
     */
    bool getPersistentStorage() const noexcept;

    /**
     * Loads minimum unmute volume level from platform configuration. The method tries to load the unmute value from
     * platform configuration files, and if it fails, it returns a hardcoded value.
     *
     * @return Minimum volume level to unmute speakers.
     */
    int getMinUnmuteVolume() const noexcept;

    /**
     * Loads mute state handling from configuration. By default the speaker manager sets the mute status to the value
     * prior to reboot, but this behaviour can be overridden by configuration.
     *
     * @return Returns configured value, where true indicates that mute status is configured according to the last
     * saved state, and "false" indicates a default shall be kept.
     */
    bool getRestoreMuteState() const noexcept;

private:
    /**
     * Default values that are used when no other configuration sources are available.
     */
    static const SpeakerManagerStorageState c_defaults;

    /// Reference to configuration config interface.
    const std::shared_ptr<SpeakerManagerConfigInterface> m_config;

    /// Reference to configuration storage interface.
    const std::shared_ptr<SpeakerManagerStorageInterface> m_storage;
};

}  // namespace speakerManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_SPEAKERMANAGERCONFIGHELPER_H_
