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
#include <memory>
#include <AVSCommon/Utils/Logger/Logger.h>
#include "SpeakerManager/ChannelVolumeManager.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace speakerManager {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs::speakerConstants;

static const std::string TAG("ChannelVolumeManager");
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The fraction of maximum volume used for the upper threshold in the default volume curve.
static const float UPPER_VOLUME_CURVE_FRACTION = 0.40;

/// The fraction of maximum volume used for the lower threshold in the default volume curve.
static const float LOWER_VOLUME_CURVE_FRACTION = 0.20;

/**
 * Checks whether a value is within the bounds.
 *
 * @tparam T The class type of the input parameters.
 * @param value The value to check.
 * @param min The minimum value.
 * @param max The maximum value.
 */
template <class T>
static bool withinBounds(T value, T min, T max) {
    if (value < min || value > max) {
        ACSDK_ERROR(LX("checkBoundsFailed").d("value", value).d("min", min).d("max", max));
        return false;
    }
    return true;
}

std::shared_ptr<ChannelVolumeManager> ChannelVolumeManager::create(
    std::shared_ptr<SpeakerInterface> speaker,
    ChannelVolumeInterface::Type type,
    VolumeCurveFunction volumeCurve) {
    if (!speaker) {
        ACSDK_ERROR(LX(__func__).d("reason", "Null SpeakerInterface").m("createFailed"));
        return nullptr;
    }

    auto channelVolumeManager =
        std::shared_ptr<ChannelVolumeManager>(new ChannelVolumeManager(speaker, type, volumeCurve));

    /// Retrieve initial volume setting from underlying speakers
    SpeakerInterface::SpeakerSettings settings;
    if (!channelVolumeManager->getSpeakerSettings(&settings)) {
        ACSDK_ERROR(LX(__func__).m("createFailed").d("reason", "Unable To Retrieve Speaker Settings"));
        return nullptr;
    }
    channelVolumeManager->m_unduckedVolume = settings.volume;

    return channelVolumeManager;
}

ChannelVolumeManager::ChannelVolumeManager(
    std::shared_ptr<SpeakerInterface> speaker,
    ChannelVolumeInterface::Type type,
    VolumeCurveFunction volumeCurve) :
        ChannelVolumeInterface{},
        m_speaker{speaker},
        m_isDucked{false},
        m_unduckedVolume{AVS_SET_VOLUME_MIN},
        m_volumeCurveFunction{volumeCurve ? volumeCurve : defaultVolumeAttenuateFunction},
        m_type{type} {
}

ChannelVolumeInterface::Type ChannelVolumeManager::getSpeakerType() const {
    std::lock_guard<std::mutex> locker{m_mutex};
    return m_type;
}

bool ChannelVolumeManager::startDucking() {
    std::lock_guard<std::mutex> locker{m_mutex};
    ACSDK_DEBUG5(LX(__func__));
    if (m_isDucked) {
        ACSDK_WARN(LX(__func__).m("Channel is Already Attenuated"));
        return true;
    }

    // consult volume curve to determine the volume to duck to
    auto desiredVolume = m_volumeCurveFunction(m_unduckedVolume);

    ACSDK_DEBUG9(LX(__func__)
                     .d("currentVolume", static_cast<int>(m_unduckedVolume))
                     .d("desiredAttenuatedVolume", static_cast<int>(desiredVolume)));

    if (!m_speaker->setVolume(desiredVolume)) {
        ACSDK_WARN(LX(__func__).m("Failed to Attenuate Channel Volume"));
        return false;
    }

    m_isDucked = true;
    return true;
}

bool ChannelVolumeManager::stopDucking() {
    std::lock_guard<std::mutex> locker{m_mutex};
    ACSDK_DEBUG5(LX(__func__));

    // exit early if the channel is not attenuated
    if (!m_isDucked) {
        return true;
    }

    // Restore the speaker volume
    if (!m_speaker->setVolume(m_unduckedVolume)) {
        return false;
    }

    ACSDK_DEBUG5(LX(__func__).d("Restored Channel Volume", static_cast<int>(m_unduckedVolume)));
    m_isDucked = false;
    return true;
}

bool ChannelVolumeManager::setUnduckedVolume(int8_t volume) {
    ACSDK_DEBUG5(LX(__func__).d("volume", static_cast<int>(volume)));
    if (!withinBounds(volume, AVS_SET_VOLUME_MIN, AVS_SET_VOLUME_MAX)) {
        ACSDK_ERROR(LX(__func__).m("Invalid Volume"));
        return false;
    }

    std::lock_guard<std::mutex> locker{m_mutex};
    // store the volume
    m_unduckedVolume = volume;
    if (m_isDucked) {
        ACSDK_WARN(LX(__func__).m("Channel is Attenuated, Deferring Operation"));
        // New Volume shall be applied upon the next call to stopDucking()
        return true;
    }

    ACSDK_DEBUG5(LX(__func__).d("Unducked Channel Volume", static_cast<int>(volume)));
    return m_speaker->setVolume(m_unduckedVolume);
}

bool ChannelVolumeManager::setMute(bool mute) {
    std::lock_guard<std::mutex> locker{m_mutex};
    ACSDK_DEBUG5(LX(__func__).d("mute", static_cast<int>(mute)));
    return m_speaker->setMute(mute);
}

bool ChannelVolumeManager::getSpeakerSettings(
    avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings) const {
    ACSDK_DEBUG(LX(__func__));
    if (!settings) {
        return false;
    }

    std::lock_guard<std::mutex> locker{m_mutex};
    if (!m_speaker->getSpeakerSettings(settings)) {
        ACSDK_ERROR(LX(__func__).m("Unable To Retrieve SpeakerSettings"));
        return false;
    }

    // if the channel is ducked : return the cached latest unducked volume
    if (m_isDucked) {
        ACSDK_DEBUG5(LX(__func__).m("Channel is Already Attenuated"));
        settings->volume = m_unduckedVolume;
    }

    return true;
}

int8_t ChannelVolumeManager::defaultVolumeAttenuateFunction(int8_t currentVolume) {
    const int8_t lowerBreakPoint = static_cast<int8_t>(AVS_SET_VOLUME_MAX * LOWER_VOLUME_CURVE_FRACTION);
    const int8_t upperBreakPoint = static_cast<int8_t>(AVS_SET_VOLUME_MAX * UPPER_VOLUME_CURVE_FRACTION);

    if (currentVolume >= upperBreakPoint) {
        return lowerBreakPoint;
    } else if (currentVolume >= lowerBreakPoint && currentVolume <= upperBreakPoint) {
        return (currentVolume - lowerBreakPoint);
    } else {
        return avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MIN;
    }
}
}  // namespace speakerManager
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
