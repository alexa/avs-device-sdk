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

/**
 * Method that returns a string to be used in CAPS negotiation (generating right PADS between gstreamer elements based
 * on audio data.) For raw PCM data without header audioFormat information needs to be passed explicitly for a
 * mediaplayer to interpret the audio bytes.
 */
static std::string getCapsString(const AudioFormat& audioFormat) {
    std::stringstream caps;
    switch (audioFormat.encoding) {
        case AudioFormat::Encoding::LPCM:
            caps << "audio/x-raw";
            break;
        case AudioFormat::Encoding::OPUS:
            ACSDK_ERROR(LX("MediaPlayer does not handle OPUS data"));
            caps << " ";
            break;
    }

    switch (audioFormat.endianness) {
        case AudioFormat::Endianness::LITTLE:
            audioFormat.dataSigned ? caps << ",format=S" << audioFormat.sampleSizeInBits << "LE"
                                   : caps << ",format=U" << audioFormat.sampleSizeInBits << "LE";
            break;
        case AudioFormat::Endianness::BIG:
            audioFormat.dataSigned ? caps << ",format=S" << audioFormat.sampleSizeInBits << "BE"
                                   : caps << ",format=U" << audioFormat.sampleSizeInBits << "BE";
            break;
    }

    switch (audioFormat.layout) {
        case AudioFormat::Layout::INTERLEAVED:
            caps << ",layout=interleaved";
            break;
        case AudioFormat::Layout::NON_INTERLEAVED:
            caps << ",layout=non-interleaved";
            break;
    }

    caps << ",channels=" << audioFormat.numChannels;
    caps << ",rate=" << audioFormat.sampleRateHz;

    return caps.str();
}

BaseStreamSource::BaseStreamSource(PipelineInterface* pipeline, const std::string& className) :
        SourceInterface(className),
        m_pipeline{pipeline},
        m_sourceId{0},
        m_sourceRetryCount{0},
        m_handleNeedDataFunction{[this]() { return handleNeedData(); }},
        m_handleEnoughDataFunction{[this]() { return handleEnoughData(); }},
        m_needDataHandlerId{0},
        m_enoughDataHandlerId{0},
        m_seekDataHandlerId{0},
        m_needDataCallbackId{0},
        m_enoughDataCallbackId{0} {
}

BaseStreamSource::~BaseStreamSource() {
    ACSDK_DEBUG9(LX("~BaseStreamSource"));
    g_signal_handler_disconnect(m_pipeline->getAppSrc(), m_needDataHandlerId);
    g_signal_handler_disconnect(m_pipeline->getAppSrc(), m_enoughDataHandlerId);
    g_signal_handler_disconnect(m_pipeline->getAppSrc(), m_seekDataHandlerId);
    if (m_pipeline->getPipeline()) {
        if (m_pipeline->getAppSrc()) {
            gst_bin_remove(GST_BIN(m_pipeline->getPipeline()), GST_ELEMENT(m_pipeline->getAppSrc()));
        }
        m_pipeline->setAppSrc(nullptr);

        if (m_pipeline->getDecoder()) {
            gst_bin_remove(GST_BIN(m_pipeline->getPipeline()), GST_ELEMENT(m_pipeline->getDecoder()));
        }
        m_pipeline->setDecoder(nullptr);
    }
    {
        std::lock_guard<std::mutex> lock(m_callbackIdMutex);
        if (m_needDataCallbackId && !g_source_remove(m_needDataCallbackId)) {
            ACSDK_ERROR(LX("gSourceRemove failed for m_needDataCallbackId"));
        }
        if (m_enoughDataCallbackId && !g_source_remove(m_enoughDataCallbackId)) {
            ACSDK_ERROR(LX("gSourceRemove failed for m_enoughDataCallbackId"));
        }
    }
    uninstallOnReadDataHandler();
}

bool BaseStreamSource::init(const AudioFormat* audioFormat) {
    auto appsrc = reinterpret_cast<GstAppSrc*>(gst_element_factory_make("appsrc", "src"));
    if (!appsrc) {
        ACSDK_ERROR(LX("initFailed").d("reason", "createSourceElementFailed"));
        return false;
    }
    gst_app_src_set_stream_type(appsrc, GST_APP_STREAM_TYPE_SEEKABLE);

    GstCaps* audioCaps = nullptr;

    if (audioFormat) {
        std::string caps = getCapsString(*audioFormat);
        audioCaps = gst_caps_from_string(caps.c_str());
        if (!audioCaps) {
            ACSDK_ERROR(LX("BaseStreamSourceInitFailed").d("reason", "capsNullForRawAudioFormat"));
            return false;
        }
        gst_app_src_set_caps(GST_APP_SRC(appsrc), audioCaps);
        g_object_set(G_OBJECT(appsrc), "format", GST_FORMAT_TIME, NULL);
    } else {
        ACSDK_DEBUG9(LX("initNoAudioFormat"));
    }

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
    if (!gst_element_link(reinterpret_cast<GstElement*>(appsrc), decoder)) {
        ACSDK_ERROR(LX("initFailed").d("reason", "createSourceToDecoderLinkFailed"));
        return false;
    }
    /*
     * When the appsrc needs data, it emits the signal need-data. Connect the need-data signal to the onNeedData
     * callback which handles pushing data to the appsrc element.
     */
    m_needDataHandlerId = g_signal_connect(appsrc, "need-data", G_CALLBACK(onNeedData), this);
    if (0 == m_needDataHandlerId) {
        ACSDK_ERROR(LX("initFailed").d("reason", "connectNeedDataSignalFailed"));
        return false;
    }
    /*
     * When the appsrc had enough data, it emits the signal enough-data. Connect the enough-data signal to the
     * onEnoughData callback which handles stopping the data push to the appsrc element.
     */
    m_enoughDataHandlerId = g_signal_connect(appsrc, "enough-data", G_CALLBACK(onEnoughData), this);
    if (0 == m_enoughDataHandlerId) {
        ACSDK_ERROR(LX("initFailed").d("reason", "connectEnoughDataSignalFailed"));
        return false;
    }
    /*
     * When the appsrc needs to seek to a position, it emits the signal seek-data. Connect the seek-data signal to the
     * onSeekData callback which handles seeking to the appropriate position.
     */
    m_seekDataHandlerId = g_signal_connect(appsrc, "seek-data", G_CALLBACK(onSeekData), this);
    if (0 == m_seekDataHandlerId) {
        ACSDK_ERROR(LX("initFailed").d("reason", "connectSeekDataSignalFailed"));
        return false;
    }

    if (audioCaps) {
        gst_caps_unref(audioCaps);
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
                        .d("reason", "gstAppSrcEndOfStreamFailed")
                        .d("result", gst_flow_get_name(flowRet)));
    }
    ACSDK_DEBUG9(LX("gstAppSrcEndOfStreamSuccess"));
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
                ACSDK_ERROR(
                    LX("installOnReadDataHandlerError").d("reason", "gSourceRemoveFailed").d("sourceId", m_sourceId));
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
            ACSDK_ERROR(
                LX("updateOnReadDataHandlerError").d("reason", "gSourceRemoveFailed").d("sourceId", m_sourceId));
        }
        auto interval = RETRY_INTERVALS_MILLISECONDS[m_sourceRetryCount];
        m_sourceRetryCount++;
        m_sourceId = g_timeout_add(interval, reinterpret_cast<GSourceFunc>(&onReadData), this);
        ACSDK_DEBUG9(LX("updateOnReadDataHandlerNewSourceId")
                         .d("action", "newSourceId")
                         .d("sourceId", m_sourceId)
                         .d("sourceRetryCount", m_sourceRetryCount));
    }
}

void BaseStreamSource::uninstallOnReadDataHandler() {
    if (m_sourceId != 0) {
        if (!g_source_remove(m_sourceId)) {
            ACSDK_ERROR(
                LX("uninstallOnReadDataHandlerError").d("reason", "gSourceRemoveFailed").d("sourceId", m_sourceId));
        }
        clearOnReadDataHandler();
    }
}

void BaseStreamSource::clearOnReadDataHandler() {
    ACSDK_DEBUG9(LX("clearOnReadDataHandlerCalled").d("sourceId", m_sourceId));
    m_sourceRetryCount = 0;
    m_sourceId = 0;
}

void BaseStreamSource::onNeedData(GstElement* pipeline, guint size, gpointer pointer) {
    ACSDK_DEBUG9(LX("onNeedDataCalled").d("size", size));
    auto source = static_cast<BaseStreamSource*>(pointer);
    std::lock_guard<std::mutex> lock(source->m_callbackIdMutex);
    if (source->m_needDataCallbackId) {
        ACSDK_DEBUG9(LX("m_needDataCallbackId already set"));
        return;
    }
    source->m_needDataCallbackId = source->m_pipeline->queueCallback(&source->m_handleNeedDataFunction);
}

gboolean BaseStreamSource::handleNeedData() {
    ACSDK_DEBUG9(LX("handleNeedDataCalled"));
    std::lock_guard<std::mutex> lock(m_callbackIdMutex);
    m_needDataCallbackId = 0;
    installOnReadDataHandler();
    return false;
}

void BaseStreamSource::onEnoughData(GstElement* pipeline, gpointer pointer) {
    ACSDK_DEBUG9(LX("onEnoughDataCalled"));
    auto source = static_cast<BaseStreamSource*>(pointer);
    std::lock_guard<std::mutex> lock(source->m_callbackIdMutex);
    if (source->m_enoughDataCallbackId) {
        ACSDK_DEBUG9(LX("m_enoughDataCallbackId already set"));
        return;
    }
    source->m_enoughDataCallbackId = source->m_pipeline->queueCallback(&source->m_handleEnoughDataFunction);
}

gboolean BaseStreamSource::handleEnoughData() {
    ACSDK_DEBUG9(LX("handleEnoughDataCalled"));
    std::lock_guard<std::mutex> lock(m_callbackIdMutex);
    m_enoughDataCallbackId = 0;
    uninstallOnReadDataHandler();
    return false;
}

gboolean BaseStreamSource::onSeekData(GstElement* pipeline, guint64 offset, gpointer pointer) {
    return static_cast<BaseStreamSource*>(pointer)->handleSeekData(offset);
}

gboolean BaseStreamSource::onReadData(gpointer pointer) {
    return static_cast<BaseStreamSource*>(pointer)->handleReadData();
}

// No additional processing is necessary.
bool BaseStreamSource::handleEndOfStream() {
    return true;
}

// Source streams do not contain additional data.
bool BaseStreamSource::hasAdditionalData() {
    return false;
}

void BaseStreamSource::preprocess() {
}

}  // namespace mediaPlayer
}  // namespace alexaClientSDK
