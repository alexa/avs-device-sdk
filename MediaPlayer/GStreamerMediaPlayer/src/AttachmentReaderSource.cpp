/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <cstring>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/AVS/Attachment/AttachmentReader.h>

#include "MediaPlayer/AttachmentReaderSource.h"

namespace alexaClientSDK {
namespace mediaPlayer {

using namespace avsCommon::utils;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::avs::attachment;

/// String to identify log entries originating from this file.
static const std::string TAG("AttachmentReaderSource");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The number of bytes read from the attachment with each read in the read loop.
static const unsigned int CHUNK_SIZE(4096);

std::unique_ptr<AttachmentReaderSource> AttachmentReaderSource::create(
    PipelineInterface* pipeline,
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
    const avsCommon::utils::AudioFormat* audioFormat,
    bool repeat) {
    std::unique_ptr<AttachmentReaderSource> result(new AttachmentReaderSource(pipeline, attachmentReader, repeat));
    if (result->init(audioFormat)) {
        return result;
    }
    return nullptr;
};

AttachmentReaderSource::~AttachmentReaderSource() {
    close();
}

AttachmentReaderSource::AttachmentReaderSource(
    PipelineInterface* pipeline,
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> reader,
    bool repeat) :
        BaseStreamSource{pipeline, "AttachmentReaderSource"},
        m_reader{reader},
        m_repeat{repeat} {};

bool AttachmentReaderSource::isPlaybackRemote() const {
    return false;
}

bool AttachmentReaderSource::isOpen() {
    return m_reader != nullptr;
}

void AttachmentReaderSource::close() {
    if (m_reader) {
        m_reader->close();
    }
    m_reader.reset();
}

gboolean AttachmentReaderSource::handleReadData() {
    if (!m_reader) {
        ACSDK_ERROR(LX("handleReadDataFailed").d("reason", "attachmentReaderIsNullPtr"));
        return false;
    }

    auto buffer = gst_buffer_new_allocate(nullptr, CHUNK_SIZE, nullptr);

    if (!buffer) {
        ACSDK_ERROR(LX("handleReadDataFailed").d("reason", "gstBufferNewAllocateFailed"));
        signalEndOfData();
        return false;
    }

    GstMapInfo info;
    if (!gst_buffer_map(buffer, &info, GST_MAP_WRITE)) {
        ACSDK_ERROR(LX("handleReadDataFailed").d("reason", "gstBufferMapFailed"));
        gst_buffer_unref(buffer);
        signalEndOfData();
        return false;
    }

    ACSDK_DEBUG9(LX("beforeRead").d("size", info.size));

    auto status = AttachmentReader::ReadStatus::OK;
    auto size = m_reader->read(info.data, info.size, &status, std::chrono::milliseconds(1));

    ACSDK_DEBUG9(LX("read").d("size", size).d("status", static_cast<int>(status)));

    gst_buffer_unmap(buffer, &info);

    if (size > 0 && size < info.size) {
        gst_buffer_resize(buffer, 0, size);
    }

    switch (status) {
        case AttachmentReader::ReadStatus::CLOSED:
            if (0 == size) {
                break;
            }
        // Fall through if some data was read.
        case AttachmentReader::ReadStatus::OK:
        case AttachmentReader::ReadStatus::OK_WOULDBLOCK:
        // Fall through to retry reading later.
        case AttachmentReader::ReadStatus::OK_TIMEDOUT:
            if (size > 0) {
                installOnReadDataHandler();
                auto flowRet = gst_app_src_push_buffer(getAppSrc(), buffer);
                if (flowRet != GST_FLOW_OK) {
                    ACSDK_ERROR(LX("handleReadDataFailed")
                                    .d("reason", "gstAppSrcPushBufferFailed")
                                    .d("error", gst_flow_get_name(flowRet)));
                    break;
                }
            } else {
                gst_buffer_unref(buffer);
                updateOnReadDataHandler();
            }
            return true;
        case AttachmentReader::ReadStatus::OK_OVERRUN_RESET:  // gstreamer requires stable stream.
        case AttachmentReader::ReadStatus::ERROR_OVERRUN:
        case AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
        case AttachmentReader::ReadStatus::ERROR_INTERNAL:
            auto error = static_cast<int>(status);
            ACSDK_ERROR(LX("handleReadDataFailed").d("reason", "readFailed").d("error", error));
            break;
    }

    if (!m_repeat) {
        ACSDK_DEBUG9(LX("handleReadData").d("info", "signalingEndOfData"));
        gst_buffer_unref(buffer);
        signalEndOfData();
        return false;
    }

    m_reader->seek(0);
    gst_buffer_unref(buffer);
    updateOnReadDataHandler();
    return true;
}

gboolean AttachmentReaderSource::handleSeekData(guint64 offset) {
    ACSDK_DEBUG9(LX("handleSeekData").d("offset", offset));
    if (m_reader) {
        return m_reader->seek(offset);
    } else {
        ACSDK_ERROR(LX("handleSeekDataFailed").d("reason", "nullReader"));
        return false;
    }
}

}  // namespace mediaPlayer
}  // namespace alexaClientSDK
