/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "MediaPlayer/IStreamSource.h"

namespace alexaClientSDK {
namespace mediaPlayer {

using namespace avsCommon::utils;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::avs::attachment;

/// String to identify log entries originating from this file.
static const std::string TAG("IStreamSource");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The number of bytes read from the attachment with each read in the read loop.
static const unsigned int CHUNK_SIZE(4096);

std::unique_ptr<IStreamSource> IStreamSource::create(
    PipelineInterface* pipeline,
    std::shared_ptr<std::istream> stream,
    bool repeat) {
    std::unique_ptr<IStreamSource> result(new IStreamSource(pipeline, std::move(stream), repeat));
    if (result->init()) {
        return result;
    }
    return nullptr;
};

IStreamSource::IStreamSource(PipelineInterface* pipeline, std::shared_ptr<std::istream> stream, bool repeat) :
        BaseStreamSource{pipeline, "IStreamSource"},
        m_stream{stream},
        m_repeat{repeat} {};

IStreamSource::~IStreamSource() {
    close();
}

bool IStreamSource::isPlaybackRemote() const {
    return false;
}

bool IStreamSource::hasAdditionalData() {
    if (!m_repeat) {
        return false;
    }
    m_stream->clear();
    m_stream->seekg(0);
    return true;
}

bool IStreamSource::isOpen() {
    return m_stream != nullptr;
}

void IStreamSource::close() {
    m_stream.reset();
}

gboolean IStreamSource::handleReadData() {
    if (!isOpen()) {
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

    if (m_repeat && m_stream->eof()) {
        m_stream->clear();
        m_stream->seekg(0);
    }

    m_stream->read(reinterpret_cast<std::istream::char_type*>(info.data), info.size);

    unsigned long size;
    if (m_stream->bad()) {
        size = 0;
        ACSDK_WARN(LX("readFailed").d("bad", m_stream->bad()).d("eof", m_stream->eof()));
    } else {
        size = m_stream->gcount();
        ACSDK_DEBUG9(LX("read").d("size", size).d("pos", m_stream->tellg()).d("eof", m_stream->eof()));
    }

    gst_buffer_unmap(buffer, &info);

    if (size > 0) {
        if (size < info.size) {
            gst_buffer_resize(buffer, 0, size);
        }
        installOnReadDataHandler();
        auto flowRet = gst_app_src_push_buffer(getAppSrc(), buffer);
        if (flowRet != GST_FLOW_OK) {
            ACSDK_ERROR(LX("handleReadDataFailed")
                            .d("reason", "gstAppSrcPushBufferFailed")
                            .d("error", gst_flow_get_name(flowRet)));
            return false;
        } else {
            return true;
        }
    }

    if (m_stream->bad() || (!m_repeat && m_stream->eof())) {
        gst_buffer_unref(buffer);
        signalEndOfData();
        return false;
    }

    gst_buffer_unref(buffer);
    updateOnReadDataHandler();
    return true;
}

gboolean IStreamSource::handleSeekData(guint64 offset) {
    m_stream->clear();
    m_stream->seekg(offset);
    return true;
}

}  // namespace mediaPlayer
}  // namespace alexaClientSDK
