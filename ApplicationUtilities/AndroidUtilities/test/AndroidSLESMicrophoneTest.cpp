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

#include <array>
#include <gtest/gtest.h>

#include <AndroidUtilities/AndroidSLESEngine.h>
#include <AndroidUtilities/AndroidSLESMicrophone.h>
#include <AVSCommon/AVS/AudioInputStream.h>

using namespace ::testing;

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {
namespace test {

/// The sample rate of microphone audio data.
static constexpr size_t SAMPLE_RATE_HZ{16000};

/// The amount of audio data to keep in the ring buffer.
static constexpr std::chrono::seconds AMOUNT_OF_AUDIO_DATA_IN_BUFFER{10};

/// The size of the ring buffer.
static constexpr size_t SDS_BUFFER_SIZE = (SAMPLE_RATE_HZ)*AMOUNT_OF_AUDIO_DATA_IN_BUFFER.count();

/// Reader timeout.
static constexpr std::chrono::milliseconds TIMEOUT{1100};

/// The size of reader buffer is one page long.
static constexpr size_t TEST_BUFFER_SIZE{4096u};

/**
 * This class tests the entire @c AndroidSLESMicrophone together with the OpenSL ES NDK implementation.
 */
class AndroidSLESMicrophoneTest : public Test {
protected:
    /**
     * Reset all shared pointers.
     */
    virtual void TearDown() {
        m_reader.reset();
        m_stream.reset();
        m_mic.reset();
    }

    /**
     * Create android microphone.
     */
    virtual void SetUp() {
        auto buffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>(SDS_BUFFER_SIZE);
        m_stream = avsCommon::avs::AudioInputStream::create(buffer);
        auto engine = AndroidSLESEngine::create();
        m_mic = engine->createMicrophoneRecorder(m_stream);
        m_reader = m_stream->createReader(avsCommon::avs::AudioInputStream::Reader::Policy::BLOCKING);
        EXPECT_TRUE(m_mic);
    }

    /// Pointer to the audio input stream.
    std::shared_ptr<avsCommon::avs::AudioInputStream> m_stream;

    /// Object under test.
    std::unique_ptr<AndroidSLESMicrophone> m_mic;

    /// Pointer to an audio input stream reader used to check writting results.
    std::shared_ptr<avsCommon::avs::AudioInputStream::Reader> m_reader;

    /// Test buffer that will be used by the reader.
    std::array<uint16_t, TEST_BUFFER_SIZE> m_testBuffer;
};

/**
 * Test if recording works.
 */
TEST_F(AndroidSLESMicrophoneTest, TestStartRecording) {
    EXPECT_TRUE(m_mic->startStreamingMicrophoneData());
    auto read = m_reader->read(m_testBuffer.data(), TEST_BUFFER_SIZE, TIMEOUT);
    EXPECT_EQ(read, static_cast<decltype(read)>(TEST_BUFFER_SIZE));
}

/**
 * Test if the stopStreamingMicrophoneData will stop writing to the buffer.
 */
TEST_F(AndroidSLESMicrophoneTest, TestPauseRecording) {
    EXPECT_TRUE(m_mic->startStreamingMicrophoneData());
    EXPECT_TRUE(m_mic->stopStreamingMicrophoneData());
    auto read = m_reader->read(m_testBuffer.data(), TEST_BUFFER_SIZE, TIMEOUT);
    EXPECT_EQ(read, avsCommon::avs::AudioInputStream::Reader::Error::TIMEDOUT);
}

/**
 * Test if recording works after mute / unmute.
 */
TEST_F(AndroidSLESMicrophoneTest, TestUnPauseRecording) {
    EXPECT_TRUE(m_mic->startStreamingMicrophoneData());
    EXPECT_TRUE(m_mic->stopStreamingMicrophoneData());
    EXPECT_TRUE(m_mic->startStreamingMicrophoneData());
    auto read = m_reader->read(m_testBuffer.data(), TEST_BUFFER_SIZE, TIMEOUT);
    EXPECT_EQ(read, static_cast<decltype(read)>(TEST_BUFFER_SIZE));
}

/**
 * Test recording for a full iteration on the buffer circular queue.
 */
TEST_F(AndroidSLESMicrophoneTest, TestLongRecording) {
    EXPECT_TRUE(m_mic->startStreamingMicrophoneData());
    constexpr size_t iterations = AndroidSLESBufferQueue::NUMBER_OF_BUFFERS + 1;
    for (size_t i = 0; i < iterations; i++) {
        auto read = m_reader->read(m_testBuffer.data(), TEST_BUFFER_SIZE, TIMEOUT);
        EXPECT_EQ(read, static_cast<decltype(read)>(TEST_BUFFER_SIZE));
    }
}

}  // namespace test
}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
