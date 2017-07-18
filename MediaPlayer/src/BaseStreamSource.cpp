/*
 * BaseStreamSource.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "MediaPlayer/BaseStreamSource.h"

namespace alexaClientSDK {
namespace mediaPlayer {

using namespace avsCommon::utils;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::avs::attachment;

/// String to identify log entries originating from this file.
static const std::string TAG("BaseStreamSource");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The interval to wait (in milliseconds) between successive attempts to read audio data when none is available.
static const guint RETRY_INTERVALS_MILLISECONDS[] = {0, 10, 10, 10, 20, 20, 50, 100};

BaseStreamSource::BaseStreamSource(PipelineInterface* pipeline) :
        m_pipeline{pipeline},
        m_sourceId{0},
        m_sourceRetryCount{0},
        m_handleNeedDataFunction{[this]() { return handleNeedData(); }},
        m_handleEnoughDataFunction{[this]() { return handleEnoughData(); }} {
}

BaseStreamSource::~BaseStreamSource() {
    uninstallOnReadDataHandler();
}

bool BaseStreamSource::init() {
    auto appsrc = reinterpret_cast<GstAppSrc *>(gst_element_factory_make("appsrc", "src"));
    if (!appsrc) {
        ACSDK_ERROR(LX("initFailed").d("reason", "createSourceElementFailed"));
        return false;
    }
    gst_app_src_set_stream_type(appsrc, GST_APP_STREAM_TYPE_STREAM);

    auto decoder = gst_element_factory_make("decodebin", "decoder");
    if (!decoder) {
        ACSDK_ERROR(LX("initFailed").d("reason", "createDecoderElementFailed"));
        return false;
    }

    if (!m_pipeline) {
        ACSDK_ERROR(LX("initFailed").d("reason", "pipelineIsNotSet"));
        return false;
    }

    if (!gst_bin_add(GST_BIN(m_pipeline->getPipeline()), reinterpret_cast<GstElement*>(appsrc))) {
        ACSDK_ERROR(LX("initFailed").d("reason", "addingAppSrcToPipelineFailed"));
        return false;
    }

    if (!gst_bin_add(GST_BIN(m_pipeline->getPipeline()), decoder)) {
        ACSDK_ERROR(LX("initFailed").d("reason", "addingDecoderToPipelineFailed"));
        return false;
    }

    /*
     * Link the source and decoder elements. The decoder source pad is added dynamically after it has determined the
     * stream type it is decoding. Once the pad has been added, pad-added signal is emitted, and the padAddedHandler
     * callback will link the newly created source pad of the decoder to the sink of the converter element.
     */
    if (!gst_element_link(reinterpret_cast<GstElement *>(appsrc), decoder)) {
        ACSDK_ERROR(LX("initFailed").d("reason", "createSourceToDecoderLinkFailed"));
        return false;
    }
    /*
     * When the appsrc needs data, it emits the signal need-data. Connect the need-data signal to the onNeedData
     * callback which handles pushing data to the appsrc element.
     */
    if (!g_signal_connect(appsrc, "need-data", G_CALLBACK(onNeedData), this)) {
        ACSDK_ERROR(LX("initFailed").d("reason", "connectNeedDataSignalFailed"));
        return false;
    }
    /*
     * When the appsrc had enough data, it emits the signal enough-data. Connect the enough-data signal to the
     * onEnoughData callback which handles stopping the data push to the appsrc element.
     */
    if (!g_signal_connect(appsrc, "enough-data", G_CALLBACK(onEnoughData), this)) {
        ACSDK_ERROR(LX("initFailed").d("reason", "connectEnoughDataSignalFailed"));
        return false;
    }

    m_pipeline->setAppSrc(appsrc);
    m_pipeline->setDecoder(decoder);

    return true;
}

GstAppSrc* BaseStreamSource::getAppSrc() const {
    if (!m_pipeline) {
        return nullptr;
    }
    return m_pipeline->getAppSrc();
}

void BaseStreamSource::signalEndOfData() {
    ACSDK_DEBUG9(LX("signalEndOfDataCalled"));
    auto flowRet = gst_app_src_end_of_stream(m_pipeline->getAppSrc());
    if (flowRet != GST_FLOW_OK) {
        ACSDK_ERROR(LX("signalEndOfDataFailed")
                .d("reason", "gstAppSrcEndOfStreamFailed").d("result", gst_flow_get_name(flowRet)));
    }
    close();
    clearOnReadDataHandler();
}

void BaseStreamSource::installOnReadDataHandler() {
    if (!isOpen()) {
        return;
    }
    if (m_sourceId != 0) {
        // Remove the existing source if it was timer based.  Otherwise it is already properly installed.
        if (m_sourceRetryCount != 0) {
            ACSDK_DEBUG9(LX("installOnReadDataHandler").d("action", "removeSourceId").d("sourceId", m_sourceId));
            if (!g_source_remove(m_sourceId)) {
                ACSDK_ERROR(LX("installOnReadDataHandlerError")
                        .d("reason", "gSourceRemoveFailed").d("sourceId", m_sourceId));
            }
        } else {
            return;
        }
    }
    m_sourceRetryCount = 0;
    m_sourceId = g_idle_add(reinterpret_cast<GSourceFunc>(&onReadData), this);
    ACSDK_DEBUG9(LX("installOnReadDataHandler").d("action", "newSourceId").d("sourceId", m_sourceId));
}

void BaseStreamSource::updateOnReadDataHandler() {
    if (m_sourceRetryCount < sizeof(RETRY_INTERVALS_MILLISECONDS) / sizeof(RETRY_INTERVALS_MILLISECONDS[0])) {
        ACSDK_DEBUG9(LX("updateOnReadDataHandler").d("action", "removeSourceId").d("sourceId", m_sourceId));
        if (!g_source_remove(m_sourceId)) {
            ACSDK_ERROR(LX("updateOnReadDataHandlerError")
                    .d("reason", "gSourceRemoveFailed").d("sourceId", m_sourceId));
        }
        auto interval = RETRY_INTERVALS_MILLISECONDS[m_sourceRetryCount];
        m_sourceRetryCount++;
        m_sourceId = g_timeout_add(interval, reinterpret_cast<GSourceFunc>(&onReadData), this);
        ACSDK_DEBUG9(LX("updateOnReadDataHandlerNewSourceId")
                .d("action", "newSourceId").d("sourceId", m_sourceId).d("sourceRetryCount", m_sourceRetryCount));
    }
}

void BaseStreamSource::uninstallOnReadDataHandler() {
    if (m_sourceId != 0) {
        if (!g_source_remove(m_sourceId)) {
            ACSDK_ERROR(LX("uninstallOnReadDataHandlerError")
                    .d("reason", "gSourceRemoveFailed").d("sourceId", m_sourceId));
        }
        clearOnReadDataHandler();
    }
}

void BaseStreamSource::clearOnReadDataHandler() {
    ACSDK_DEBUG9(LX("clearOnReadDataHandlerCalled"));
    m_sourceRetryCount = 0;
    m_sourceId = 0;
}

void BaseStreamSource::onNeedData(GstElement *pipeline, guint size, gpointer pointer) {
    ACSDK_DEBUG9(LX("onNeedDataCalled").d("size", size));
    auto source = static_cast<BaseStreamSource*>(pointer);
    source->m_pipeline->queueCallback(&source->m_handleNeedDataFunction);
}

gboolean BaseStreamSource::handleNeedData() {
    ACSDK_DEBUG9(LX("handleNeedDataCalled"));
    installOnReadDataHandler();
    return false;
}

void BaseStreamSource::onEnoughData(GstElement *pipeline, gpointer pointer) {
    ACSDK_DEBUG9(LX("onEnoughDataCalled"));
    auto source = static_cast<BaseStreamSource*>(pointer);
    source->m_pipeline->queueCallback(&source->m_handleEnoughDataFunction);
}

gboolean BaseStreamSource::handleEnoughData() {
    ACSDK_DEBUG9(LX("handleEnoughDataCalled"));
    uninstallOnReadDataHandler();
    return false;
}

gboolean BaseStreamSource::onReadData(gpointer pointer) {
    return static_cast<BaseStreamSource*>(pointer)->handleReadData();
}

} // namespace mediaPlayer
} // namespace alexaClientSDK
