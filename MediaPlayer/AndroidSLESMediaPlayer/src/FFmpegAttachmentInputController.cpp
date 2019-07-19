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
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/common.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
}

#include <AVSCommon/AVS/Attachment/AttachmentReader.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "AndroidSLESMediaPlayer/FFmpegAttachmentInputController.h"
#include "AndroidSLESMediaPlayer/FFmpegDeleter.h"

/// String to identify log entries originating from this file.
static const std::string TAG("FFmpegAttachmentInputController");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils;

/// Timeout for read operations.
static const std::chrono::milliseconds READ_TIMEOUT{10};

/// Buffers will be the size of a regular page.
static constexpr int BUFFER_SIZE{4096};

/// The size of a byte in bits.
static constexpr unsigned int BYTE_TO_BITS{8u};

/// Use no special flags during av_dict_copy.
static const int EMPTY_FLAGS = 0;

std::unique_ptr<FFmpegAttachmentInputController> FFmpegAttachmentInputController::create(
    std::shared_ptr<AttachmentReader> reader,
    const avsCommon::utils::AudioFormat* format) {
    if (!reader) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullReader"));
        return nullptr;
    }

    std::shared_ptr<AVInputFormat> inputFormat;
    std::shared_ptr<AVDictionary> inputOptions;
    if (format) {
        if (format->encoding == AudioFormat::Encoding::OPUS) {
            ACSDK_ERROR(LX("createFailed").d("reason", "opusNotSupported"));
            return nullptr;
        } else {
            std::string inputName = format->dataSigned ? "s" : "u";
            inputName += std::to_string(format->sampleSizeInBits);
            if (format->sampleSizeInBits > BYTE_TO_BITS) {
                inputName += format->endianness == AudioFormat::Endianness::LITTLE ? "le" : "be";
            }
            inputFormat =
                std::shared_ptr<AVInputFormat>(av_find_input_format(inputName.c_str()), AVInputFormatDeleter());

            ACSDK_INFO(LX("create").d("name", inputName));
            if (!inputFormat) {
                ACSDK_ERROR(LX("createFailed")
                                .d("reason", "formatNotSupported")
                                .d("name", inputName)
                                .d("encoding", format->encoding)
                                .d("rate", format->sampleRateHz)
                                .d("sampleSize", format->sampleSizeInBits)
                                .d("numChannels", format->numChannels)
                                .d("signed", format->dataSigned)
                                .d("endianness", format->endianness));
                return nullptr;
            }
        }

        AVDictionary* dictionary = nullptr;
        av_dict_set_int(&dictionary, "channels", format->numChannels, 0);
        av_dict_set_int(&dictionary, "framerate", format->sampleRateHz, 0);
        inputOptions = std::shared_ptr<AVDictionary>(dictionary, AVDictionaryDeleter());
    }

    return std::unique_ptr<FFmpegAttachmentInputController>(
        new FFmpegAttachmentInputController(reader, inputFormat, inputOptions));
}

int FFmpegAttachmentInputController::read(uint8_t* buffer, int bufferSize) {
    AttachmentReader::ReadStatus readStatus;
    auto readSize = m_reader->read(buffer, bufferSize, &readStatus, READ_TIMEOUT);
    switch (readStatus) {
        case AttachmentReader::ReadStatus::OK:
            return readSize;
        case AttachmentReader::ReadStatus::OK_WOULDBLOCK:
        case AttachmentReader::ReadStatus::OK_TIMEDOUT:
            ACSDK_DEBUG3(LX(__func__).d("status", readStatus).d("readSize", readSize));
            return readSize ? readSize : AVERROR(EAGAIN);
        case AttachmentReader::ReadStatus::OK_OVERRUN_RESET:
            ACSDK_DEBUG3(LX(__func__).d("status", readStatus));
            return AVERROR(EAGAIN);
        case AttachmentReader::ReadStatus::CLOSED:
            ACSDK_DEBUG5(LX(__func__).m("Found EOF"));
            return AVERROR_EOF;
        case AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
        case AttachmentReader::ReadStatus::ERROR_INTERNAL:
        case AttachmentReader::ReadStatus::ERROR_OVERRUN:
            ACSDK_ERROR(LX("readFailed").d("reason", readStatus));
            return AVERROR_EXTERNAL;
    }
    return readSize;
}

bool FFmpegAttachmentInputController::hasNext() const {
    return false;
}

bool FFmpegAttachmentInputController::next() {
    ACSDK_ERROR(LX("nextFailed").d("reason", "unsupportedOperation"));
    return false;
}

FFmpegAttachmentInputController::FFmpegAttachmentInputController(
    std::shared_ptr<AttachmentReader> reader,
    std::shared_ptr<AVInputFormat> inputFormat,
    std::shared_ptr<AVDictionary> inputOptions) :
        m_reader{reader},
        m_inputFormat{inputFormat},
        m_inputOptions{inputOptions} {
}

int FFmpegAttachmentInputController::feedBuffer(void* userData, uint8_t* buffer, int bufferSize) {
    auto inputController = reinterpret_cast<FFmpegAttachmentInputController*>(userData);
    if (!inputController) {
        ACSDK_ERROR(LX("feedAvioBufferFailed").d("reason", "nullInputController"));
        return AVERROR_EXTERNAL;
    }
    return inputController->read(buffer, bufferSize);
}

std::tuple<FFmpegInputControllerInterface::Result, std::shared_ptr<AVFormatContext>, std::chrono::milliseconds>
FFmpegAttachmentInputController::getCurrentFormatContext() {
    unsigned char* buffer =
        static_cast<unsigned char*>(av_malloc(BUFFER_SIZE + AVPROBE_PADDING_SIZE));  // Owned by m_ioContext
    if (!buffer) {
        ACSDK_ERROR(LX("getContextFailed").d("reason", "avMallocFailed"));
        return {Result::ERROR, nullptr, std::chrono::milliseconds::zero()};
    }

    if (m_ioContext) {
        // Invalidate possible references to this object.
        m_ioContext->opaque = nullptr;
    }

    m_ioContext = std::shared_ptr<AVIOContext>(
        avio_alloc_context(buffer, BUFFER_SIZE, false, this, feedBuffer, nullptr, nullptr), AVIOContextDeleter());
    if (!m_ioContext) {
        ACSDK_ERROR(LX("getContextFailed").d("reason", "avioAllocFailed"));
        return {Result::ERROR, nullptr, std::chrono::milliseconds::zero()};
    }

    // Allocate the AVFormatContext:
    auto avFormatContext = avformat_alloc_context();
    if (!avFormatContext) {
        ACSDK_ERROR(LX("getContextFailed").d("reason", "avFormatAllocFailed"));
        return {Result::ERROR, nullptr, std::chrono::milliseconds::zero()};
    }

    avFormatContext->pb = m_ioContext.get();
    avFormatContext->format_probesize = BUFFER_SIZE;

    AVDictionary* options = nullptr;
    av_dict_copy(&options, m_inputOptions.get(), EMPTY_FLAGS);  // Open_input will change the pointer value.
    auto error = avformat_open_input(&avFormatContext, "", m_inputFormat.get(), &options);
    av_dict_free(&options);
    if (error != 0) {
        // The avFormatContext will be freed on failure.
        if (-EAGAIN == error) {
            ACSDK_DEBUG(LX("getContextdFailed").d("reason", "Data unavailable. Try again."));
            return {Result::TRY_AGAIN, nullptr, std::chrono::milliseconds::zero()};
        }
        auto errorStr = av_err2str(error);
        ACSDK_ERROR(LX("getContextFailed").d("reason", "openInputFailed").d("error", errorStr));
        return {Result::ERROR, nullptr, std::chrono::milliseconds::zero()};
    }

    auto ioContext = m_ioContext;
    return {Result::OK,
            std::shared_ptr<AVFormatContext>(
                avFormatContext, [ioContext](AVFormatContext* context) { AVFormatContextDeleter()(context); }),
            std::chrono::milliseconds::zero()};
}

FFmpegAttachmentInputController::~FFmpegAttachmentInputController() {
    if (m_ioContext) {
        m_ioContext->opaque = nullptr;
    }
}

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
