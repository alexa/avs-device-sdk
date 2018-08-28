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

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/common.h>
#include <libavutil/error.h>
}

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "AndroidSLESMediaPlayer/FFmpegDeleter.h"
#include "AndroidSLESMediaPlayer/FFmpegUrlInputController.h"

/// String to identify log entries originating from this file.
static const std::string TAG("FFmpegUrlInputController");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::playlistParser;

/// Define invalid duration.
static const auto INVALID_DURATION = PlaylistParserObserverInterface::INVALID_DURATION;

/// This string represents the FFmpeg HTTP User-Agent option key.
static const char* USER_AGENT_OPTION{"user_agent"};

std::unique_ptr<FFmpegUrlInputController> FFmpegUrlInputController::create(
    std::shared_ptr<IterativePlaylistParserInterface> playlistParser,
    const std::string& url,
    const std::chrono::milliseconds& offset) {
    ACSDK_DEBUG5(LX("createUrlInput").sensitive("url", url).d("offset(ms)", offset.count()));

    if (!playlistParser) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullParser"));
        return nullptr;
    }

    if (url.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyUrl"));
        return nullptr;
    }

    if (!playlistParser->initializeParsing(url)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "parsePlaylistFailed"));
        return nullptr;
    }

    auto controller = std::unique_ptr<FFmpegUrlInputController>(new FFmpegUrlInputController(playlistParser, offset));
    if (!controller->findFirstEntry()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Empty playlist"));
        return nullptr;
    }

    return controller;
}

bool FFmpegUrlInputController::hasNext() const {
    return !m_done;
}

bool FFmpegUrlInputController::next() {
    if (m_done) {
        ACSDK_ERROR(LX("nextFailed").d("reason", "finished"));
        return false;
    }

    m_offset = std::chrono::milliseconds::zero();
    auto entry = m_parser->next();
    if (entry.url.empty()) {
        ACSDK_ERROR(LX("nextFailed").d("reason", "urlUnavailable").d("result", entry.parseResult));
        return false;
    }

    ACSDK_DEBUG5(LX("foundNext").sensitive("url", entry.url));
    m_done = entry.parseResult == PlaylistParseResult::FINISHED;
    m_currentUrl = entry.url;
    return true;
}

bool FFmpegUrlInputController::findFirstEntry() {
    auto offset = m_offset;
    while (!m_done) {
        ACSDK_DEBUG5(LX("findFirstEntry").d("offset", offset.count()));
        auto entry = m_parser->next();
        if (entry.url.empty()) {
            ACSDK_ERROR(LX("findFirstEntryFailed").d("reason", "urlUnavailable").d("result", entry.parseResult));
            return false;
        }

        m_done = entry.parseResult == PlaylistParseResult::FINISHED;

        if (entry.duration == INVALID_DURATION) {
            if (offset.count()) {
                ACSDK_WARN(LX("invalidDuration").m("Cannot find media duration. Start playing from the beginning"));
            }
            ACSDK_DEBUG5(LX("foundFirst")
                             .d("offset(ms)", m_offset.count())
                             .d("duration(ms)", "INVALID")
                             .sensitive("url", entry.url));
            m_offset = std::chrono::milliseconds::zero();
            m_currentUrl = entry.url;
            return true;
        }

        if ((offset - entry.duration) < std::chrono::milliseconds::zero()) {
            ACSDK_DEBUG5(LX("foundFirst")
                             .d("offset(ms)", m_offset.count())
                             .d("duration(ms)", entry.duration.count())
                             .sensitive("url", entry.url));
            m_currentUrl = entry.url;
            return true;
        }

        offset -= entry.duration;
    }

    ACSDK_ERROR(LX("findFirstEntryFailed").d("reason", "finished"));
    return false;
}

FFmpegUrlInputController::~FFmpegUrlInputController() {
}

FFmpegUrlInputController::FFmpegUrlInputController(
    std::shared_ptr<IterativePlaylistParserInterface> parser,
    const std::chrono::milliseconds& offset) :
        m_parser{parser},
        m_offset{offset},
        m_done{false} {
}

std::tuple<FFmpegInputControllerInterface::Result, std::shared_ptr<AVFormatContext>, std::chrono::milliseconds>
android::FFmpegUrlInputController::getCurrentFormatContext() {
    if (m_currentUrl.empty()) {
        ACSDK_ERROR(LX("getContextFailed").d("reason", "emptyUrl"));
        return {Result::ERROR, nullptr, std::chrono::milliseconds::zero()};
    }

    auto avFormatContext = avformat_alloc_context();
    if (!avFormatContext) {
        ACSDK_ERROR(LX("getContextFailed").d("reason", "avFormatAllocFailed"));
        return {Result::ERROR, nullptr, std::chrono::milliseconds::zero()};
    }

    AVDictionary* options = nullptr;
    av_dict_set(&options, USER_AGENT_OPTION, HTTPContentFetcherInterface::getUserAgent().c_str(), 0);
    auto error = avformat_open_input(&avFormatContext, m_currentUrl.c_str(), nullptr, &options);
    auto optionsPtr = std::unique_ptr<AVDictionary, AVDictionaryDeleter>(options);

    if (!optionsPtr) {
        // FFmpeg will put the options that could not be set back into the options dictionary.
        auto option = av_dict_get(optionsPtr.get(), USER_AGENT_OPTION, nullptr, 0);
        if (option) {
            // Do not modify or free the option pointer.
            ACSDK_WARN(LX(__func__).d("issue", "unableSetUserAgent").d(option->key, option->value));
        }
    }
    if (error != 0) {
        // The avFormatContext will be freed on failure.
        if (-EAGAIN == error) {
            ACSDK_DEBUG(LX("getContextFailed").d("reason", "Data unavailable. Try again."));
            return {Result::TRY_AGAIN, nullptr, std::chrono::milliseconds::zero()};
        }
        auto errorStr = av_err2str(error);
        ACSDK_ERROR(
            LX("getContextFailed").d("reason", "openInputFailed").sensitive("url", m_currentUrl).d("error", errorStr));
        return {Result::ERROR, nullptr, std::chrono::milliseconds::zero()};
    }

    return {Result::OK, std::shared_ptr<AVFormatContext>(avFormatContext, AVFormatContextDeleter()), m_offset};
}

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
