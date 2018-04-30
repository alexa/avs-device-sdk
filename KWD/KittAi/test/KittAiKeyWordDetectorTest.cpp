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

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AVSCommon/SDKInterfaces/KeyWordDetectorStateObserverInterface.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/Utils/SDS/SharedDataStream.h>

#include "KittAi/KittAiKeyWordDetector.h"

namespace alexaClientSDK {
namespace kwd {
namespace test {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;

/// The path to the inputs folder that should be passed in via command line argument.
std::string inputsDirPath;

/// The name of the resource file required for Kitt.ai.
static const std::string RESOURCE_FILE = "/KittAiModels/common.res";

/// The name of the Alexa model file for Kitt.ai.
static const std::string MODEL_FILE = "/KittAiModels/alexa.umdl";

/// The keyword associated with alexa.umdl.
static const std::string MODEL_KEYWORD = "ALEXA";

/// The name of a test audio file.
static const std::string FOUR_ALEXAS_AUDIO_FILE = "/four_alexa.wav";

/// The name of a test audio file.
static const std::string ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE = "/alexa_stop_alexa_joke.wav";

/// The number of samples per millisecond, assuming a sample rate of 16 kHz.
static const int SAMPLES_PER_MS = 16;

/// The margin in milliseconds for testing indices of keyword detections.
static const std::chrono::milliseconds MARGIN = std::chrono::milliseconds(100);

/// The margin in samples for testing indices of keyword detections.
static const AudioInputStream::Index MARGIN_IN_SAMPLES = MARGIN.count() * SAMPLES_PER_MS;

/// The number of "Alexa" keywords in the four_alexa.wav file.
static const size_t NUM_ALEXAS_IN_FOUR_ALEXAS_AUDIO_FILE = 4;

/// The approximate end indices of the four "Alexa" hotwords in the four_alexa.wav file.
std::vector<AudioInputStream::Index> END_INDICES_OF_ALEXAS_IN_FOUR_ALEXAS_AUDIO_FILE = {21440, 52800, 72480, 91552};

/// The number of "Alexa" keywords in the alexa_stop_alexa_joke.wav file.
static const size_t NUM_ALEXAS_IN_ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE = 2;

/// The approximate end indices of the two "Alexa" hotwords in the alexa_stop_alexa_joke.wav file.
std::vector<AudioInputStream::Index> END_INDICES_OF_ALEXAS_IN_ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE = {20960, 51312};

/// The compatible encoding for Kitt.ai.
static const avsCommon::utils::AudioFormat::Encoding COMPATIBLE_ENCODING =
    avsCommon::utils::AudioFormat::Encoding::LPCM;

/// The compatible endianness for Kitt.ai.
static const avsCommon::utils::AudioFormat::Endianness COMPATIBLE_ENDIANNESS =
    avsCommon::utils::AudioFormat::Endianness::LITTLE;

/// The compatible sample rate for Kitt.ai.
static const unsigned int COMPATIBLE_SAMPLE_RATE = 16000;

/// The compatible bits per sample for Kitt.ai.
static const unsigned int COMPATIBLE_SAMPLE_SIZE_IN_BITS = 16;

/// The compatible number of channels for Kitt.ai
static const unsigned int COMPATIBLE_NUM_CHANNELS = 1;

/// Timeout for expected callbacks.
static const auto DEFAULT_TIMEOUT = std::chrono::milliseconds(4000);

/// The audio gain to apply to the detectors so that the expected detections occur.
static const double KITTAI_AUDIO_GAIN = 2.0;

/// Whether to tell Kitt.ai to apply front end processing. This is false since this only works on Raspberry Pi.
static const bool KITTAI_APPLY_FRONTEND_PROCESSING = false;

/**
 * The sensitivity to the keyword in the model. Set to 0.6 as this is what was described as optimal on the Kitt.ai
 * Github page.
 */
static const double KITTAI_SENSITIVITY = 0.6;

/// A test observer that mocks out the KeyWordObserverInterface##onKeyWordDetected() call.
class testKeyWordObserver : public KeyWordObserverInterface {
public:
    /// A struct used for bookkeeping of keyword detections.
    struct detectionResult {
        AudioInputStream::Index endIndex;
        std::string keyword;
    };

    /// Implementation of the KeyWordObserverInterface##onKeyWordDetected() call.
    void onKeyWordDetected(
        std::shared_ptr<AudioInputStream> stream,
        std::string keyword,
        AudioInputStream::Index beginIndex,
        AudioInputStream::Index endIndex,
        std::shared_ptr<const std::vector<char>> KWDMetadata) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_detectionResults.push_back({endIndex, keyword});
        m_detectionOccurred.notify_one();
    };

    /**
     * Waits for the KeyWordObserverInterface##onKeyWordDetected() call N times.
     *
     * @param numDetectionsExpected The number of detections expected.
     * @param timeout The amount of time to wait for the calls.
     * @return The detection results that actually occurred.
     */
    std::vector<detectionResult> waitForNDetections(
        unsigned int numDetectionsExpected,
        std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_detectionOccurred.wait_for(lock, timeout, [this, numDetectionsExpected]() {
            return m_detectionResults.size() == numDetectionsExpected;
        });
        return m_detectionResults;
    }

private:
    /// The detection results that have occurred.
    std::vector<detectionResult> m_detectionResults;

    /// A lock to guard against new detections.
    std::mutex m_mutex;

    /// A condition variable to wait for detection calls.
    std::condition_variable m_detectionOccurred;
};

/// A test observer that mocks out the KeyWordDetectorStateObserverInterface##onStateChanged() call.
class testStateObserver : public KeyWordDetectorStateObserverInterface {
public:
    /**
     * Constructor.
     */
    testStateObserver() :
            m_state(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::STREAM_CLOSED),
            m_stateChangeOccurred{false} {
    }

    /// Implementation of the KeyWordDetectorStateObserverInterface##onStateChanged() call.
    void onStateChanged(KeyWordDetectorStateObserverInterface::KeyWordDetectorState keyWordDetectorState) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_state = keyWordDetectorState;
        m_stateChangeOccurred = true;
        m_stateChanged.notify_one();
    }

    /**
     * Waits for the KeyWordDetectorStateObserverInterface##onStateChanged() call.
     *
     * @param timeout The amount of time to wait for the call.
     * @param stateChanged An output parameter that notifies the caller whether a call occurred.
     * @return Returns the state of the observer.
     */
    KeyWordDetectorStateObserverInterface::KeyWordDetectorState waitForStateChange(
        std::chrono::milliseconds timeout,
        bool* stateChanged) {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool success = m_stateChanged.wait_for(lock, timeout, [this]() { return m_stateChangeOccurred; });

        if (!success) {
            *stateChanged = false;
        } else {
            m_stateChangeOccurred = false;
            *stateChanged = true;
        }
        return m_state;
    }

private:
    /// The state of the observer.
    KeyWordDetectorStateObserverInterface::KeyWordDetectorState m_state;

    /// A boolean flag so that we can re-use the observer even after a callback has occurred.
    bool m_stateChangeOccurred;

    /// A lock to guard against state changes.
    std::mutex m_mutex;

    /// A condition variable to wait for state changes.
    std::condition_variable m_stateChanged;
};

class KittAiKeyWordTest : public ::testing::Test {
protected:
    std::vector<int16_t> readAudioFromFile(const std::string& fileName, bool* errorOccurred) {
        const int RIFF_HEADER_SIZE = 44;

        std::ifstream inputFile(fileName.c_str(), std::ifstream::binary);
        if (!inputFile.good()) {
            std::cout << "Couldn't open audio file!" << std::endl;
            if (errorOccurred) {
                *errorOccurred = true;
            }
            return {};
        }
        inputFile.seekg(0, std::ios::end);
        int fileLengthInBytes = inputFile.tellg();
        if (fileLengthInBytes <= RIFF_HEADER_SIZE) {
            std::cout << "File should be larger than 44 bytes, which is the size of the RIFF header" << std::endl;
            if (errorOccurred) {
                *errorOccurred = true;
            }
            return {};
        }

        inputFile.seekg(RIFF_HEADER_SIZE, std::ios::beg);

        int numSamples = (fileLengthInBytes - RIFF_HEADER_SIZE) / 2;

        std::vector<int16_t> retVal(numSamples, 0);

        inputFile.read((char*)&retVal[0], numSamples * 2);

        if (inputFile.gcount() != numSamples * 2) {
            std::cout << "Error reading audio file" << std::endl;
            if (errorOccurred) {
                *errorOccurred = true;
            }
            return {};
        }

        inputFile.close();
        if (errorOccurred) {
            *errorOccurred = false;
        }
        return retVal;
    }

    bool isResultPresent(
        std::vector<testKeyWordObserver::detectionResult>& results,
        AudioInputStream::Index expectedEndIndex,
        const std::string& expectedKeyword) {
        AudioInputStream::Index highBound = expectedEndIndex + MARGIN_IN_SAMPLES;
        AudioInputStream::Index lowBound = expectedEndIndex - MARGIN_IN_SAMPLES;
        for (auto result : results) {
            if (result.endIndex <= highBound && result.endIndex >= lowBound && expectedKeyword == result.keyword) {
                return true;
            }
        }
        return false;
    }

    std::shared_ptr<testKeyWordObserver> keyWordObserver1;

    std::shared_ptr<testKeyWordObserver> keyWordObserver2;

    std::shared_ptr<testStateObserver> stateObserver;

    AudioFormat compatibleAudioFormat;

    KittAiKeyWordDetector::KittAiConfiguration config;

    virtual void SetUp() {
        keyWordObserver1 = std::make_shared<testKeyWordObserver>();
        keyWordObserver2 = std::make_shared<testKeyWordObserver>();
        stateObserver = std::make_shared<testStateObserver>();

        compatibleAudioFormat.sampleRateHz = COMPATIBLE_SAMPLE_RATE;
        compatibleAudioFormat.sampleSizeInBits = COMPATIBLE_SAMPLE_SIZE_IN_BITS;
        compatibleAudioFormat.numChannels = COMPATIBLE_NUM_CHANNELS;
        compatibleAudioFormat.endianness = COMPATIBLE_ENDIANNESS;
        compatibleAudioFormat.encoding = COMPATIBLE_ENCODING;

        std::ifstream filePresent((inputsDirPath + MODEL_FILE).c_str());
        ASSERT_TRUE(filePresent.good()) << "Unable to find " + inputsDirPath + MODEL_FILE
                                        << ". Please place model file within this location.";

        std::ifstream filePresent2((inputsDirPath + RESOURCE_FILE).c_str());
        ASSERT_TRUE(filePresent2.good()) << "Unable to find " + inputsDirPath + RESOURCE_FILE
                                         << ". Please place model file within this location.";

        config = {inputsDirPath + MODEL_FILE, MODEL_KEYWORD, KITTAI_SENSITIVITY};
    }
};

/// Tests that we don't get back a valid detector if an invalid stream is passed in.
TEST_F(KittAiKeyWordTest, invalidStream) {
    auto detector = KittAiKeyWordDetector::create(
        nullptr,
        compatibleAudioFormat,
        {keyWordObserver1},
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>>(),
        inputsDirPath + RESOURCE_FILE,
        {config},
        KITTAI_AUDIO_GAIN,
        KITTAI_APPLY_FRONTEND_PROCESSING);
    ASSERT_FALSE(detector);
}

/// Tests that we don't get back a valid detector if an invalid endianness is passed in.
TEST_F(KittAiKeyWordTest, incompatibleEndianness) {
    auto rawBuffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>(500000);
    auto uniqueSds = avsCommon::avs::AudioInputStream::create(rawBuffer, 2, 1);
    std::shared_ptr<AudioInputStream> sds = std::move(uniqueSds);

    compatibleAudioFormat.endianness = AudioFormat::Endianness::BIG;

    auto detector = KittAiKeyWordDetector::create(
        sds,
        compatibleAudioFormat,
        {keyWordObserver1},
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>>(),
        inputsDirPath + RESOURCE_FILE,
        {config},
        1.0,
        false);
    ASSERT_FALSE(detector);
}

/// Tests that we get back the expected number of keywords for the four_alexa.wav file for one keyword observer.
TEST_F(KittAiKeyWordTest, getExpectedNumberOfDetectionsInFourAlexasAudioFileForOneObserver) {
    auto fourAlexasBuffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>(500000);
    auto fourAlexasSds = avsCommon::avs::AudioInputStream::create(fourAlexasBuffer, 2, 1);
    std::shared_ptr<AudioInputStream> fourAlexasAudioBuffer = std::move(fourAlexasSds);

    std::unique_ptr<AudioInputStream::Writer> fourAlexasAudioBufferWriter =
        fourAlexasAudioBuffer->createWriter(avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE);

    std::string audioFilePath = inputsDirPath + FOUR_ALEXAS_AUDIO_FILE;
    bool error;
    std::vector<int16_t> audioData = readAudioFromFile(audioFilePath, &error);
    ASSERT_FALSE(error);

    fourAlexasAudioBufferWriter->write(audioData.data(), audioData.size());

    auto detector = KittAiKeyWordDetector::create(
        fourAlexasAudioBuffer,
        compatibleAudioFormat,
        {keyWordObserver1},
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>>(),
        inputsDirPath + RESOURCE_FILE,
        {config},
        KITTAI_AUDIO_GAIN,
        KITTAI_APPLY_FRONTEND_PROCESSING);
    ASSERT_TRUE(detector);
    auto detections =
        keyWordObserver1->waitForNDetections(END_INDICES_OF_ALEXAS_IN_FOUR_ALEXAS_AUDIO_FILE.size(), DEFAULT_TIMEOUT);
    ASSERT_EQ(detections.size(), NUM_ALEXAS_IN_FOUR_ALEXAS_AUDIO_FILE);

    for (auto index : END_INDICES_OF_ALEXAS_IN_FOUR_ALEXAS_AUDIO_FILE) {
        ASSERT_TRUE(isResultPresent(detections, index, MODEL_KEYWORD));
    }
}

/// Tests that we get back the expected number of keywords for the four_alexa.wav file for two keyword observers.
TEST_F(KittAiKeyWordTest, getExpectedNumberOfDetectionsInFourAlexasAudioFileForTwoObservers) {
    auto fourAlexasBuffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>(500000);
    auto fourAlexasSds = avsCommon::avs::AudioInputStream::create(fourAlexasBuffer, 2, 1);
    std::shared_ptr<AudioInputStream> fourAlexasAudioBuffer = std::move(fourAlexasSds);

    std::unique_ptr<AudioInputStream::Writer> fourAlexasAudioBufferWriter =
        fourAlexasAudioBuffer->createWriter(avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE);

    std::string audioFilePath = inputsDirPath + FOUR_ALEXAS_AUDIO_FILE;
    bool error;
    std::vector<int16_t> audioData = readAudioFromFile(audioFilePath, &error);
    ASSERT_FALSE(error);

    fourAlexasAudioBufferWriter->write(audioData.data(), audioData.size());

    auto detector = KittAiKeyWordDetector::create(
        fourAlexasAudioBuffer,
        compatibleAudioFormat,
        {keyWordObserver1, keyWordObserver2},
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>>(),
        inputsDirPath + RESOURCE_FILE,
        {config},
        KITTAI_AUDIO_GAIN,
        KITTAI_APPLY_FRONTEND_PROCESSING);
    ASSERT_TRUE(detector);
    auto detections = keyWordObserver1->waitForNDetections(NUM_ALEXAS_IN_FOUR_ALEXAS_AUDIO_FILE, DEFAULT_TIMEOUT);
    ASSERT_EQ(detections.size(), NUM_ALEXAS_IN_FOUR_ALEXAS_AUDIO_FILE);

    for (auto index : END_INDICES_OF_ALEXAS_IN_FOUR_ALEXAS_AUDIO_FILE) {
        ASSERT_TRUE(isResultPresent(detections, index, MODEL_KEYWORD));
    }

    detections = keyWordObserver2->waitForNDetections(NUM_ALEXAS_IN_FOUR_ALEXAS_AUDIO_FILE, DEFAULT_TIMEOUT);
    ASSERT_EQ(detections.size(), NUM_ALEXAS_IN_FOUR_ALEXAS_AUDIO_FILE);

    for (auto index : END_INDICES_OF_ALEXAS_IN_FOUR_ALEXAS_AUDIO_FILE) {
        ASSERT_TRUE(isResultPresent(detections, index, MODEL_KEYWORD));
    }
}

/**
 * Tests that we get back the expected number of keywords for the alexa_stop_alexa_joke.wav file for one keyword
 * observer.
 */
TEST_F(KittAiKeyWordTest, getExpectedNumberOfDetectionsInAlexaStopAlexaJokeAudioFileForOneObserver) {
    auto alexaStopAlexaJokeBuffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>(500000);
    auto alexaStopAlexaJokeSds = avsCommon::avs::AudioInputStream::create(alexaStopAlexaJokeBuffer, 2, 1);
    std::shared_ptr<AudioInputStream> alexaStopAlexaJokeAudioBuffer = std::move(alexaStopAlexaJokeSds);

    std::unique_ptr<AudioInputStream::Writer> alexaStopAlexaJokeAudioBufferWriter =
        alexaStopAlexaJokeAudioBuffer->createWriter(avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE);

    std::string audioFilePath = inputsDirPath + ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE;
    bool error;
    std::vector<int16_t> audioData = readAudioFromFile(audioFilePath, &error);
    ASSERT_FALSE(error);

    alexaStopAlexaJokeAudioBufferWriter->write(audioData.data(), audioData.size());

    auto detector = KittAiKeyWordDetector::create(
        alexaStopAlexaJokeAudioBuffer,
        compatibleAudioFormat,
        {keyWordObserver1},
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>>(),
        inputsDirPath + RESOURCE_FILE,
        {config},
        KITTAI_AUDIO_GAIN,
        KITTAI_APPLY_FRONTEND_PROCESSING);
    ASSERT_TRUE(detector);
    auto detections =
        keyWordObserver1->waitForNDetections(NUM_ALEXAS_IN_ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE, DEFAULT_TIMEOUT);
    ASSERT_EQ(detections.size(), NUM_ALEXAS_IN_ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE);

    for (auto index : END_INDICES_OF_ALEXAS_IN_ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE) {
        ASSERT_TRUE(isResultPresent(detections, index, MODEL_KEYWORD));
    }
}

/**
 * Tests that we get back the expected number of keywords for the alexa_stop_alexa_joke.wav file for two keyword
 * observer.
 */
TEST_F(KittAiKeyWordTest, getExpectedNumberOfDetectionsInAlexaStopAlexaJokeAudioFileForTwoObservers) {
    auto alexaStopAlexaJokeBuffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>(500000);
    auto alexaStopAlexaJokeSds = avsCommon::avs::AudioInputStream::create(alexaStopAlexaJokeBuffer, 2, 1);
    std::shared_ptr<AudioInputStream> alexaStopAlexaJokeAudioBuffer = std::move(alexaStopAlexaJokeSds);

    std::unique_ptr<AudioInputStream::Writer> alexaStopAlexaJokeAudioBufferWriter =
        alexaStopAlexaJokeAudioBuffer->createWriter(avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE);

    std::string audioFilePath = inputsDirPath + ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE;
    bool error;
    std::vector<int16_t> audioData = readAudioFromFile(audioFilePath, &error);
    ASSERT_FALSE(error);

    alexaStopAlexaJokeAudioBufferWriter->write(audioData.data(), audioData.size());

    auto detector = KittAiKeyWordDetector::create(
        alexaStopAlexaJokeAudioBuffer,
        compatibleAudioFormat,
        {keyWordObserver1, keyWordObserver2},
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>>(),
        inputsDirPath + RESOURCE_FILE,
        {config},
        KITTAI_AUDIO_GAIN,
        KITTAI_APPLY_FRONTEND_PROCESSING);
    ASSERT_TRUE(detector);
    auto detections =
        keyWordObserver1->waitForNDetections(NUM_ALEXAS_IN_ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE, DEFAULT_TIMEOUT);
    ASSERT_EQ(detections.size(), NUM_ALEXAS_IN_ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE);

    for (auto index : END_INDICES_OF_ALEXAS_IN_ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE) {
        ASSERT_TRUE(isResultPresent(detections, index, MODEL_KEYWORD));
    }

    detections = keyWordObserver2->waitForNDetections(NUM_ALEXAS_IN_ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE, DEFAULT_TIMEOUT);
    ASSERT_EQ(detections.size(), NUM_ALEXAS_IN_ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE);

    for (auto index : END_INDICES_OF_ALEXAS_IN_ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE) {
        ASSERT_TRUE(isResultPresent(detections, index, MODEL_KEYWORD));
    }
}

/// Tests that the detector state changes to ACTIVE when the detector is initialized properly.
TEST_F(KittAiKeyWordTest, getActiveState) {
    auto alexaStopAlexaJokeBuffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>(500000);
    auto alexaStopAlexaJokeSds = avsCommon::avs::AudioInputStream::create(alexaStopAlexaJokeBuffer, 2, 1);
    std::shared_ptr<AudioInputStream> alexaStopAlexaJokeAudioBuffer = std::move(alexaStopAlexaJokeSds);

    std::unique_ptr<AudioInputStream::Writer> alexaStopAlexaJokeAudioBufferWriter =
        alexaStopAlexaJokeAudioBuffer->createWriter(avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE);

    std::string audioFilePath = inputsDirPath + ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE;
    bool error;
    std::vector<int16_t> audioData = readAudioFromFile(audioFilePath, &error);
    ASSERT_FALSE(error);

    alexaStopAlexaJokeAudioBufferWriter->write(audioData.data(), audioData.size());

    auto detector = KittAiKeyWordDetector::create(
        alexaStopAlexaJokeAudioBuffer,
        compatibleAudioFormat,
        std::unordered_set<std::shared_ptr<KeyWordObserverInterface>>(),
        {stateObserver},
        inputsDirPath + RESOURCE_FILE,
        {config},
        KITTAI_AUDIO_GAIN,
        false);
    ASSERT_TRUE(detector);
    bool stateChanged = false;
    KeyWordDetectorStateObserverInterface::KeyWordDetectorState stateReceived =
        stateObserver->waitForStateChange(DEFAULT_TIMEOUT, &stateChanged);
    ASSERT_TRUE(stateChanged);
    ASSERT_EQ(stateReceived, KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);
}

/**
 * Tests that the stream is closed and that the detector state changes to STREAM_CLOSED when we close the only writer
 * of the SDS passed in and all keyword detections have occurred.
 */
TEST_F(KittAiKeyWordTest, getStreamClosedState) {
    auto alexaStopAlexaJokeBuffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>(500000);
    auto alexaStopAlexaJokeSds = avsCommon::avs::AudioInputStream::create(alexaStopAlexaJokeBuffer, 2, 1);
    std::shared_ptr<AudioInputStream> alexaStopAlexaJokeAudioBuffer = std::move(alexaStopAlexaJokeSds);

    std::unique_ptr<AudioInputStream::Writer> alexaStopAlexaJokeAudioBufferWriter =
        alexaStopAlexaJokeAudioBuffer->createWriter(avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE);

    std::string audioFilePath = inputsDirPath + ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE;
    bool error;
    std::vector<int16_t> audioData = readAudioFromFile(audioFilePath, &error);
    ASSERT_FALSE(error);

    alexaStopAlexaJokeAudioBufferWriter->write(audioData.data(), audioData.size());

    auto detector = KittAiKeyWordDetector::create(
        alexaStopAlexaJokeAudioBuffer,
        compatibleAudioFormat,
        {keyWordObserver1},
        {stateObserver},
        inputsDirPath + RESOURCE_FILE,
        {config},
        KITTAI_AUDIO_GAIN,
        false);
    ASSERT_TRUE(detector);

    // so that when we close the writer, we know for sure that the reader will be closed
    auto detections =
        keyWordObserver1->waitForNDetections(NUM_ALEXAS_IN_ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE, DEFAULT_TIMEOUT);
    ASSERT_EQ(detections.size(), NUM_ALEXAS_IN_ALEXA_STOP_ALEXA_JOKE_AUDIO_FILE);

    bool stateChanged = false;
    KeyWordDetectorStateObserverInterface::KeyWordDetectorState stateReceived =
        stateObserver->waitForStateChange(DEFAULT_TIMEOUT, &stateChanged);
    ASSERT_TRUE(stateChanged);
    ASSERT_EQ(stateReceived, KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);

    alexaStopAlexaJokeAudioBufferWriter->close();
    stateChanged = false;
    stateReceived = stateObserver->waitForStateChange(DEFAULT_TIMEOUT, &stateChanged);
    ASSERT_TRUE(stateChanged);
    ASSERT_EQ(stateReceived, KeyWordDetectorStateObserverInterface::KeyWordDetectorState::STREAM_CLOSED);
}

}  // namespace test
}  // namespace kwd
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 2) {
        std::cerr << "USAGE: " << std::string(argv[0]) << " <path_to_inputs_folder>" << std::endl;
        return 1;
    } else {
        alexaClientSDK::kwd::test::inputsDirPath = std::string(argv[1]);
        return RUN_ALL_TESTS();
    }
}
