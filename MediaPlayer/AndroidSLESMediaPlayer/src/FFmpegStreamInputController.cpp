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

#include <istream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/common.h>
#include <libavutil/error.h>
}

#include <AVSCommon/Utils/Logger/Logger.h>

#include "AndroidSLESMediaPlayer/FFmpegDeleter.h"
#include "AndroidSLESMediaPlayer/FFmpegStreamInputController.h"

/// String to identify log entries originating from this file.
static const std::string TAG("FFmpegStreamInputController");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

/// Buffers will be the size of a regular page.
static constexpr int BUFFER_SIZE{4096};

std::unique_ptr<FFmpegStreamInputController> FFmpegStreamInputController::create(
    std::shared_ptr<std::istream> stream,
    bool repeat) {
    if (!stream) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullStream"));
        return nullptr;
    }

    return std::unique_ptr<FFmpegStreamInputController>(new FFmpegStreamInputController(stream, repeat));
}

int FFmpegStreamInputController::read(uint8_t* buffer, int bufferSize) {
    m_stream->read(reinterpret_cast<char*>(buffer), bufferSize);
    auto bytesRead = m_stream->gcount();
    if (!bytesRead) {
        if (m_stream->bad()) {
            ACSDK_ERROR(LX("readFailed").d("readState", m_stream->rdstate()));
            return AVERROR_EXTERNAL;
        }
        return AVERROR_EOF;
    }
    return bytesRead;
}

bool FFmpegStreamInputController::hasNext() const {
    return m_repeat;
}

bool FFmpegStreamInputController::next() {
    if (!m_repeat) {
        ACSDK_ERROR(LX("nextFailed").d("reason", "repeatIsOff"));
        return false;
    }

    m_stream->clear();
    m_stream->seekg(0);
    return m_stream->good();
}

FFmpegStreamInputController::FFmpegStreamInputController(std::shared_ptr<std::istream> stream, bool repeat) :
        m_stream{stream},
        m_repeat{repeat} {
}

std::tuple<FFmpegInputControllerInterface::Result, std::shared_ptr<AVFormatContext>, std::chrono::milliseconds>
FFmpegStreamInputController::getCurrentFormatContext() {
    if (m_ioContext) {
        // Invalidate possible references to this object.
        m_ioContext->opaque = nullptr;
    }

    unsigned char* buffer =
        static_cast<unsigned char*>(av_malloc(BUFFER_SIZE + AVPROBE_PADDING_SIZE));  // Owned by m_ioContext
    if (!buffer) {
        ACSDK_ERROR(LX("getContextFailed").d("reason", "avMallocFailed"));
        return {Result::ERROR, nullptr, std::chrono::milliseconds::zero()};
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

    auto error = avformat_open_input(&avFormatContext, "", nullptr, nullptr);
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

int FFmpegStreamInputController::feedBuffer(void* userData, uint8_t* buffer, int bufferSize) {
    auto inputController = reinterpret_cast<FFmpegStreamInputController*>(userData);
    if (!inputController) {
        ACSDK_ERROR(LX("feedAvioBufferFailed").d("reason", "nullInputController"));
        return AVERROR_EXTERNAL;
    }
    return inputController->read(buffer, bufferSize);
}

FFmpegStreamInputController::~FFmpegStreamInputController() {
    if (m_ioContext) {
        m_ioContext->opaque = nullptr;
    }
}

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
