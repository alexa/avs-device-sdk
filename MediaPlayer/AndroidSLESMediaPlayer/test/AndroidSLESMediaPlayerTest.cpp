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
#include <algorithm>
#include <fstream>
#include <memory>
#include <thread>
#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/AVS/Attachment/AttachmentWriter.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachment.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AndroidSLESMediaPlayer/AndroidSLESMediaPlayer.h>
#include <AndroidSLESMediaPlayer/FFmpegDecoder.h>
#include <AndroidUtilities/AndroidLogger.h>
#include <AndroidUtilities/AndroidSLESEngine.h>
#include <Audio/Data/med_alerts_notification_01._TTH_.mp3.h>
#include <Audio/Data/med_system_alerts_melodic_01_short._TTH_.wav.h>

/// String to identify log entries originating from this file.
static const std::string TAG("AndroidSLESMediaPlayerTest");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::mediaPlayer;
using namespace applicationUtilities::androidUtilities;

/// The size of the input buffer.
static constexpr size_t MP3_INPUT_SIZE =
    applicationUtilities::resources::audio::data::med_alerts_notification_01__TTH__mp3_len;

/// An input buffer with an mp3 file.
static const auto MP3_INPUT_CSTR = applicationUtilities::resources::audio::data::med_alerts_notification_01__TTH__mp3;

/// The mp3 duration in milliseconds.
static const std::chrono::milliseconds MP3_INPUT_DURATION{1440};

/// The size of the input buffer.
static const size_t RAW_INPUT_SIZE =
    applicationUtilities::resources::audio::data::med_system_alerts_melodic_01_short__TTH__wav_len;

/// An input buffer with an mp3 file.
static const auto RAW_INPUT_CSTR =
    applicationUtilities::resources::audio::data::med_system_alerts_melodic_01_short__TTH__wav;

/// This class mocks an attachment reader that reads from the @c INPUT_CSTR.
class MockAttachmentReader : public AttachmentReader {
public:
    MOCK_METHOD1(seek, bool(uint64_t offset));
    MOCK_CONST_METHOD0(tell, uint64_t());
    MOCK_METHOD0(getNumUnreadBytes, uint64_t());
    MOCK_METHOD1(close, void(ClosePoint closePoint));

    /**
     * Method that simulates files reading.
     *
     * @param buf The buffer to read into.
     * @param numBytes The number of bytes to read.
     * @param readStatus[out] The status from the last read.
     * @param timeoutMs The timeout used for the read operation.
     * @return The number of bytes read.
     */
    size_t read(
        void* buf,
        std::size_t numBytes,
        AttachmentReader::ReadStatus* readStatus,
        std::chrono::milliseconds timeoutMs);

    /**
     * @c MockAttachmentReader constructor.
     *
     * @param input The buffer that holds the input data.
     * @param length The @c input buffer length.
     * @param timeoutIteration Optional iteration that should trigger a timeout response.
     */
    MockAttachmentReader(const unsigned char* input, size_t length, ssize_t timeoutIteration = -1);

private:
    /// Save the index for the last read.
    size_t m_index;

    /// Buffer used to read the input data from.
    const unsigned char* m_input;

    /// The length of the input data.
    size_t m_length;

    /// Read iterations counter.
    ssize_t m_iteration;

    /// This can be used to trigger a timeout response when the iteration reaches the timeout iteration.
    ssize_t m_timeoutIteration;
};

/// Class that can be used to wait for an event.
class WaitEvent {
public:
    /// Wake up a thread that is waiting for this event.
    void wakeUp();

    /// The default timeout for an expected event.
    static const std::chrono::seconds DEFAULT_TIMEOUT;

    /**
     * Wait for wake up event.
     *
     * @param timeout The maximum amount of time to wait for the event.
     */
    std::cv_status wait(const std::chrono::milliseconds& timeout = DEFAULT_TIMEOUT);

private:
    /// The condition variable used to wake up the thread that is waiting.
    std::condition_variable m_condition;
};

/// Mocks the content fetcher factory.
class MockContentFetcherFactory : public HTTPContentFetcherInterfaceFactoryInterface {
public:
    MOCK_METHOD1(create, std::unique_ptr<HTTPContentFetcherInterface>(const std::string& url));
};

/// Mocks the media player observer.
class MockObserver : public MediaPlayerObserverInterface {
public:
    MOCK_METHOD1(onPlaybackStarted, void(SourceId));
    MOCK_METHOD1(onPlaybackFinished, void(SourceId));
    MOCK_METHOD1(onPlaybackStopped, void(SourceId));
    MOCK_METHOD1(onPlaybackPaused, void(SourceId));
    MOCK_METHOD1(onPlaybackResumed, void(SourceId));
    MOCK_METHOD3(onPlaybackError, void(SourceId, const ErrorType&, std::string));

    MOCK_METHOD1(onBufferRefilled, void(SourceId));
    MOCK_METHOD1(onBufferUnderrun, void(SourceId));
};

/**
 * Test class for @c AndroidSLESMediaPlayer
 */
class AndroidSLESMediaPlayerTest : public Test {
protected:
    void SetUp() override {
        m_reader = std::make_shared<MockAttachmentReader>(MP3_INPUT_CSTR, MP3_INPUT_SIZE);
        m_engine = AndroidSLESEngine::create();
        auto factory = std::make_shared<MockContentFetcherFactory>();
        m_player = AndroidSLESMediaPlayer::create(factory, m_engine, SpeakerInterface::Type::AVS_SPEAKER_VOLUME, false);
        m_observer = std::make_shared<NiceMock<MockObserver>>();
        m_player->setObserver(m_observer);
    }

    std::shared_ptr<std::stringstream> createStream() {
        auto stream = std::make_shared<std::stringstream>();
        stream->write(reinterpret_cast<const char*>(MP3_INPUT_CSTR), MP3_INPUT_SIZE);
        return stream;
    }

    void TearDown() override {
        if (m_player) {
            m_player->shutdown();
            m_player.reset();
        }
        m_reader.reset();
    }

    /// We need to instantiate a player in order to use AMedia* functionalitites.
    std::shared_ptr<AndroidSLESMediaPlayer> m_player;

    /// Mock the attachment reader.
    std::shared_ptr<MockAttachmentReader> m_reader;

    /// Mock a media player observer.
    std::shared_ptr<MockObserver> m_observer;

    /// Keep a pointer to the engine.
    std::shared_ptr<AndroidSLESEngine> m_engine;
};

/// Test create with null factory.
TEST_F(AndroidSLESMediaPlayerTest, testCreateNullFactory) {
    auto player = AndroidSLESMediaPlayer::create(nullptr, m_engine, SpeakerInterface::Type::AVS_SPEAKER_VOLUME, false);
    EXPECT_EQ(player, nullptr);
}

/// Test create with null engine.
TEST_F(AndroidSLESMediaPlayerTest, testCreateNullEngine) {
    auto factory = std::make_shared<MockContentFetcherFactory>();
    auto player = AndroidSLESMediaPlayer::create(factory, nullptr, SpeakerInterface::Type::AVS_SPEAKER_VOLUME, false);
    EXPECT_EQ(player, nullptr);
}

/// Test buffer queue with media player.
TEST_F(AndroidSLESMediaPlayerTest, testBQMediaPlayer) {
    auto id = m_player->setSource(m_reader, nullptr);

    WaitEvent finishedEvent;
    EXPECT_CALL(*m_observer, onPlaybackStarted(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackFinished(id)).WillOnce(InvokeWithoutArgs([&finishedEvent]() {
        finishedEvent.wakeUp();
    }));

    EXPECT_TRUE(m_player->play(id));
    EXPECT_EQ(finishedEvent.wait(), std::cv_status::no_timeout);
}

/// Test buffer queue with media player and raw file.
TEST_F(AndroidSLESMediaPlayerTest, testBQRawMediaPlayer) {
    auto format = avsCommon::utils::AudioFormat{.dataSigned = true,
                                                .numChannels = 2,
                                                .sampleSizeInBits = 16,
                                                .sampleRateHz = 48000,
                                                .endianness = avsCommon::utils::AudioFormat::Endianness::LITTLE,
                                                .encoding = avsCommon::utils::AudioFormat::Encoding::LPCM};
    m_reader = std::make_shared<MockAttachmentReader>(RAW_INPUT_CSTR, RAW_INPUT_SIZE);
    auto id = m_player->setSource(m_reader, &format);

    WaitEvent finishedEvent;
    EXPECT_CALL(*m_observer, onPlaybackStarted(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackFinished(id)).WillOnce(InvokeWithoutArgs([&finishedEvent]() {
        finishedEvent.wakeUp();
    }));
    EXPECT_TRUE(m_player->play(id));
    EXPECT_EQ(finishedEvent.wait(), std::cv_status::no_timeout);
}

/// Test that media is played correct even if a timeout happens in the first read.
TEST_F(AndroidSLESMediaPlayerTest, testFirstReadTimeout) {
    // This read iteration indicates the first read call.
    static const ssize_t firstIteration = 0;
    m_reader = std::make_shared<MockAttachmentReader>(MP3_INPUT_CSTR, MP3_INPUT_SIZE, firstIteration);
    auto id = m_player->setSource(m_reader, nullptr);

    WaitEvent finishedEvent;
    EXPECT_CALL(*m_observer, onPlaybackStarted(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackFinished(id)).WillOnce(InvokeWithoutArgs([&finishedEvent]() {
        finishedEvent.wakeUp();
    }));
    EXPECT_TRUE(m_player->play(id));
    EXPECT_EQ(finishedEvent.wait(), std::cv_status::no_timeout);
    EXPECT_EQ(m_player->getOffset(id), MP3_INPUT_DURATION);
}

/// Test that media is played correct even after a timeout during initialization.
TEST_F(AndroidSLESMediaPlayerTest, testInitializeTimeout) {
    // This read iteration occurs during decoder initialization.
    static const ssize_t initializationIteration = 1;
    m_reader = std::make_shared<MockAttachmentReader>(MP3_INPUT_CSTR, MP3_INPUT_SIZE, initializationIteration);
    auto id = m_player->setSource(m_reader, nullptr);

    WaitEvent finishedEvent;
    EXPECT_CALL(*m_observer, onPlaybackStarted(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackFinished(id)).WillOnce(InvokeWithoutArgs([&finishedEvent]() {
        finishedEvent.wakeUp();
    }));
    EXPECT_TRUE(m_player->play(id));
    EXPECT_EQ(finishedEvent.wait(), std::cv_status::no_timeout);
    EXPECT_EQ(m_player->getOffset(id), MP3_INPUT_DURATION);
}

/// Test that media is played correct even after a timeout during decoding.
TEST_F(AndroidSLESMediaPlayerTest, testDecodingTimeout) {
    // This read iteration occurs during decoding state.
    const ssize_t decodeIteration = 10;
    m_reader = std::make_shared<MockAttachmentReader>(MP3_INPUT_CSTR, MP3_INPUT_SIZE, decodeIteration);
    auto id = m_player->setSource(m_reader, nullptr);

    WaitEvent finishedEvent;
    EXPECT_CALL(*m_observer, onPlaybackStarted(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackFinished(id)).WillOnce(InvokeWithoutArgs([&finishedEvent]() {
        finishedEvent.wakeUp();
    }));
    EXPECT_TRUE(m_player->play(id));
    EXPECT_EQ(finishedEvent.wait(), std::cv_status::no_timeout);
    EXPECT_EQ(m_player->getOffset(id), MP3_INPUT_DURATION);
}

/// Test media player and istream source.
TEST_F(AndroidSLESMediaPlayerTest, testStreamMediaPlayer) {
    auto id = m_player->setSource(createStream(), false);

    WaitEvent finishedEvent;
    EXPECT_CALL(*m_observer, onPlaybackStarted(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackFinished(id)).WillOnce(InvokeWithoutArgs([&finishedEvent]() {
        finishedEvent.wakeUp();
    }));

    EXPECT_TRUE(m_player->play(id));
    EXPECT_EQ(finishedEvent.wait(), std::cv_status::no_timeout);
}

/// Test media player, istream source and repeat on.
TEST_F(AndroidSLESMediaPlayerTest, testStreamRepeatMediaPlayer) {
    auto repeat = true;
    auto id = m_player->setSource(createStream(), repeat);

    EXPECT_CALL(*m_observer, onPlaybackStarted(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackStopped(id)).Times(1);
    EXPECT_TRUE(m_player->play(id));

    std::chrono::milliseconds sleepPeriod{100};
    std::this_thread::sleep_for(sleepPeriod);
    EXPECT_TRUE(m_player->stop(id));
}

/// Test media player pause / resume.
TEST_F(AndroidSLESMediaPlayerTest, testResumeMediaPlayer) {
    auto repeat = true;
    auto id = m_player->setSource(createStream(), repeat);

    EXPECT_CALL(*m_observer, onPlaybackStarted(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackStopped(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackPaused(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackResumed(id)).Times(1);
    EXPECT_TRUE(m_player->play(id));

    std::chrono::milliseconds sleepPeriod{100};
    std::this_thread::sleep_for(sleepPeriod);
    EXPECT_TRUE(m_player->pause(id));

    std::this_thread::sleep_for(sleepPeriod);
    EXPECT_TRUE(m_player->resume(id));

    std::this_thread::sleep_for(sleepPeriod);
    EXPECT_TRUE(m_player->stop(id));
}

/// Test play fails with wrong id.
TEST_F(AndroidSLESMediaPlayerTest, testPlayFailed) {
    auto id = m_player->setSource(m_reader, nullptr);
    EXPECT_CALL(*m_observer, onPlaybackStarted(_)).Times(0);
    EXPECT_FALSE(m_player->play(id + 1));
}

/// Test pause fails with wrong id.
TEST_F(AndroidSLESMediaPlayerTest, testPauseFailed) {
    auto id = m_player->setSource(m_reader, nullptr);
    EXPECT_CALL(*m_observer, onPlaybackStarted(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackPaused(_)).Times(0);
    EXPECT_TRUE(m_player->play(id));
    EXPECT_FALSE(m_player->pause(id + 1));
    EXPECT_TRUE(m_player->stop(id));
}

/// Test pause fails if not playing.
TEST_F(AndroidSLESMediaPlayerTest, testPauseFailedNotPlaying) {
    auto id = m_player->setSource(m_reader, nullptr);
    EXPECT_CALL(*m_observer, onPlaybackStarted(id)).Times(0);
    EXPECT_CALL(*m_observer, onPlaybackPaused(_)).Times(0);
    EXPECT_FALSE(m_player->pause(id));
}

/// Test resume fails after stop.
TEST_F(AndroidSLESMediaPlayerTest, testResumeFailedAfterStop) {
    auto id = m_player->setSource(m_reader, nullptr);
    EXPECT_CALL(*m_observer, onPlaybackStarted(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackStopped(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackPaused(_)).Times(0);
    EXPECT_TRUE(m_player->play(id));
    EXPECT_TRUE(m_player->stop(id));
    EXPECT_FALSE(m_player->resume(id));
}

/// Test stop fails with wrong id.
TEST_F(AndroidSLESMediaPlayerTest, testStopFailed) {
    auto id = m_player->setSource(m_reader, nullptr);
    auto fakeId = id + 1;
    EXPECT_CALL(*m_observer, onPlaybackStarted(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackStopped(_)).Times(AtLeast(1));
    EXPECT_CALL(*m_observer, onPlaybackStopped(fakeId)).Times(0);
    EXPECT_TRUE(m_player->play(id));
    EXPECT_FALSE(m_player->stop(fakeId));
    EXPECT_TRUE(m_player->stop(id));
}

/// Test get offset.
TEST_F(AndroidSLESMediaPlayerTest, testGetOffset) {
    auto id = m_player->setSource(m_reader, nullptr);

    WaitEvent finishedEvent;
    EXPECT_CALL(*m_observer, onPlaybackStarted(id)).Times(1);
    EXPECT_CALL(*m_observer, onPlaybackFinished(id)).WillOnce(InvokeWithoutArgs([&finishedEvent]() {
        finishedEvent.wakeUp();
    }));

    EXPECT_TRUE(m_player->play(id));
    EXPECT_EQ(finishedEvent.wait(), std::cv_status::no_timeout);
    EXPECT_EQ(m_player->getOffset(id), MP3_INPUT_DURATION);
}

size_t MockAttachmentReader::read(
    void* buf,
    std::size_t numBytes,
    AttachmentReader::ReadStatus* readStatus,
    std::chrono::milliseconds timeoutMs) {
    m_iteration++;
    if (m_iteration == m_timeoutIteration) {
        (*readStatus) = AttachmentReader::ReadStatus::OK_WOULDBLOCK;
        return 0;
    }

    if (m_index < m_length) {
        numBytes = std::min(numBytes, m_length - m_index);
        memcpy(buf, &m_input[m_index], numBytes);
        (*readStatus) = AttachmentReader::ReadStatus::OK;
        m_index += numBytes;
        return numBytes;
    }
    (*readStatus) = AttachmentReader::ReadStatus::CLOSED;
    return 0;
}

MockAttachmentReader::MockAttachmentReader(const unsigned char* input, size_t length, ssize_t timeoutIteration) :
        m_index{0},
        m_input{input},
        m_length{length},
        m_iteration{-1},
        m_timeoutIteration{timeoutIteration} {
}

const std::chrono::seconds WaitEvent::DEFAULT_TIMEOUT{5};

void WaitEvent::wakeUp() {
    m_condition.notify_one();
}

std::cv_status WaitEvent::wait(const std::chrono::milliseconds& timeout) {
    std::mutex mutex;
    std::unique_lock<std::mutex> lock{mutex};
    return m_condition.wait_for(lock, timeout);
}

}  // namespace test
}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
