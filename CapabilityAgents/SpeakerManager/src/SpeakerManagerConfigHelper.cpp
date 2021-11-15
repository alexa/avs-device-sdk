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

#include <SpeakerManager/SpeakerManager.h>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>

using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::sdkInterfaces::storage;
using namespace alexaClientSDK::avsCommon::avs::speakerConstants;
using namespace alexaClientSDK::capabilityAgents::speakerManager;
using namespace alexaClientSDK::avsCommon::utils::configuration;

/// String to identify log entries originating from this file.
static const std::string TAG{"SpeakerManagerConfigHelper"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param TAG Component tag.
 * @param evemt The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The key in our config file to find the root of speaker manager configuration.
static const std::string SPEAKERMANAGER_CONFIGURATION_ROOT_KEY = "speakerManagerCapabilityAgent";
/// The key in our config file to find the minUnmuteVolume value.
static const std::string SPEAKERMANAGER_MIN_UNMUTE_VOLUME_KEY = "minUnmuteVolume";
/// The key in our config file to find the defaultSpeakerVolume value.
static const std::string SPEAKERMANAGER_DEFAULT_SPEAKER_VOLUME_KEY = "defaultSpeakerVolume";
/// The key in our config file to find the defaultAlertsVolume value.
static const std::string SPEAKERMANAGER_DEFAULT_ALERTS_VOLUME_KEY = "defaultAlertsVolume";
/// The key in our config file to find mute status keep flag
static const std::string SPEAKERMANAGER_RESTORE_MUTE_STATE_KEY = "restoreMuteState";

const SpeakerManagerStorageState SpeakerManagerConfigHelper::c_defaults = {{DEFAULT_SPEAKER_VOLUME, false},
                                                                           {DEFAULT_ALERTS_VOLUME, false}};

SpeakerManagerConfigHelper::SpeakerManagerConfigHelper(const std::shared_ptr<SpeakerManagerStorageInterface>& storage) :
        m_storage(storage) {
}

int SpeakerManagerConfigHelper::getMinUnmuteVolume() const {
    int minUnmuteVolume = MIN_UNMUTE_VOLUME;

    auto node = ConfigurationNode::getRoot()[SPEAKERMANAGER_CONFIGURATION_ROOT_KEY];
    // If key is present, then read and initialize the value from config or set to default.
    node.getInt(SPEAKERMANAGER_MIN_UNMUTE_VOLUME_KEY, &minUnmuteVolume, MIN_UNMUTE_VOLUME);

    return minUnmuteVolume;
}

void SpeakerManagerConfigHelper::loadState(SpeakerManagerStorageState& state) {
    if (!m_storage->loadState(state) && !loadStateFromConfig(state)) {
        loadHardcodedState(state);
    }
}

bool SpeakerManagerConfigHelper::saveState(const SpeakerManagerStorageState& state) {
    return m_storage->saveState(state);
}

bool SpeakerManagerConfigHelper::loadStateFromConfig(SpeakerManagerStorageState& state) {
    int speakerVolume = 0;
    int alertsVolume = 0;

    auto node = ConfigurationNode::getRoot()[SPEAKERMANAGER_CONFIGURATION_ROOT_KEY];

    if (node.getInt(SPEAKERMANAGER_DEFAULT_SPEAKER_VOLUME_KEY, &speakerVolume) &&
        node.getInt(SPEAKERMANAGER_DEFAULT_ALERTS_VOLUME_KEY, &alertsVolume)) {
        state.speakerChannelState.channelMuteStatus = false;
        state.speakerChannelState.channelVolume = speakerVolume;
        state.alertsChannelState.channelMuteStatus = false;
        state.alertsChannelState.channelVolume = alertsVolume;

        return true;
    }
    return false;
}

void SpeakerManagerConfigHelper::loadHardcodedState(SpeakerManagerStorageState& state) {
    state = c_defaults;
}

bool SpeakerManagerConfigHelper::getRestoreMuteState() const {
    auto node = ConfigurationNode::getRoot()[SPEAKERMANAGER_CONFIGURATION_ROOT_KEY];
    bool result = false;
    if (node.getBool(SPEAKERMANAGER_RESTORE_MUTE_STATE_KEY, &result)) {
        return result;
    } else {
        return true;
    }
}
