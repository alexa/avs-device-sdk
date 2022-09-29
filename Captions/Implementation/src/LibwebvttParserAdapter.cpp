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

#include <algorithm>
#include <map>
#include <iostream>
#include <thread>

#include <AVSCommon/Utils/Logger/LoggerUtils.h>
#include <webvtt/parser.h>
#include <webvtt/node.h>

#include "Captions/LibwebvttParserAdapter.h"
#include "Captions/CaptionData.h"

namespace alexaClientSDK {
namespace captions {

using namespace alexaClientSDK::avsCommon::utils::logger;

/// String to identify log entries originating from this file.
#define TAG "LibwebvttParserAdapter"

/// Return value indicating an error occurred during parsing.
static const int WEBVTT_CALLBACK_ERROR = -1;

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The object to receive parsed caption frames.
static std::shared_ptr<CaptionFrameParseListenerInterface> g_parseListener;

/// A vector which maintains a reference to every MediaPlayerSourceId passed through this adapter.
static std::vector<CaptionFrame::MediaPlayerSourceId> g_captionSourceIds;

/// A mapping of the last known end time for every caption ID, for calculating frame delays.
static std::map<CaptionFrame::MediaPlayerSourceId, std::chrono::milliseconds> g_captionIdsToLastEndTime;

/// The singleton instance returned by @c LibwebvttParserAdapter::getInstance();
static std::shared_ptr<LibwebvttParserAdapter> m_libwebvttParserAdapterInstance;

/// Global mutex for guarding access to the singleton instance and the maps.
static std::mutex g_mutex;

std::shared_ptr<LibwebvttParserAdapter> LibwebvttParserAdapter::getInstance() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!m_libwebvttParserAdapterInstance) {
        m_libwebvttParserAdapterInstance = std::shared_ptr<LibwebvttParserAdapter>(new LibwebvttParserAdapter());
    }
    return m_libwebvttParserAdapterInstance;
}

/**
 * Recursively walk the tree structure returned by libwebvtt, extracting the styles and text.
 *
 * @note The node may contain sensitive information, so certain data elements will only be printed if @c
 * ACSDK_EMIT_SENSITIVE_LOGS is ON.
 *
 * @param cleanText The @c std::stringstream where the final text will be placed as output of this function.
 * @param styles The @c std::vector of @c TextStyle objects where the parsed styles will be placed.
 * @param node The tree structure provided by the libwebvtt library, containing the parsed styles and text.
 */
void buildStyles(std::stringstream& cleanText, std::vector<TextStyle>& styles, const webvtt_node& node) {
    if (node.kind == WEBVTT_HEAD_NODE) {
        int dataLength = static_cast<int>(node.data.internal_data->length);
        for (int i = 0; i < dataLength; i++) {
            buildStyles(cleanText, styles, *node.data.internal_data->children[i]);
        }
    } else if (node.kind == WEBVTT_TEXT) {
        std::string childNodeText = static_cast<const char*>(webvtt_string_text(&node.data.text));
        cleanText << childNodeText;
        ACSDK_DEBUG9(LX("Node").d("kind", "WEBVTT_TEXT").sensitive("text", childNodeText));
    } else if (node.kind == WEBVTT_ITALIC || node.kind == WEBVTT_BOLD || node.kind == WEBVTT_UNDERLINE) {
        auto styleStart = TextStyle(styles.back());
        styleStart.charIndex = cleanText.str().length();
        int childNodeCount = static_cast<int>(node.data.internal_data->length);
        for (int i = 0; i < childNodeCount; i++) {
            buildStyles(cleanText, styles, *node.data.internal_data->children[i]);
        }
        auto styleEnd = TextStyle(styles.back());
        styleEnd.charIndex = cleanText.str().length();

        switch (node.kind) {
            case WEBVTT_ITALIC:
                styleStart.activeStyle.m_italic = true;
                styleEnd.activeStyle.m_italic = false;
                break;
            case WEBVTT_BOLD:
                styleStart.activeStyle.m_bold = true;
                styleEnd.activeStyle.m_bold = false;
                break;
            case WEBVTT_UNDERLINE:
                styleStart.activeStyle.m_underline = true;
                styleEnd.activeStyle.m_underline = false;
                break;
            default:
                break;
        }

        styles.emplace_back(styleStart);
        styles.emplace_back(styleEnd);
    } else {
        ACSDK_DEBUG9(LX("unsupportedNode").sensitive("kind", node.kind));
    }
}

/**
 * The callback function that is called when libwebvtt completes the parsing of a single caption frame.
 *
 * @param userdata The MediaPlayerSourceId that was sent in to the @c webvtt_create_parser().
 * @param cue The object containing the parsed caption frame and style information.
 */
void WEBVTT_CALLBACK onCueParsed(void* userdata, webvtt_cue* cue) {
    ACSDK_DEBUG7(LX(__func__));
    // Unpack and convert the values returned by libwebvtt.
    std::string body_text(static_cast<const char*>(webvtt_string_text(&cue->body)));
    auto start_time = static_cast<uint64_t>(cue->from);
    auto end_time = static_cast<uint64_t>(cue->until);

    std::stringstream cleanText;
    std::vector<TextStyle> styles;
    const webvtt_node* head = cue->node_head;

    if (head != nullptr) {
        // pre-load the styles vector with the basic/empty styles.
        styles.emplace_back(TextStyle{0, Style()});
        buildStyles(cleanText, styles, *head);
    } else {
        ACSDK_WARN(LX("libwebvtt returned a null node for style information."));
    }

    std::unique_lock<std::mutex> lock(g_mutex);
    // look up the captionId
    const uint64_t captionId = *static_cast<CaptionFrame::MediaPlayerSourceId*>(userdata);
    ACSDK_DEBUG9(LX("captionContentToCaptionIdLookup").d("captionId", captionId).d("userdataVoidPtr", userdata));

    // Calculate the delay and save the end time for the next frame.
    auto delayItr = g_captionIdsToLastEndTime.find(captionId);
    auto delayMs = std::chrono::milliseconds(0);
    if (delayItr == g_captionIdsToLastEndTime.end()) {
        ACSDK_WARN(LX("captionDelayInaccurate").d("reason", "lastEndTimeUnknown"));
    } else {
        delayMs = std::chrono::milliseconds(start_time) - delayItr->second;
    }
    g_captionIdsToLastEndTime[captionId] = std::chrono::milliseconds(end_time);
    ACSDK_DEBUG9(
        LX("captionTimesCalculated").d("delayMs", delayMs.count()).d("startTime", start_time).d("endTime", end_time));
    lock.unlock();

    if (g_parseListener != nullptr) {
        std::vector<CaptionLine> captionLines;

        // Remove any newlines found and translate them into CaptionLine objects.
        CaptionLine tempLine = CaptionLine{cleanText.str(), styles};
        std::stringstream ss(cleanText.str());
        std::string textLine;
        while (std::getline(ss, textLine)) {
            std::vector<CaptionLine> split = tempLine.splitAtTextIndex(textLine.length());
            captionLines.emplace_back(split[0]);
            if (split.size() == 2) {
                tempLine = split[1];
            }
        }

        // Build the CaptionFrame and send it back to the parse listener.
        CaptionFrame caption_frame =
            CaptionFrame(captionId, std::chrono::milliseconds(end_time - start_time), delayMs, captionLines);
        g_parseListener->onParsed(caption_frame);
        ACSDK_DEBUG9(LX("libwebvttSentParsedCaptionFrame"));
    } else {
        ACSDK_WARN(LX("libwebvttCannotSendParsedCaptionFrame").d("reason", "parseListenerIsNull"));
    }
}

/**
 * The callback function that is called when libwebvtt encounters an error during parsing.
 *
 * @param userdata The MediaPlayerSourceId that was sent in to the @c webvtt_create_parser().
 * @param line The line number in userdata that sourced the error.
 * @param col The column number in userdata that sourced the error.
 * @param errcode The error code describing the failure type.
 * @return The return code, unused in this implementation.
 */
int WEBVTT_CALLBACK onParseError(void* userdata, webvtt_uint line, webvtt_uint col, webvtt_error errcode) {
    const uint64_t captionId = *static_cast<CaptionFrame::MediaPlayerSourceId*>(userdata);
    ACSDK_ERROR(LX("libwebvttError")
                    .d("line", line)
                    .d("col", col)
                    .d("error code", errcode)
                    .d("error message", webvtt_strerror(errcode))
                    .d("captionId", captionId)
                    .d("userdataVoidPtr", userdata));
    return WEBVTT_CALLBACK_ERROR;
}

void LibwebvttParserAdapter::parse(CaptionFrame::MediaPlayerSourceId captionId, const CaptionData& captionData) {
    ACSDK_DEBUG7(LX(__func__));
    webvtt_parser vtt;
    webvtt_status result;

    auto captionContentVp = static_cast<const void*>(captionData.content.c_str());
    std::unique_lock<std::mutex> lock(g_mutex);
    g_captionSourceIds.emplace_back(captionId);
    size_t sourceIdIndex = g_captionSourceIds.size() - 1;
    g_captionIdsToLastEndTime.emplace(captionId, std::chrono::milliseconds(0));
    lock.unlock();
    ACSDK_DEBUG9(
        LX("captionContentToCaptionIdCreation").d("content_cp_to_vp", captionContentVp).d("captionId", captionId));

    result = webvtt_create_parser((webvtt_cue_fn)&onCueParsed, &onParseError, &g_captionSourceIds[sourceIdIndex], &vtt);
    if (result != WEBVTT_SUCCESS) {
        ACSDK_ERROR(LX("failed to create WebVTT parser").d("webvtt_status", result));
        return;
    }

    result = webvtt_parse_chunk(vtt, captionContentVp, static_cast<webvtt_uint>(captionData.content.length()));
    if (result != WEBVTT_SUCCESS) {
        ACSDK_ERROR(LX("WebVTT parser failed to parse"));
        return;
    }
    ACSDK_DEBUG9(LX("libwebvttFinished"));
    webvtt_finish_parsing(vtt);

    webvtt_delete_parser(vtt);
}

void LibwebvttParserAdapter::addListener(std::shared_ptr<CaptionFrameParseListenerInterface> listener) {
    ACSDK_DEBUG7(LX(__func__));
    g_parseListener = listener;
}

void LibwebvttParserAdapter::releaseResourcesFor(CaptionFrame::MediaPlayerSourceId captionId) {
    ACSDK_DEBUG7(LX(__func__).d("captionId", captionId));

    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto it1 = g_captionSourceIds.begin(); it1 != g_captionSourceIds.end(); ++it1) {
        if (captionId == *it1) {
            g_captionSourceIds.erase(it1);
            break;
        }
    }

    auto it2 = g_captionIdsToLastEndTime.find(captionId);
    if (it2 != g_captionIdsToLastEndTime.end()) {
        g_captionIdsToLastEndTime.erase(it2);
    }
}

}  // namespace captions
}  // namespace alexaClientSDK