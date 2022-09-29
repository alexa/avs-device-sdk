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

#include <limits>
#include <fstream>

#include "AVSCommon/Utils/Endian.h"
#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/WavUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

#define TAG "WavUtils"

#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

static const int SHORT_CHUNK_SIZE = 2;

static const int LONG_CHUNK_SIZE = 4;

/**
 * Helper Method to write uint16_t into WAVE byte vector.
 *
 * @param input The input being written into the header.
 * @param [out] header The WAVE header to write to.
 */
static void writeToWavHeader(uint16_t input, ByteVector* header) {
    if (!header) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullHeader"));
        return;
    }

    // WAVE Header should be written with little endian value
    if (!littleEndianMachine()) {
        input = swapEndian(input);
    }

    header->insert(
        header->end(),
        reinterpret_cast<const unsigned char*>(&input),
        reinterpret_cast<const unsigned char*>(&input) + sizeof(input));
};

/**
 * Helper Method to write uint32_t into WAVE byte vector.
 *
 * @param input The input being written into the header.
 * @param [out] header The WAVE header to write to.
 */
static void writeToWavHeader(uint32_t input, ByteVector* header) {
    if (!header) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullHeader"));
        return;
    }

    // WAVE Header should be written with little endian value
    if (!littleEndianMachine()) {
        input = swapEndian(input);
    }

    header->insert(
        header->end(),
        reinterpret_cast<const unsigned char*>(&input),
        reinterpret_cast<const unsigned char*>(&input) + sizeof(input));
};

/**
 * Helper Method to write 4 char into WAVE byte vector.
 *
 * @param input The input being written into the header.
 * @param [out] header The WAVE header to write to.
 */
static void writeToWavHeader(const char* input, ByteVector* header) {
    if (!header) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullHeader"));
        return;
    }

    if (!input) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullInput"));
        return;
    }

    auto inputSize = strlen(input);
    if (inputSize != LONG_CHUNK_SIZE) {
        ACSDK_ERROR(LX(__func__).d("reason", "inputInvalidSize").d("inputSize", inputSize));
        return;
    }

    header->insert(
        header->end(),
        reinterpret_cast<const unsigned char*>(input),
        reinterpret_cast<const unsigned char*>(input) + LONG_CHUNK_SIZE);
};

/**
 * Helper Method to read 2 bytes from WAVE header.
 *
 * @param buffer The buffer containing the WAVE header.
 * @param offset The offset to read from the buffer.
 * @return The 2 bytes as a uint_16t.
 */
static uint16_t readShortFromHeader(char* buffer, unsigned int offset) {
    if (!buffer) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullBuffer"));
    }
    uint16_t output;
    memcpy(&output, buffer + offset, SHORT_CHUNK_SIZE);
    if (!littleEndianMachine()) {
        output = swapEndian(output);
    }
    return output;
}

/**
 * Helper Method to read 4 bytes from WAVE header.
 *
 * @param buffer The buffer containing the WAVE header.
 * @param offset The offset to read from the buffer.
 * @return The 4 bytes as a uint_32t.
 */
static uint32_t readLongFromHeader(char* buffer, unsigned int offset) {
    if (!buffer) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullBuffer"));
    }
    uint32_t output;
    memcpy(&output, buffer + offset, LONG_CHUNK_SIZE);
    if (!littleEndianMachine()) {
        output = swapEndian(output);
    }
    return output;
}

/**
 * Helper Method to read 4 char from WAVE header.
 *
 * @param buffer The buffer containing the WAVE header.
 * @param offset The offset to read from the buffer.
 * @return The 4 char as a uint_32t.
 */
static uint32_t readFourCharFromHeader(char* buffer, unsigned int offset) {
    if (!buffer) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullBuffer"));
    }
    uint32_t output;
    memcpy(&output, buffer + offset, LONG_CHUNK_SIZE);
    return output;
}

ByteVector generateWavHeader(
    unsigned int bytesPerSample,
    unsigned int channels,
    unsigned int rate,
    std::chrono::milliseconds totalDuration,
    bool isPCM) {
    uint32_t numberSample = static_cast<uint32_t>(totalDuration.count()) * (rate / 100);
    if (numberSample > 0) {
        numberSample /= 10;
    }
    int headerSize = isPCM ? PCM_HEADER_SIZE : NON_PCM_HEADER_SIZE;

    ByteVector output;

    // riffId
    writeToWavHeader(ID_RIFF, &output);
    // riffSz
    writeToWavHeader(static_cast<uint32_t>(numberSample * channels * bytesPerSample + headerSize - 8), &output);
    // riffFmt
    writeToWavHeader(ID_WAVE, &output);
    // fmtId
    writeToWavHeader(ID_FMT, &output);
    // fmtSz
    writeToWavHeader(FLOAT_SUBCHUNK_SZ, &output);
    // audioFormat
    writeToWavHeader(FORMAT_IEEE_FLOAT, &output);
    // numChannels
    writeToWavHeader(static_cast<uint16_t>(channels), &output);
    // sampleRate
    writeToWavHeader(static_cast<uint32_t>(rate), &output);
    // byteRate
    writeToWavHeader(static_cast<uint32_t>((bytesPerSample * channels * rate)), &output);
    // blockAlign
    writeToWavHeader(static_cast<uint16_t>(bytesPerSample * channels), &output);
    // bitsPerSample
    writeToWavHeader(static_cast<uint16_t>(bytesPerSample * BITS_PER_BYTE), &output);
    if (!isPCM) {
        // cbSz
        writeToWavHeader(static_cast<uint16_t>(0), &output);
        // factId
        writeToWavHeader(ID_FACT, &output);
        // factSz
        writeToWavHeader(FACT_CHUNK_SZ, &output);
        // factSampleLen
        writeToWavHeader(static_cast<uint32_t>(channels * numberSample), &output);
    }
    // dataId
    writeToWavHeader(ID_DATA, &output);
    // dataSz
    writeToWavHeader((uint32_t)(numberSample * channels * bytesPerSample), &output);

    return output;
}

bool readWAVFile(
    const std::string& absoluteFilePath,
    std::vector<uint16_t>* audioBuffer,
    WavHeader& wavHeader,
    bool isPCM) {
    if (!audioBuffer) {
        ACSDK_ERROR(LX("readAudioFileFailed").d("reason", "audioBuffer is nullptr"));
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
    size_t fileLengthInBytes = static_cast<size_t>(inputFile.tellg());

    const size_t headerSize = isPCM ? PCM_HEADER_SIZE : NON_PCM_HEADER_SIZE;
    if (fileLengthInBytes <= headerSize) {
        ACSDK_ERROR(LX("readAudioFileFailed").d("reason", "file size less than RIFF header"));
        return false;
    }

    inputFile.seekg(0, std::ios::beg);

    // Read each part of the WAVE header into audioBuffer
    std::vector<char> buffer(headerSize);
    inputFile.read(buffer.data(), headerSize);

    char* pBuffer = buffer.data();

    if (static_cast<size_t>(inputFile.gcount()) != headerSize) {
        ACSDK_ERROR(LX("readAudioFileFailed").d("reason", "failed reading header"));
        return false;
    }

    wavHeader.riffId = readFourCharFromHeader(pBuffer, RIFF_ID_OFFSET);
    wavHeader.riffSz = readLongFromHeader(pBuffer, RIFF_SZ_OFFSET);
    wavHeader.riffFmt = readFourCharFromHeader(pBuffer, RIFF_FMT_OFFSET);
    wavHeader.fmtId = readFourCharFromHeader(pBuffer, FMT_ID_OFFSET);
    wavHeader.fmtSz = readLongFromHeader(pBuffer, FMT_SZ_OFFSET);
    wavHeader.audioFormat = readShortFromHeader(pBuffer, AUDIO_FORMAT_OFFSET);
    wavHeader.numChannels = readShortFromHeader(pBuffer, NUM_CHANNELS_OFFSET);
    wavHeader.sampleRate = readLongFromHeader(pBuffer, SAMPLE_RATE_OFFSET);
    wavHeader.byteRate = readLongFromHeader(pBuffer, BYTE_RATE_OFFSET);
    wavHeader.blockAlign = readShortFromHeader(pBuffer, BLOCK_ALIGN_OFFSET);
    wavHeader.bitsPerSample = readShortFromHeader(pBuffer, BITS_PER_SAMPLE_OFFSET);
    if (!isPCM) {
        wavHeader.cbSz = readShortFromHeader(pBuffer, CB_SZ_OFFSET);
        wavHeader.factId = readFourCharFromHeader(pBuffer, FACT_ID_OFFSET);
        wavHeader.factSz = readLongFromHeader(pBuffer, FACT_SZ_OFFSET);
        wavHeader.factSampleLen = readLongFromHeader(pBuffer, FACT_SAMPLE_LEN_OFFSET);
    }
    wavHeader.dataId = readLongFromHeader(pBuffer, DATA_ID_OFFSET + (isPCM ? 0 : NON_PCM_OFFSET));
    wavHeader.dataSz = readLongFromHeader(pBuffer, DATA_SZ_OFFSET + (isPCM ? 0 : NON_PCM_OFFSET));

    // Read the remainder of the wav file (excluding header) into the audioBuffer.
    size_t numSamples = (fileLengthInBytes - headerSize) / sizeof(uint16_t);

    audioBuffer->resize(numSamples, 0);
    inputFile.read(reinterpret_cast<char*>(&(audioBuffer->at(0))), numSamples * sizeof(uint16_t));

    // Check that all audio data was read successfully.
    if (static_cast<size_t>(inputFile.gcount()) != (numSamples * sizeof(uint16_t))) {
        ACSDK_ERROR(LX("readAudioFileFailed").d("reason", "failed reading audio data"));
        return false;
    }

    return true;
};

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
