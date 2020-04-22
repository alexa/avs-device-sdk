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
static const std::string TAG("DiagnosticsUtils");

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

bool validateAudioFormat(const WavFileHeader& wavFileHeader) {
    if (wavFileHeader.bitsPerSample != NUM_BITS_PER_SAMPLE) {
        ACSDK_ERROR(LX("validateAudioFormatFailed").d("reason", "only 16 bits per sample supported"));
        return false;
    }

    if (wavFileHeader.numberOfChannels != NUM_OF_AUDIO_CHANNELS) {
        ACSDK_ERROR(LX("validateAudioFormatFailed").d("reason", "only 1 audio channel supported"));
        return false;
    }

    if (wavFileHeader.samplesPerSec != SAMPLES_PER_SECOND) {
        ACSDK_ERROR(LX("validateAudioFormatFailed").d("reason", "only 16000 samples per second supported"));
        return false;
    }

    if (wavFileHeader.audioFormat != LPCM_AUDIO_FORMAT) {
        ACSDK_ERROR(LX("validateAudioFormatFailed").d("reason", "only LPCM supported"));
        return false;
    }

    return true;
}

bool readWavFileToBuffer(const std::string& absoluteFilePath, std::vector<uint16_t>* audioBuffer) {
    if (!audioBuffer) {
        ACSDK_ERROR(LX("readAudioFileFailed").d("reason", "nullptrAudioBuffer"));
        return false;
    }

    // Attempt to open the given file.
    std::ifstream inputFile(absoluteFilePath, std::ios::binary);
    if (!inputFile.good()) {
        ACSDK_ERROR(LX("readAudioFileFailed").d("reason", "unable to open file"));
        return false;
    }

    // Check if the file size is at least the size of a .wav file header.
    inputFile.seekg(0, std::ios::end);
    int fileLengthInBytes = inputFile.tellg();

    const int wavFileHeaderSize = sizeof(WavFileHeader);
    if (fileLengthInBytes <= wavFileHeaderSize) {
        ACSDK_ERROR(LX("readAudioFileFailed").d("reason", "file size less than RIFF header"));
        return false;
    }

    inputFile.seekg(0, std::ios::beg);

    // Attempt to read the wav file header.
    WavFileHeader wavFileHeader;
    inputFile.read(reinterpret_cast<char*>(&wavFileHeader), wavFileHeaderSize);

    if (static_cast<size_t>(inputFile.gcount()) != wavFileHeaderSize) {
        ACSDK_ERROR(LX("readAudioFileFailed").d("reason", "failed reading header"));
        return false;
    }

    // TODO (ACSDK-3226) Correctly verify that the data read from the file is a wav header regardless of device
    // endianness.

    // Read the remainder of the wav file (excluding header) into the audioBuffer.
    int numSamples = (fileLengthInBytes - wavFileHeaderSize) / sizeof(uint16_t);

    audioBuffer->resize(numSamples, 0);
    inputFile.read(reinterpret_cast<char*>(&(audioBuffer->at(0))), numSamples * sizeof(uint16_t));

    // Check that all audio data was read successfully.
    if (static_cast<size_t>(inputFile.gcount()) != (numSamples * sizeof(uint16_t))) {
        ACSDK_ERROR(LX("readAudioFileFailed").d("reason", "failed reading audio data"));
        return false;
    }

    return true;
}

}  // namespace utils
}  // namespace diagnostics
}  // namespace alexaClientSDK
