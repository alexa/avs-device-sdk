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

#include <acsdk/SpeakerManager/private/SpeakerManager.h>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>

namespace alexaClientSDK {
namespace speakerManager {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::storage;
using namespace avsCommon::avs::speakerConstants;
using namespace avsCommon::utils::configuration;

/// String to identify log entries originating from this file.
/// @private
#define TAG "SpeakerManagerConfigHelper"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 * @private
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

const SpeakerManagerStorageState SpeakerManagerConfigHelper::c_defaults = {{DEFAULT_SPEAKER_VOLUME, false},
                                                                           {DEFAULT_ALERTS_VOLUME, false}};

SpeakerManagerConfigHelper::SpeakerManagerConfigHelper(
    std::shared_ptr<SpeakerManagerConfigInterface> config,
    std::shared_ptr<SpeakerManagerStorageInterface> storage) noexcept :
        m_config{std::move(config)},
        m_storage{std::move(storage)} {
}

int SpeakerManagerConfigHelper::getMinUnmuteVolume() const noexcept {
    std::uint8_t minUnmuteVolume;
    if (!m_config->getMinUnmuteVolume(minUnmuteVolume)) {
        minUnmuteVolume = MIN_UNMUTE_VOLUME;
    }
    return minUnmuteVolume;
}

bool SpeakerManagerConfigHelper::getPersistentStorage() const noexcept {
    bool speakerPersistentStorageSetting;
    if (!m_config->getPersistentStorage(speakerPersistentStorageSetting)) {
        speakerPersistentStorageSetting = false;
    }
    return speakerPersistentStorageSetting;
}

void SpeakerManagerConfigHelper::loadState(SpeakerManagerStorageState& state) noexcept {
    if (m_storage->loadState(state)) {
        return;
    }

    state = c_defaults;

    std::uint8_t tmp;
    if (m_config->getDefaultSpeakerVolume(tmp)) {
        state.speakerChannelState.channelVolume = tmp;
    }
    if (m_config->getDefaultAlertsVolume(tmp)) {
        state.alertsChannelState.channelVolume = tmp;
    }
}

bool SpeakerManagerConfigHelper::saveState(const SpeakerManagerStorageState& state) noexcept {
    return m_storage->saveState(state);
}

bool SpeakerManagerConfigHelper::getRestoreMuteState() const noexcept {
    bool restoreMuteState;
    if (!m_config->getRestoreMuteState(restoreMuteState)) {
        restoreMuteState = true;
    }
    return restoreMuteState;
}

}  // namespace speakerManager
}  // namespace alexaClientSDK
