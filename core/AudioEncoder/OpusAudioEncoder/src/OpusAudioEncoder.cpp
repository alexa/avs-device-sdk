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

#include <iostream>
#include <climits>
#include <cstring>

#include <acsdk/OpusAudioEncoder/private/OpusAudioEncoder.h>
#include <AVSCommon/Utils/Endian.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace opusAudioEncoder {

using namespace avsCommon;
using namespace avsCommon::utils;
using namespace audioEncoderInterfaces;

/// String to identify log entries originating from this file.
#define TAG "OpusEncoderContext"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// AVS OPUS format name
static constexpr char AVS_FORMAT[] = "OPUS";

/// Audio sample rate: 16kHz
static constexpr unsigned int SAMPLE_RATE = 16000;

/// OPUS bitrate: 32kbps CBR
static constexpr unsigned int BIT_RATE = 32000;

/// OPUS frame length: 20ms
static constexpr unsigned int FRAME_LENGTH = 20;

/// PCM frame size
static constexpr unsigned int FRAME_SIZE = (SAMPLE_RATE / 1000) * FRAME_LENGTH;

/// OPUS packet size (CBR)
static constexpr unsigned int PACKET_SIZE = ((BIT_RATE / CHAR_BIT) / 1000) * FRAME_LENGTH;

/// Maximum packet size
static constexpr unsigned int MAX_PACKET_SIZE = PACKET_SIZE * 2;

inline uint16_t Reverse16(uint16_t value) {
    return (((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8));
}

std::unique_ptr<BlockAudioEncoderInterface> OpusAudioEncoder::createEncoder() {
    return std::unique_ptr<OpusAudioEncoder>(new OpusAudioEncoder);
}

OpusAudioEncoder::~OpusAudioEncoder() {
    close();
}

bool OpusAudioEncoder::init(AudioFormat inputFormat) {
    m_inputFormat = inputFormat;

    if (inputFormat.sampleRateHz != SAMPLE_RATE) {
        ACSDK_ERROR(LX("initFailed").d("reason", "Input sampling rate is invalid"));
        return false;
    }
    if (inputFormat.encoding != AudioFormat::Encoding::LPCM) {
        ACSDK_ERROR(LX("initFailed").d("reason", "Input audio format must be LPCM"));
        return false;
    }
    if (inputFormat.numChannels == 2 && inputFormat.layout != AudioFormat::Layout::INTERLEAVED) {
        // Error, only interleaved frames are supported for 2 channel
        ACSDK_ERROR(LX("initFailed").d("reason", "Input audio format must be interleaved"));
        return false;
    }

    m_outputFormat.numChannels = inputFormat.numChannels;
    return true;
}

size_t OpusAudioEncoder::getInputFrameSize() {
    return FRAME_SIZE;
}

size_t OpusAudioEncoder::getOutputFrameSize() {
    return PACKET_SIZE;
}

bool OpusAudioEncoder::requiresFullyRead() {
    return true;
}

AudioFormat OpusAudioEncoder::getAudioFormat() {
    return m_outputFormat;
}

std::string OpusAudioEncoder::getAVSFormatName() {
    return AVS_FORMAT;
}

bool OpusAudioEncoder::configureEncoder() {
    int err;

    // Use 32k bit rate
    err = opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(BIT_RATE));
    if (err != OPUS_OK) {
        ACSDK_ERROR(LX("startFailed").d("reason", "Failed to set bitrate to 32kbps").d("err", err));
        return false;
    }

    // CBR Only
    err = opus_encoder_ctl(m_encoder, OPUS_SET_VBR(0));
    if (err != OPUS_OK) {
        ACSDK_ERROR(LX("startFailed").d("reason", "Failed to set hard-CBR").d("err", err));
        return false;
    }

    // 20ms framesize
    err = opus_encoder_ctl(m_encoder, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_20_MS));
    if (err != OPUS_OK) {
        ACSDK_ERROR(LX("startFailed").d("reason", "Failed to set frame size to 20ms").d("err", err));
        return false;
    }

    return true;
}

bool OpusAudioEncoder::start(Bytes&) {
    int err;

    if (m_encoder) {
        ACSDK_ERROR(LX("startFailed").d("reason", "OpusEncoder is not null"));
        return false;
    }

    m_encoder = opus_encoder_create(m_inputFormat.sampleRateHz, m_inputFormat.numChannels, OPUS_APPLICATION_VOIP, &err);

    if (err != OPUS_OK) {
        ACSDK_ERROR(LX("startFailed").d("reason", "Failed to create OpusEncoder").d("err", err));
        return false;
    }

    if (!m_encoder) {
        ACSDK_ERROR(LX("startFailed").d("reason", "OpusEncoder is null"));
        return false;
    }

    if (!configureEncoder()) {
        // Destroy previously created encoder
        opus_encoder_destroy(m_encoder);
        return false;
    }

    return true;
}

bool OpusAudioEncoder::processSamples(Bytes::const_iterator begin, Bytes::const_iterator end, Bytes& buffer) {
    opus_int16 pcm[FRAME_SIZE];

    if (end <= begin) {
        ACSDK_ERROR(LX("processSamplesError").d("reason", "InputRangeNegative"));
        return false;
    }
    const auto byteCount = static_cast<std::size_t>(std::distance(begin, end));
    if (byteCount > sizeof(pcm)) {
        ACSDK_ERROR(LX("processSamplesError").d("reason", "InputTooLarge"));
        return false;
    }
    if (byteCount % sizeof(opus_int16)) {
        ACSDK_ERROR(LX("processSamplesError").d("reason", "InputHasIncompleteSample"));
        return false;
    }

    std::memcpy(pcm, &*begin, byteCount);
    const bool littleEndianInput = m_inputFormat.endianness == AudioFormat::Endianness::LITTLE;
    const size_t numberOfWords = byteCount / sizeof(opus_int16);

    if (littleEndianInput != littleEndianMachine()) {
        for (size_t i = 0; i < numberOfWords; i++) {
            pcm[i] = Reverse16(pcm[i]);
        }
    }

    const auto bufferOffset = buffer.size();
#if __cpp_exceptions || defined(__EXCEPTIONS)
    try {
#endif
        buffer.resize(bufferOffset + MAX_PACKET_SIZE);
#if __cpp_exceptions || defined(__EXCEPTIONS)
    } catch (const std::bad_alloc& e) {
        ACSDK_ERROR(LX("processSamplesError").d("bufferResizeFailed", e.what()));
        return false;
    }
#endif

    opus_int32 res =
        opus_encode(m_encoder, pcm, static_cast<int>(numberOfWords), buffer.data() + bufferOffset, MAX_PACKET_SIZE);
    if (res >= 0) {
        buffer.resize(bufferOffset + static_cast<size_t>(res));
        return true;
    } else {
        ACSDK_ERROR(LX("processSamplesError").d("code", res));
        buffer.resize(bufferOffset);
        return false;
    }
}

bool OpusAudioEncoder::flush(Bytes&) {
    return true;
}

void OpusAudioEncoder::close() {
    if (m_encoder) {
        opus_encoder_destroy(m_encoder);
        m_encoder = nullptr;
    }
}

OpusAudioEncoder::OpusAudioEncoder() :
        m_encoder{nullptr},
        m_outputFormat{
            AudioFormat::Encoding::OPUS,
            AudioFormat::Endianness::LITTLE,
            SAMPLE_RATE,
            16,
            0,
            false,
            AudioFormat::Layout::INTERLEAVED,
        } {
}

}  // namespace opusAudioEncoder
}  // namespace alexaClientSDK
