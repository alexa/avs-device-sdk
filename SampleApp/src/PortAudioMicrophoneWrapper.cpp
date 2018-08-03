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
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <future>
#include <chrono>

#include <rapidjson/document.h>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include "SampleApp/PortAudioMicrophoneWrapper.h"
#include "SampleApp/ConsolePrinter.h"

namespace alexaClientSDK {
namespace sampleApp {

using avsCommon::avs::AudioInputStream;

static const int NUM_INPUT_CHANNELS = 1;
static const int NUM_OUTPUT_CHANNELS = 0;
static const double SAMPLE_RATE = 16000;
static const unsigned long PREFERRED_SAMPLES_PER_CALLBACK = paFramesPerBufferUnspecified;

static const std::string SAMPLE_APP_CONFIG_ROOT_KEY("sampleApp");
static const std::string PORTAUDIO_CONFIG_ROOT_KEY("portAudio");
static const std::string PORTAUDIO_CONFIG_SUGGESTED_LATENCY_KEY("suggestedLatency");

/// String to identify log entries originating from this file.
static const std::string TAG("PortAudioMicrophoneWrapper");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<PortAudioMicrophoneWrapper> PortAudioMicrophoneWrapper::create(
    std::shared_ptr<AudioInputStream> stream) {
    if (!stream) {
        ACSDK_CRITICAL(LX("Invalid stream passed to PortAudioMicrophoneWrapper"));
        return nullptr;
    }
    std::unique_ptr<PortAudioMicrophoneWrapper> portAudioMicrophoneWrapper(new PortAudioMicrophoneWrapper(stream));
    if (!portAudioMicrophoneWrapper->initialize()) {
        ACSDK_CRITICAL(LX("Failed to initialize PortAudioMicrophoneWrapper"));
        return nullptr;
    }
    return portAudioMicrophoneWrapper;
}

PortAudioMicrophoneWrapper::PortAudioMicrophoneWrapper(std::shared_ptr<AudioInputStream> stream) :
        m_audioInputStream{stream},
        m_paStream{nullptr},
        m_readerThread{nullptr},
        m_fileStream{nullptr},
        m_threadPromise{nullptr},
        m_threadFuture{nullptr},
        m_samplesRead{0},
        m_eofReached{false} {
}

PortAudioMicrophoneWrapper::~PortAudioMicrophoneWrapper() {
    Pa_StopStream(m_paStream);
    Pa_CloseStream(m_paStream);
    Pa_Terminate();
    if (m_readerThread) {
        m_threadPromise->set_value();
        m_readerThread->join();
        delete m_readerThread;
        delete m_threadFuture;
        delete m_threadPromise;
        m_readerThread = nullptr;
        m_threadFuture = nullptr;
        m_threadPromise = nullptr;
        ACSDK_LOG(alexaClientSDK::avsCommon::utils::logger::Level::INFO, alexaClientSDK::avsCommon::utils::logger::LogEntry("FileInput", "threadDestroyed"));
    }
    if (m_fileStream) {
        delete m_fileStream;
        m_fileStream = nullptr;
        ACSDK_LOG(alexaClientSDK::avsCommon::utils::logger::Level::INFO, alexaClientSDK::avsCommon::utils::logger::LogEntry("FileInput", "fileClosed"));
    }
}

bool PortAudioMicrophoneWrapper::initialize() {
    m_writer = m_audioInputStream->createWriter(AudioInputStream::Writer::Policy::NONBLOCKABLE);
    if (!m_writer) {
        ACSDK_CRITICAL(LX("Failed to create stream writer"));
        return false;
    }

    m_fileStream = new std::ifstream{"/tmp/in.raw", std::ios::binary};
    if (!m_fileStream->is_open()) {
        ACSDK_CRITICAL(LX("Failed to open input audio file"));
        return false;
    }
    ACSDK_LOG(alexaClientSDK::avsCommon::utils::logger::Level::INFO, alexaClientSDK::avsCommon::utils::logger::LogEntry("FileInput", "fileOpen"));
    return true;

    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) {
        ACSDK_CRITICAL(LX("Failed to initialize PortAudio"));
        return false;
    }

    PaTime suggestedLatency;
    bool latencyInConfig = getConfigSuggestedLatency(suggestedLatency);

    if (!latencyInConfig) {
        err = Pa_OpenDefaultStream(
            &m_paStream,
            NUM_INPUT_CHANNELS,
            NUM_OUTPUT_CHANNELS,
            paInt16,
            SAMPLE_RATE,
            PREFERRED_SAMPLES_PER_CALLBACK,
            PortAudioCallback,
            this);
    } else {
        ACSDK_INFO(
            LX("PortAudio suggestedLatency has been configured to ").d("Seconds", std::to_string(suggestedLatency)));

        PaStreamParameters inputParameters;
        std::memset(&inputParameters, 0, sizeof(inputParameters));
        inputParameters.device = Pa_GetDefaultInputDevice();
        inputParameters.channelCount = NUM_INPUT_CHANNELS;
        inputParameters.sampleFormat = paInt16;
        inputParameters.suggestedLatency = suggestedLatency;
        inputParameters.hostApiSpecificStreamInfo = nullptr;

        err = Pa_OpenStream(
            &m_paStream,
            &inputParameters,
            nullptr,
            SAMPLE_RATE,
            PREFERRED_SAMPLES_PER_CALLBACK,
            paNoFlag,
            PortAudioCallback,
            this);
    }

    if (err != paNoError) {
        ACSDK_CRITICAL(LX("Failed to open PortAudio default stream"));
        return false;
    }
    return true;
}

void PortAudioMicrophoneWrapper::ReaderThread(PortAudioMicrophoneWrapper *wrapper)
{
    ACSDK_LOG(alexaClientSDK::avsCommon::utils::logger::Level::INFO, alexaClientSDK::avsCommon::utils::logger::LogEntry("FileInput", "threadAlive"));
    signed short block[128];
    const unsigned period = sizeof(block) / 2 * 1000 / (unsigned)SAMPLE_RATE;
    while (wrapper->m_threadFuture->wait_for(std::chrono::milliseconds(period)) == std::future_status::timeout) {
        wrapper->m_fileStream->read((char*)block, sizeof(block));
        if (wrapper->m_fileStream->fail() || wrapper->m_fileStream->eof()) {
            if (!wrapper->m_eofReached) {
                ACSDK_LOG(alexaClientSDK::avsCommon::utils::logger::Level::INFO, alexaClientSDK::avsCommon::utils::logger::LogEntry("FileInput", "eofReached"));
                wrapper->m_eofReached = true;
            }
            std::memset(block, 0, sizeof(block));
        }
        ssize_t returnCode = wrapper->m_writer->write(block, sizeof(block) / 2);
        if (returnCode <= 0) {
            ACSDK_CRITICAL(LX("Failed to write to stream."));
        }
        if (wrapper->m_samplesRead % (unsigned)SAMPLE_RATE == 0) {
            ACSDK_LOG(alexaClientSDK::avsCommon::utils::logger::Level::INFO, alexaClientSDK::avsCommon::utils::logger::LogEntry("FileInput", "timeElapsedApprox").d("seconds", wrapper->m_samplesRead / (unsigned)SAMPLE_RATE));
        }
        wrapper->m_samplesRead += sizeof(block) / 2;
    }
    ACSDK_LOG(alexaClientSDK::avsCommon::utils::logger::Level::INFO, alexaClientSDK::avsCommon::utils::logger::LogEntry("FileInput", "threadShuttingDown"));
}

bool PortAudioMicrophoneWrapper::startStreamingMicrophoneData() {
    m_threadPromise = new std::promise<void>;
    m_threadFuture = new std::future<void>{m_threadPromise->get_future()};
    m_readerThread = new std::thread{ReaderThread, this};
    m_samplesRead = 0;
    m_eofReached = false;
    ACSDK_LOG(alexaClientSDK::avsCommon::utils::logger::Level::INFO, alexaClientSDK::avsCommon::utils::logger::LogEntry("FileInput", "threadCreated"));
    return true;
    std::lock_guard<std::mutex> lock{m_mutex};
    PaError err = Pa_StartStream(m_paStream);
    if (err != paNoError) {
        ACSDK_CRITICAL(LX("Failed to start PortAudio stream"));
        return false;
    }
    return true;
}

bool PortAudioMicrophoneWrapper::stopStreamingMicrophoneData() {
    return true;
    std::lock_guard<std::mutex> lock{m_mutex};
    PaError err = Pa_StopStream(m_paStream);
    if (err != paNoError) {
        ACSDK_CRITICAL(LX("Failed to stop PortAudio stream"));
        return false;
    }
    return true;
}

int PortAudioMicrophoneWrapper::PortAudioCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long numSamples,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData) {
    PortAudioMicrophoneWrapper* wrapper = static_cast<PortAudioMicrophoneWrapper*>(userData);
    ssize_t returnCode = wrapper->m_writer->write(inputBuffer, numSamples);
    if (returnCode <= 0) {
        ACSDK_CRITICAL(LX("Failed to write to stream."));
        return paAbort;
    }
    return paContinue;
}

bool PortAudioMicrophoneWrapper::getConfigSuggestedLatency(PaTime& suggestedLatency) {
    bool latencyInConfig = false;
    auto config = avsCommon::utils::configuration::ConfigurationNode::getRoot()[SAMPLE_APP_CONFIG_ROOT_KEY]
                                                                               [PORTAUDIO_CONFIG_ROOT_KEY];
    if (config) {
        latencyInConfig = config.getValue(
            PORTAUDIO_CONFIG_SUGGESTED_LATENCY_KEY,
            &suggestedLatency,
            suggestedLatency,
            &rapidjson::Value::IsDouble,
            &rapidjson::Value::GetDouble);
    }

    return latencyInConfig;
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
