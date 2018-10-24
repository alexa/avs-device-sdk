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

#include <AndroidUtilities/AndroidSLESEngine.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <AVSCommon/AVS/AudioInputStream.h>
#include <AndroidUtilities/AndroidSLESMicrophone.h>

/// String to identify log entries originating from this file.
static const std::string TAG{"AndroidSLESEngine"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {

using namespace avsCommon::utils::memory;

std::atomic_flag AndroidSLESEngine::m_created = ATOMIC_FLAG_INIT;

std::shared_ptr<AndroidSLESEngine> AndroidSLESEngine::create() {
    if (!m_created.test_and_set()) {
        SLObjectItf slObjectItf;
        auto result = slCreateEngine(&slObjectItf, 0, nullptr, 0, nullptr, nullptr);
        if (result != SL_RESULT_SUCCESS) {
            ACSDK_ERROR(LX("createAndroidSLESEngineFailed").d("result", result));
            m_created.clear();
            return nullptr;
        }

        auto engineObject = AndroidSLESObject::create(slObjectItf);
        if (!engineObject) {
            ACSDK_ERROR(LX("createAndroidSLESEngineFailed"));
            m_created.clear();
            return nullptr;
        }

        SLEngineItf engineInterface;
        if (!engineObject->getInterface(SL_IID_ENGINE, &engineInterface)) {
            ACSDK_ERROR(LX("createRecorderFailed").d("reason", "failed to get engine interface"));
            m_created.clear();
            return nullptr;
        }

        return std::shared_ptr<AndroidSLESEngine>(new AndroidSLESEngine(std::move(engineObject), engineInterface));
    }

    ACSDK_ERROR(LX("createEngineFailed").d("reason", "singleton engine has been created already"));
    return nullptr;
}

AndroidSLESEngine::AndroidSLESEngine(std::unique_ptr<AndroidSLESObject> object, SLEngineItf engine) :
        m_object{std::move(object)},
        m_engine{engine} {
}

AndroidSLESEngine::~AndroidSLESEngine() {
    m_created.clear();
}

std::unique_ptr<AndroidSLESMicrophone> AndroidSLESEngine::createMicrophoneRecorder(
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream) {
    auto writer = stream->createWriter(avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE);
    if (!writer) {
        ACSDK_ERROR(LX("createAndroidMicFailed").d("reason", "failed to create writer"));
        return nullptr;
    }

    auto audioSink = AndroidSLESMicrophone::createSinkConfiguration();
    auto audioSource = AndroidSLESMicrophone::createSourceConfiguration();

    constexpr uint32_t interfaceSize = 1;
    const SLInterfaceID interfaceIDs[interfaceSize] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean requiredInterfaces[interfaceSize] = {SL_BOOLEAN_TRUE};

    SLObjectItf recorderObject;

    (*m_engine)->CreateAudioRecorder(
        m_engine, &recorderObject, &audioSource, &audioSink, interfaceSize, interfaceIDs, requiredInterfaces);
    if (!recorderObject) {
        ACSDK_ERROR(LX("initializeAndroidMicFailed").d("reason", "Failed to create recorder."));
        return nullptr;
    }

    std::shared_ptr<AndroidSLESObject> recorder = AndroidSLESObject::create(recorderObject);
    if (!recorder) {
        ACSDK_ERROR(LX("initializeAndroidMicFailed").d("reason", "Failed to create recorder wrapper."));
        return nullptr;
    }

    SLRecordItf recorderInterface;
    if (!recorder->getInterface(SL_IID_RECORD, &recorderInterface)) {
        ACSDK_ERROR(LX("initializeAndroidMicFailed").d("reason", "Failed to get recorder interface."));
        return nullptr;
    }

    auto queueObject = AndroidSLESBufferQueue::create(recorder, std::move(writer));
    if (!queueObject) {
        ACSDK_ERROR(LX("createRecorderFailed").d("reason", "Failed to create buffer queue."));
        return nullptr;
    }

    auto androidRecorder =
        make_unique<AndroidSLESMicrophone>(shared_from_this(), recorder, recorderInterface, std::move(queueObject));

    if (!androidRecorder->configureRecognizeMode()) {
        ACSDK_WARN(LX("Failed to set Recognize mode. This might affect the voice recognition."));
    }

    return androidRecorder;
}

std::unique_ptr<AndroidSLESObject> AndroidSLESEngine::createOutputMix() const {
    // Do not use any extra mix_id in order to use audio fast path.
    SLObjectItf outputMixObject;
    auto result = (*m_engine)->CreateOutputMix(m_engine, &outputMixObject, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("createPlayerFailed").d("reason", "Failed to create output mix.").d("result", result));
        return nullptr;
    }

    return AndroidSLESObject::create(outputMixObject);
}

std::unique_ptr<AndroidSLESObject> AndroidSLESEngine::createPlayer(
    SLDataSource& source,
    SLDataSink& sink,
    bool requireEqualizer) const {
    constexpr uint32_t interfaceSize = 4;
    const SLInterfaceID interfaceIds[interfaceSize] = {
        SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_PREFETCHSTATUS, SL_IID_EQUALIZER};
    const SLboolean requiredInterfaces[interfaceSize] = {
        SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_FALSE, requireEqualizer ? SL_BOOLEAN_TRUE : SL_BOOLEAN_FALSE};

    SLObjectItf playerObject;
    auto result = (*m_engine)->CreateAudioPlayer(
        m_engine, &playerObject, &source, &sink, interfaceSize, interfaceIds, requiredInterfaces);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("createFailed").d("reason", "createAudioPlayerFailed"));
        return nullptr;
    }

    return AndroidSLESObject::create(playerObject);
}

}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
