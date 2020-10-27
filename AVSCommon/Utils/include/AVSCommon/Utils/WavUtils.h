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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_WAVUTILS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_WAVUTILS_H_

#include <chrono>
#include <string>
#include <vector>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

// WAV definitions
constexpr const char* ID_RIFF = "RIFF";
constexpr const char* ID_WAVE = "WAVE";
constexpr const char* ID_FMT = "fmt ";
constexpr const char* ID_DATA = "data";
constexpr uint32_t PCM_SUBCHUNK_SZ = 16;

// PCM definitions
constexpr uint16_t FORMAT_PCM = 1;
constexpr uint16_t BITS_PER_BYTE = 8;
constexpr uint16_t PLAY_BITS_PER_SAMPLE = 16;

// FLOAT definitions
constexpr uint16_t FORMAT_IEEE_FLOAT = 3;
constexpr uint32_t FLOAT_SUBCHUNK_SZ = 18;
constexpr uint32_t FACT_CHUNK_SZ = 4;
constexpr const char* ID_FACT = "fact";

// WAV Header Sizes
constexpr unsigned int PCM_HEADER_SIZE = 44;
constexpr unsigned int NON_PCM_HEADER_SIZE = 58;

// WAV Header Offsets
constexpr unsigned int RIFF_ID_OFFSET = 0;
constexpr unsigned int RIFF_SZ_OFFSET = 4;
constexpr unsigned int RIFF_FMT_OFFSET = 8;
constexpr unsigned int FMT_ID_OFFSET = 12;
constexpr unsigned int FMT_SZ_OFFSET = 16;
constexpr unsigned int AUDIO_FORMAT_OFFSET = 20;
constexpr unsigned int NUM_CHANNELS_OFFSET = 22;
constexpr unsigned int SAMPLE_RATE_OFFSET = 24;
constexpr unsigned int BYTE_RATE_OFFSET = 28;
constexpr unsigned int BLOCK_ALIGN_OFFSET = 32;
constexpr unsigned int BITS_PER_SAMPLE_OFFSET = 34;
constexpr unsigned int DATA_ID_OFFSET = 36;
constexpr unsigned int DATA_SZ_OFFSET = 40;

// Non-PCM Fields
constexpr unsigned int CB_SZ_OFFSET = 36;
constexpr unsigned int FACT_ID_OFFSET = 38;
constexpr unsigned int FACT_SZ_OFFSET = 42;
constexpr unsigned int FACT_SAMPLE_LEN_OFFSET = 46;

// OFFSET for Non-PCM fields
constexpr unsigned int NON_PCM_OFFSET = 14;

using ByteVector = std::vector<unsigned char>;

/**
 * Structure defining the WavHeader contents.
 */
struct WavHeader {
    uint32_t riffId;
    uint32_t riffSz;
    uint32_t riffFmt;
    uint32_t fmtId;
    uint32_t fmtSz;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint16_t cbSz;
    uint32_t factId;
    uint32_t factSz;
    uint32_t factSampleLen;
    uint32_t dataId;
    uint32_t dataSz;
};

/**
 * Generates a WAVE header.
 *
 * @param bytesPerSample The number of bytes per sample.
 * @param channels The number of channels in the audio data.
 * @param rate The sample rate of the audio data, in blocks per second.
 * @param totalDuration The total duration of the audio data.
 * @param isPCM Bool indicating if header to generate is PCM format or Non-PCM.
 *
 * @return The ByteVector of the generated WAVE header.
 */
ByteVector generateWavHeader(
    unsigned int bytesPerSample,
    unsigned int channels,
    unsigned int rate,
    std::chrono::milliseconds totalDuration,
    bool isPCM = true);

/**
 * Reads a .wav file and seperates the audio data from the header data.
 *
 * @param absoluteFilePath The path to a file to retrieve the WAVE header from.
 * @param audioBuffer The pointer to the audioBuffer vector.
 * @param wavHeader The WavHeader.
 * @param isPCM Bool indicating if header to generate is PCM format or Non-PCM.
 *
 * @return true if successful, else false.
 */
bool readWAVFile(
    const std::string& absoluteFilePath,
    std::vector<uint16_t>* audioBuffer,
    WavHeader& wavHeader,
    bool isPCM = true);

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_WAVUTILS_H_
