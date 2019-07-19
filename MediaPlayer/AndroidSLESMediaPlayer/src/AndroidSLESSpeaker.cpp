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

#include <algorithm>
#include <cmath>

#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>

#include "AndroidSLESMediaPlayer/AndroidSLESSpeaker.h"

/// String to identify log entries originating from this file.
static const std::string TAG("AndroidSLESSpeaker");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

using namespace avsCommon::avs::speakerConstants;

/// Conversion factor used to convert the avs volume level to millibel.
const float CONVERSION_FACTOR = 20.0;

/// The AVS volume range.
static const auto AVS_VOLUME_RANGE = AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN;

/// Assert that the AVS maximum volume is greater than the minimum volume.
static_assert((AVS_VOLUME_RANGE) > 0, "Invalid volume range.");

/// The expected maximum device volume. OpenSL ES determines that device max volume is 0 or above. For android, the max
/// value is 0. For simplicity. we based our conversion on that assumption.
static constexpr SLmillibel DEVICE_MAX_VOLUME = 0;

/// There is no predefined minimum value. Use the minimum value allowed by the @c SLmillibel type.
static constexpr SLmillibel DEVICE_MIN_VOLUME = SL_MILLIBEL_MIN;

std::unique_ptr<AndroidSLESSpeaker> AndroidSLESSpeaker::create(
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESEngine> engine,
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> outputMixObject,
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> speakerObject,
    avsCommon::sdkInterfaces::SpeakerInterface::Type type) {
    if (!engine) {
        ACSDK_ERROR(LX("createFailed").d("reason", "invalidEngine"));
        return nullptr;
    }

    if (!outputMixObject) {
        ACSDK_ERROR(LX("createFailed").d("reason", "invalidOutputMix"));
        return nullptr;
    }

    if (!speakerObject) {
        ACSDK_ERROR(LX("createFailed").d("reason", "invalidSpeaker"));
        return nullptr;
    }

    SLVolumeItf volumeInterface;
    if (!speakerObject->getInterface(SL_IID_VOLUME, &volumeInterface)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "volumeInterfaceUnavailable"));
        return nullptr;
    }

    SLmillibel maxVolume;
    auto result = (*volumeInterface)->GetMaxVolumeLevel(volumeInterface, &maxVolume);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("createFailed").d("reason", "maxVolumeUnavailable").d("result", result));
        return nullptr;
    }

    if (maxVolume != DEVICE_MAX_VOLUME) {
        if (maxVolume > DEVICE_MAX_VOLUME) {
            ACSDK_WARN(LX("create").m("Amplification is not supported. Maximum volume will be 0mB."));
        } else {
            ACSDK_ERROR(LX("createFailed").m("Max volume should be at least 0mB according to OpenSL ES."));
            return nullptr;
        }
    }

    return std::unique_ptr<AndroidSLESSpeaker>(
        new AndroidSLESSpeaker(engine, outputMixObject, speakerObject, type, volumeInterface));
}

bool AndroidSLESSpeaker::setVolume(int8_t volume) {
    auto deviceVolume = toDeviceVolume(volume);
    auto result = (*m_volumeInterface)->SetVolumeLevel(m_volumeInterface, deviceVolume);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("setVolumeFailed").d("result", result).d("volume", deviceVolume));
        return false;
    }

    ACSDK_DEBUG5(LX("setVolume").d("avsVolume", volume).d("deviceVolume", deviceVolume));
    return true;
}

bool AndroidSLESSpeaker::adjustVolume(int8_t delta) {
    bool ok;
    int8_t currentVolume;
    std::tie(ok, currentVolume) = getAvsVolume();
    if (!ok) {
        return false;
    }

    // Use int16_t to avoid over/under flow.
    int16_t newVolume = static_cast<int16_t>(currentVolume) + delta;
    newVolume = std::min<int16_t>(std::max<int16_t>(newVolume, AVS_SET_VOLUME_MIN), AVS_SET_VOLUME_MAX);
    auto deviceVolume = toDeviceVolume(newVolume);

    auto result = (*m_volumeInterface)->SetVolumeLevel(m_volumeInterface, deviceVolume);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("adjustVolumeFailed").d("result", result).d("volume", deviceVolume));
        return false;
    }

    ACSDK_DEBUG5(LX("adjustVolume").d("avsVolume", newVolume).d("deviceVolume", deviceVolume));
    return true;
}

bool AndroidSLESSpeaker::setMute(bool mute) {
    auto result = (*m_volumeInterface)->SetMute(m_volumeInterface, mute);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("setMuteFailed").d("result", result).d("mute", mute));
        return false;
    }
    return true;
}

bool AndroidSLESSpeaker::getSpeakerSettings(avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings) {
    if (!settings) {
        ACSDK_ERROR(LX("getSpeakerSettingsFailed").d("reason", "nullSettings"));
        return false;
    }

    bool ok;
    int8_t volume;
    std::tie(ok, volume) = getAvsVolume();
    if (!ok) {
        return false;
    }

    SLboolean mute;
    auto result = (*m_volumeInterface)->GetMute(m_volumeInterface, &mute);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("getSpeakerSettingsFailed").d("result", result).d("mute", mute));
        return false;
    }

    ACSDK_DEBUG9(LX("getSettings").d("volume", (int)volume).d("mute", mute).d("type", m_type));
    settings->volume = volume;
    settings->mute = mute;
    return true;
}

avsCommon::sdkInterfaces::SpeakerInterface::Type AndroidSLESSpeaker::getSpeakerType() {
    return m_type;
}

AndroidSLESSpeaker::AndroidSLESSpeaker(
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESEngine> engine,
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> outputMixObject,
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> speakerObject,
    SpeakerInterface::Type type,
    SLVolumeItf volumeInterface) :
        m_engine{engine},
        m_outputMixObject{outputMixObject},
        m_speakerObject{speakerObject},
        m_volumeInterface{volumeInterface},
        m_type{type} {
}

std::pair<bool, int8_t> AndroidSLESSpeaker::getAvsVolume() const {
    SLmillibel deviceVolume;
    auto result = (*m_volumeInterface)->GetVolumeLevel(m_volumeInterface, &deviceVolume);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("getVolumeFailed").d("reason", "volumeUnavailable").d("result", result));
        return {false, 0};
    }
    deviceVolume = std::min(deviceVolume, DEVICE_MAX_VOLUME);
    return {true, toAvsVolume(deviceVolume)};
}

// ---------------------------------------------------------------------
// We base our calculations on the following conversion formula (in dB):
//
// DeviceVolume = 20 * (log10 ((AvsVolume - AvsMin) / AvsRange))
// -> AvsVolume = AvsRange * (10 ^ (DeviceVolume/20)) + AvsMin
//
// see https://en.wikipedia.org/wiki/Decibel for more details.
// ---------------------------------------------------------------------

int8_t AndroidSLESSpeaker::toAvsVolume(SLmillibel deviceVolume) const {
    float asDBel = deviceVolume / 100.0;                           // millibel is 100x DB so divide to get back to dB.
    float scaleFactor = std::pow(10, asDBel / CONVERSION_FACTOR);  // compute power level.
    return (scaleFactor * AVS_VOLUME_RANGE) + AVS_SET_VOLUME_MIN;  // Normalize to AVS range.
}

SLmillibel AndroidSLESSpeaker::toDeviceVolume(int8_t avsVolume) const {
    float volume = avsVolume - AVS_SET_VOLUME_MIN;
    if (0 == volume) {
        // We can't do log 0, so return minimum volume.
        return DEVICE_MIN_VOLUME;
    }

    float scaleFactor = volume / AVS_VOLUME_RANGE;
    float asDBel = CONVERSION_FACTOR * std::log10(scaleFactor);  // compute value as DB.
    return asDBel * 100;                                         // convert to millibel.
}

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
