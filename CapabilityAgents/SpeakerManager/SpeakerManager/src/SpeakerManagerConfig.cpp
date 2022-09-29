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

#include <acsdk/SpeakerManager/private/SpeakerManagerConfig.h>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

namespace alexaClientSDK {
namespace speakerManager {

using namespace alexaClientSDK::avsCommon::avs::speakerConstants;
using namespace alexaClientSDK::avsCommon::utils::configuration;
using namespace alexaClientSDK::avsCommon::utils;

/// @addtogroup Lib_acsdkSpeakerManager
/// @{

/// String to identify log entries originating from this file.
/// @private
#define TAG "SpeakerManagerConfig"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 * @private
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The key in our config file to find the root of speaker manager configuration.
/// @private
static const auto SPEAKERMANAGER_CONFIGURATION_ROOT_KEY = "speakerManagerCapabilityAgent";
/// The key in our config file to find the persistentStorage value.
/// @private
static const auto SPEAKERMANAGER_PERSISTENT_STORAGE_SETTING_KEY = "persistentStorage";
/// The key in our config file to find the minUnmuteVolume value.
/// @private
static const auto SPEAKERMANAGER_MIN_UNMUTE_VOLUME_KEY = "minUnmuteVolume";
/// The key in our config file to find the defaultSpeakerVolume value.
/// @private
static const auto SPEAKERMANAGER_DEFAULT_SPEAKER_VOLUME_KEY = "defaultSpeakerVolume";
/// The key in our config file to find the defaultAlertsVolume value.
/// @private
static const auto SPEAKERMANAGER_DEFAULT_ALERTS_VOLUME_KEY = "defaultAlertsVolume";
/// The key in our config file to find mute status keep flag
/// @private
static const auto SPEAKERMANAGER_RESTORE_MUTE_STATE_KEY = "restoreMuteState";

/**
 * @brief Helper to load a single volume level entry from configuration.
 *
 * @param[in]  node  Configuration node.
 * @param[in]  key   Entry name.
 * @param[out] value Volume entry result.
 *
 * @return True if entry with name @a key have been found, and it's value is within valid range. False otherwise.
 * @private
 */
static bool loadVolumeConfig(ConfigurationNode& node, const char* key, Optional<std::uint8_t>& value) noexcept {
    int tmp;
    if (node.getInt(key, &tmp) && tmp >= AVS_SET_VOLUME_MIN && tmp <= AVS_SET_VOLUME_MAX) {
        value = static_cast<std::uint8_t>(tmp);
        return true;
    } else {
        return false;
    }
}

/// @}

SpeakerManagerConfig::SpeakerManagerConfig() noexcept {
    loadPlatformConfig();
}

bool SpeakerManagerConfig::getPersistentStorage(bool& persistentStorage) noexcept {
    if (m_persistentStorage.hasValue()) {
        persistentStorage = m_persistentStorage.value();
        return true;
    }
    return false;
}

bool SpeakerManagerConfig::getMinUnmuteVolume(std::uint8_t& minUnmuteVolume) noexcept {
    if (m_minUnmuteVolume.hasValue()) {
        minUnmuteVolume = m_minUnmuteVolume.value();
        return true;
    }
    return false;
}

bool SpeakerManagerConfig::getRestoreMuteState(bool& restoreMuteState) noexcept {
    if (m_restoreMuteState.hasValue()) {
        restoreMuteState = m_restoreMuteState.value();
        return true;
    }
    return false;
}

bool SpeakerManagerConfig::getDefaultSpeakerVolume(std::uint8_t& defaultSpeakerVolume) noexcept {
    if (m_defaultSpeakerVolume.hasValue()) {
        defaultSpeakerVolume = m_defaultSpeakerVolume.value();
        return true;
    }
    return false;
}

bool SpeakerManagerConfig::getDefaultAlertsVolume(std::uint8_t& defaultAlertsVolume) noexcept {
    if (m_defaultAlertsVolume.hasValue()) {
        defaultAlertsVolume = m_defaultAlertsVolume.value();
        return true;
    }
    return false;
}

void SpeakerManagerConfig::loadPlatformConfig() noexcept {
    auto configRoot = ConfigurationNode::getRoot().getChildNode(SPEAKERMANAGER_CONFIGURATION_ROOT_KEY);

    loadVolumeConfig(configRoot, SPEAKERMANAGER_DEFAULT_SPEAKER_VOLUME_KEY, m_defaultSpeakerVolume);
    loadVolumeConfig(configRoot, SPEAKERMANAGER_DEFAULT_ALERTS_VOLUME_KEY, m_defaultAlertsVolume);
    loadVolumeConfig(configRoot, SPEAKERMANAGER_MIN_UNMUTE_VOLUME_KEY, m_minUnmuteVolume);

    bool tmp;
    if (configRoot.getBool(SPEAKERMANAGER_RESTORE_MUTE_STATE_KEY, &tmp)) {
        m_restoreMuteState = tmp;
    }
    if (configRoot.getBool(SPEAKERMANAGER_PERSISTENT_STORAGE_SETTING_KEY, &tmp)) {
        m_persistentStorage = tmp;
    }
}

}  // namespace speakerManager
}  // namespace alexaClientSDK
