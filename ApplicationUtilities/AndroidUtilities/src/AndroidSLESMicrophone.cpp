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

#include <AndroidUtilities/AndroidSLESMicrophone.h>

#include <SLES/OpenSLES_Android.h>
#include <AndroidUtilities/AndroidSLESEngine.h>

/// String to identify log entries originating from this file.
static const std::string TAG{"AndroidMicrophone"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {

std::unique_ptr<AndroidSLESMicrophone> AndroidSLESMicrophone::create(
    std::shared_ptr<AndroidSLESEngine> engine,
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream) {
    std::shared_ptr<avsCommon::avs::AudioInputStream::Writer> writer =
        stream->createWriter(avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE);
    if (!writer) {
        ACSDK_ERROR(LX("createAndroidSLESMicrophoneFailed").d("reason", "failed to create writer"));
        return nullptr;
    }

    return std::unique_ptr<AndroidSLESMicrophone>(new AndroidSLESMicrophone(engine, writer));
}

AndroidSLESMicrophone::AndroidSLESMicrophone(
    std::shared_ptr<AndroidSLESEngine> engine,
    std::shared_ptr<avsCommon::avs::AudioInputStream::Writer> writer) :
        m_engineObject{engine},
        m_writer{writer},
        m_isStreaming{false} {
}

SLDataSink AndroidSLESMicrophone::createSinkConfiguration() {
    static SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,     // type
                                                            AndroidSLESBufferQueue::NUMBER_OF_BUFFERS};  // # buffers

    static SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM,            // PCM Format
                                          1,                            // 1 Channel
                                          SL_SAMPLINGRATE_16,           // 16 kHz sampling rate
                                          SL_PCMSAMPLEFORMAT_FIXED_16,  // Fixed-point 16-bit samples
                                          SL_PCMSAMPLEFORMAT_FIXED_16,  // 16 bit container
                                          SL_SPEAKER_FRONT_CENTER,      // channel mask
                                          SL_BYTEORDER_LITTLEENDIAN};   // Data endianness
    return SLDataSink{&loc_bq, &format_pcm};
}

SLDataSource AndroidSLESMicrophone::createSourceConfiguration() {
    static SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE,        // Locator type
                                             SL_IODEVICE_AUDIOINPUT,         // Type of audio device
                                             SL_DEFAULTDEVICEID_AUDIOINPUT,  // Use default device id
                                             nullptr};  // Using default device so no device object needed
    return SLDataSource{&loc_dev, nullptr};
}

AndroidSLESMicrophone::~AndroidSLESMicrophone() {
    stopStreamingMicrophoneData();
}

bool AndroidSLESMicrophone::configureRecognizeMode() {
    SLAndroidConfigurationItf configurationInterface;
    if (!m_recorderObject) {
        ACSDK_ERROR(LX("configureRecognizeModeFailed").d("reason", "recorderObjectUnavailable"));
        return false;
    }

    if (!m_recorderObject->getInterface(SL_IID_RECORD, &configurationInterface) || !configurationInterface) {
        ACSDK_WARN(LX("configureRecognizeModeFailed")
                       .d("reason", "configurationInterfaceUnavailable")
                       .d("configuration", configurationInterface));
        return false;
    }

    auto presetValue = SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION;
    auto result =
        (*configurationInterface)
            ->SetConfiguration(configurationInterface, SL_ANDROID_KEY_RECORDING_PRESET, &presetValue, sizeof(SLuint32));
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_WARN(LX("configureRecognizeModeFailed").d("reason", "cannot set configuration").d("result", result));
        return false;
    }
    return true;
}

bool AndroidSLESMicrophone::isStreaming() {
    return m_isStreaming;
}

bool AndroidSLESMicrophone::startStreamingMicrophoneData() {
    ACSDK_INFO(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    m_recorderObject = m_engineObject->createAudioRecorder();
    if (!m_recorderObject) {
        ACSDK_ERROR(LX("startStreamingFailed").d("reason", "Failed to create recorder."));
        return false;
    }

    if (!m_recorderObject->getInterface(SL_IID_RECORD, &m_recorderInterface)) {
        ACSDK_ERROR(LX("startStreamingFailed").d("reason", "Failed to get recorder interface."));
        return false;
    }

    m_queue = AndroidSLESBufferQueue::create(m_recorderObject, m_writer);
    if (!m_queue) {
        ACSDK_ERROR(LX("startStreamingFailed").d("reason", "Failed to create buffer queue."));
        return false;
    }

    if (!configureRecognizeMode()) {
        ACSDK_WARN(LX("Failed to set Recognize mode. This might affect the voice recognition."));
    }

    if (!m_queue->enqueueBuffers()) {
        ACSDK_ERROR(LX("startStreamingFailed").d("reason", "Failed to enqueue buffers."));
        return false;
    }

    // start recording
    auto result = (*m_recorderInterface)->SetRecordState(m_recorderInterface, SL_RECORDSTATE_RECORDING);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("startStreamingFailed").d("reason", "failed to set state").d("result", result));
        return false;
    }

    m_isStreaming = true;
    return true;
}

bool AndroidSLESMicrophone::stopStreamingMicrophoneData() {
    ACSDK_INFO(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};

    if (m_recorderObject && m_recorderInterface) {
        auto result = (*m_recorderInterface)->SetRecordState(m_recorderInterface, SL_RECORDSTATE_STOPPED);
        if (result != SL_RESULT_SUCCESS) {
            ACSDK_ERROR(LX("stopStreamingFailed").d("result", result));
            return false;
        }

        // Unblock Android microphone.
        m_recorderObject.reset();
        m_recorderInterface = nullptr;
        m_queue.reset();
        m_isStreaming = false;
        return true;
    } else {
        ACSDK_ERROR(LX("stopStreamingFailed").d("reason", "Recorder object or interface not available."));
        return false;
    }
}

}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
