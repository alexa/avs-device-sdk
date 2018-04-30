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
#include <AIP/AudioProvider.h>
#include <AIP/ESPData.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include "ESP/ESPDataProvider.h"

/// String to identify log entries originating from this file.
static const std::string TAG{"ESPDataProvider"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The ESP compatible AVS sample rate of 16 kHz.
static const unsigned int ESP_COMPATIBLE_SAMPLE_RATE = 16000;

/// The ESP compatible bits per sample of 16.
static const unsigned int ESP_COMPATIBLE_SAMPLE_SIZE_IN_BITS = 16;

/// The ESP frame size in ms.  The ESP library supports 8ms, 15ms and 16ms
static const unsigned int ESP_FRAMES_IN_MILLISECONDS = 16;

namespace alexaClientSDK {
namespace esp {

static const auto TIMEOUT = std::chrono::seconds(1);

using AudioInputStream = avsCommon::avs::AudioInputStream;
using ESPData = alexaClientSDK::capabilityAgents::aip::ESPData;

std::unique_ptr<ESPDataProvider> ESPDataProvider::create(const capabilityAgents::aip::AudioProvider& audioProvider) {
    if ((ESP_COMPATIBLE_SAMPLE_RATE != audioProvider.format.sampleRateHz) ||
        (ESP_COMPATIBLE_SAMPLE_SIZE_IN_BITS != audioProvider.format.sampleSizeInBits)) {
        ACSDK_ERROR(LX(__func__)
                        .d("reason", "unsupportedFormat")
                        .d("sampleSize", audioProvider.format.sampleSizeInBits)
                        .d("sampleRateHz", audioProvider.format.sampleRateHz));
        return nullptr;
    }

    unsigned int frameSize = (audioProvider.format.sampleRateHz / 1000) * ESP_FRAMES_IN_MILLISECONDS;

    auto reader = audioProvider.stream->createReader(avsCommon::avs::AudioInputStream::Reader::Policy::BLOCKING);
    if (!reader) {
        ACSDK_ERROR(LX(__func__).d("reason", "createReaderFailed"));
        return nullptr;
    }

    auto connector = std::unique_ptr<ESPDataProvider>(new ESPDataProvider(std::move(reader), frameSize));
    connector->enable();
    return connector;
}

ESPDataProvider::~ESPDataProvider() {
    m_isShuttingDown = true;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

ESPData ESPDataProvider::getESPData() {
    std::lock_guard<std::mutex> lock{m_mutex};
    if (m_isEnabled) {
        return ESPData{std::to_string(m_frameEnergyCompute.getVoicedEnergy()),
                       std::to_string(m_frameEnergyCompute.getAmbientEnergy())};
    }
    return ESPData::EMPTY_ESP_DATA;
}

bool ESPDataProvider::isEnabled() const {
    return m_isEnabled;
}

void ESPDataProvider::disable() {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_isEnabled = false;
}

void ESPDataProvider::enable() {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_isEnabled = true;
}

void ESPDataProvider::espLoop() {
    Word64 currentFrameEnergy = 0;
    short procBuff[m_frameSize];
    const auto numWords = sizeof(procBuff) / m_reader->getWordSize();
    bool GVAD = false;
    bool hasErrorOccurred = false;

    while (!m_isShuttingDown) {
        auto words = m_reader->read(&procBuff, numWords, TIMEOUT);

        if (words > 0) {
            // Call VAD and get frame energy
            m_vad.process(procBuff, GVAD, currentFrameEnergy);
            {
                std::lock_guard<std::mutex> lock{m_mutex};
                m_frameEnergyCompute.process(GVAD, currentFrameEnergy);
            }
        } else {
            switch (words) {
                case AudioInputStream::Reader::Error::CLOSED:
                    ACSDK_CRITICAL(LX("espLoopFailed").d("reason", "streamClosed"));
                    hasErrorOccurred = true;
                    break;
                case AudioInputStream::Reader::Error::OVERRUN:
                    ACSDK_ERROR(LX("espLoopFailed").d("reason", "streamOverrun"));
                    m_reader->seek(0, AudioInputStream::Reader::Reference::BEFORE_WRITER);
                    break;
                case AudioInputStream::Reader::Error::TIMEDOUT:
                    ACSDK_INFO(LX("espLoopFailed").d("reason", "readerTimeOut"));
                    break;
                default:
                    // We should never get this since we are using a Blocking Reader.
                    ACSDK_CRITICAL(LX("espLoopFailed")
                                       .d("reason", "unexpectedError")
                                       // Leave as ssize_t to avoid messiness of casting to enum.
                                       .d("error", words));
                    hasErrorOccurred = true;
                    break;
            }
        }
        if (hasErrorOccurred) {
            ACSDK_CRITICAL(LX("espLoop").m("An error has occurred, exiting loop."));
            break;
        }
    }
    m_reader->close();
}

ESPDataProvider::ESPDataProvider(
    std::unique_ptr<avsCommon::avs::AudioInputStream::Reader> reader,
    unsigned int frameSize) :
        m_reader{std::move(reader)},
        m_vad{frameSize},
        m_frameEnergyCompute{frameSize},
        m_isEnabled{true},
        m_isShuttingDown{false},
        m_frameSize{frameSize} {
    m_vad.blkReset();
    m_frameEnergyCompute.blkReset();
    m_thread = std::thread(&ESPDataProvider::espLoop, this);
}

}  // namespace esp
}  // namespace alexaClientSDK
