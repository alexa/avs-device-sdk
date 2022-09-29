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

#include "Diagnostics/DiagnosticsUtils.h"

#include <fstream>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace diagnostics {
namespace utils {

/// String to identify log entries originating from this file.
#define TAG "DiagnosticsUtils"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Constant indicating the number of bits per sample.
static constexpr unsigned int NUM_BITS_PER_SAMPLE = 16;

/// Constant indicating the number of audio channels supported.
static constexpr unsigned int NUM_OF_AUDIO_CHANNELS = 1;

/// Constant indicating the Samples per second supported.
static constexpr unsigned int SAMPLES_PER_SECOND = 16000;

/// Constant indicating the LPCM audio format in the WAV file header.
static constexpr unsigned int LPCM_AUDIO_FORMAT = 1;

bool validateAudioFormat(const avsCommon::utils::WavHeader& wavHeader) {
    if (wavHeader.bitsPerSample != NUM_BITS_PER_SAMPLE) {
        ACSDK_ERROR(LX("validateAudioFormatFailed").d("reason", "only 16 bits per sample supported"));
        return false;
    }

    if (wavHeader.numChannels != NUM_OF_AUDIO_CHANNELS) {
        ACSDK_ERROR(LX("validateAudioFormatFailed").d("reason", "only 1 audio channel supported"));
        return false;
    }

    if (wavHeader.sampleRate != SAMPLES_PER_SECOND) {
        ACSDK_ERROR(LX("validateAudioFormatFailed").d("reason", "only 16000 samples per second supported"));
        return false;
    }

    if (wavHeader.audioFormat != LPCM_AUDIO_FORMAT) {
        ACSDK_ERROR(LX("validateAudioFormatFailed").d("reason", "only LPCM supported"));
        return false;
    }

    return true;
}

bool readWavFileToBuffer(const std::string& absoluteFilePath, std::vector<uint16_t>* audioBuffer) {
    avsCommon::utils::WavHeader wavHeader;

    if (!avsCommon::utils::readWAVFile(absoluteFilePath, audioBuffer, wavHeader)) {
        ACSDK_ERROR(LX(__func__).m("readWavFileFailed"));
        return false;
    }

    if (!validateAudioFormat(wavHeader)) {
        ACSDK_ERROR(LX(__func__).m("validateWavHeaderFailed"));
        return false;
    }

    return true;
}

}  // namespace utils
}  // namespace diagnostics
}  // namespace alexaClientSDK
