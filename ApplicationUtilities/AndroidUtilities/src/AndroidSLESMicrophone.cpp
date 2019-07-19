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

AndroidSLESMicrophone::AndroidSLESMicrophone(
    std::shared_ptr<AndroidSLESEngine> engine,
    std::shared_ptr<AndroidSLESObject> recorderObject,
    SLRecordItf recorderInterface,
    std::unique_ptr<AndroidSLESBufferQueue> queue) :
        m_engineObject{engine},
        m_recorderObject{std::move(recorderObject)},
        m_recorderInterface{recorderInterface},
        m_queue(std::move(queue)) {
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
    if (!m_recorderObject->getInterface(SL_IID_RECORD, &configurationInterface) || !configurationInterface) {
        ACSDK_ERROR(LX("configureRecognizeModeFailed")
                        .d("reason", "configurationInterfaceUnavailable")
                        .d("configuration", configurationInterface));
        return false;
    }

    auto presetValue = SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION;
    auto result =
        (*configurationInterface)
            ->SetConfiguration(configurationInterface, SL_ANDROID_KEY_RECORDING_PRESET, &presetValue, sizeof(SLuint32));
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("configureRecognizeModeFailed").d("reason", "cannot set configuration").d("result", result));
        return false;
    }
    return true;
}

bool AndroidSLESMicrophone::startStreamingMicrophoneData() {
    ACSDK_INFO(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};

    // Ensure that the buffers are clean
    stopLocked();

    if (!m_queue->enqueueBuffers()) {
        ACSDK_ERROR(LX("startStreamingFailed").d("reason", "failed to enqueue buffers"));
        return false;
    }

    // start recording
    auto result = (*m_recorderInterface)->SetRecordState(m_recorderInterface, SL_RECORDSTATE_RECORDING);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("startStreamingFailed").d("reason", "failed to set state").d("result", result));
        return false;
    }

    return true;
}

bool AndroidSLESMicrophone::stopStreamingMicrophoneData() {
    ACSDK_INFO(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    return stopLocked();
}

bool AndroidSLESMicrophone::stopLocked() {
    auto result = (*m_recorderInterface)->SetRecordState(m_recorderInterface, SL_RECORDSTATE_STOPPED);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("stopStreamingFailed").d("result", result));
        return false;
    }

    return m_queue->clearBuffers();
}

}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
