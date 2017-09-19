/*
 * MediaPlayerTest.cpp
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

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <memory>
#include <mutex>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Memory/Memory.h>

#include "MediaPlayer/MediaPlayer.h"

namespace alexaClientSDK {
namespace mediaPlayer {
namespace test {

using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils::memory;
using namespace ::testing;

/// String to identify log entries originating from this file.
static const std::string TAG("MediaPlayerTest");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The path to the input Dir containing the test audio files.
std::string inputsDirPath;

/// MP3 test file path.
static const std::string MP3_FILE_PATH("/fox_dog.mp3");

/// Playlist file path.
static const std::string M3U_FILE_PATH("/fox_dog_playlist.m3u");

/// file URI Prefix
static const std::string FILE_PREFIX("file://");

/// File length for the MP3 test file.
static const std::chrono::milliseconds MP3_FILE_LENGTH(2688);

// setOffset timing constants.

/// Offset to start playback at.
static const std::chrono::milliseconds OFFSET(2000);

/// Tolerance when setting expectations.
static const std::chrono::milliseconds TOLERANCE(200);

/// Padding to add to offsets when necessary.
static const std::chrono::milliseconds PADDING(10);

/**
 * Mock AttachmentReader.
 */
class MockAttachmentReader : public AttachmentReader {
public:
    /**
     * Constructor.
     *
     * @param iterations The number of times this AttachmentReader will (re)read the input file before @c read
     * will return a @c CLOSED status.
     * @param receiveSizes An vector of sizes (in bytes) that this @c AttachmentReceiver will simulate receiving.
     * Each successive element in the vector corresponds to a successive 100 millisecond interval starting from
     * the time this @c MockAttachmentReader was created.
     */
    MockAttachmentReader(int iterations = 1, std::vector<size_t> receiveSizes = {std::numeric_limits<size_t>::max()});

    size_t read(
            void* buf, std::size_t numBytes, ReadStatus* readStatus, std::chrono::milliseconds timeoutMs) override;

    void close(ClosePoint closePoint) override;

    /**
     * Receive bytes from the test file.
     *
     * @param buf The buffer to receive the bytes.
     * @param size The number of bytes to receive.
     * @return The number of bytes received.
     */
    size_t receiveBytes(char* buf, std::size_t size);

    /// The number of iterations of reading the input file that are left before this reader returns closed.
    int m_iterationsLeft;

    /**
     * The total number of bytes that are supposed to have been received (and made available) by this
     * @c AttachmentReader at 100 millisecond increments from @c m_startTime.
     */
    std::vector<size_t> m_receiveTotals;

    /// The start of time for reading from this AttachmentReader.
    std::chrono::steady_clock::time_point m_startTime;

    /// The number of bytes returned so far by @c read().
    size_t m_totalRead;

    /// The current ifstream (if any) from which to read the attachment.
    std::unique_ptr<std::ifstream> m_stream;
};

MockAttachmentReader::MockAttachmentReader(int iterations, std::vector<size_t> receiveSizes) :
        m_iterationsLeft{iterations},
        m_startTime(std::chrono::steady_clock::now()),
        m_totalRead{0} {
    // Convert human friendly vector of received sizes in to a vector of received totals.
    EXPECT_GT(receiveSizes.size(), 0U);
    m_receiveTotals.reserve(receiveSizes.size());
    size_t total = 0;
    for (size_t ix = 0; ix < receiveSizes.size(); ix++) {
        total += receiveSizes[ix];
        m_receiveTotals.push_back(total);
    }
    EXPECT_EQ(m_receiveTotals.size(), receiveSizes.size());
}

size_t MockAttachmentReader::read(
        void* buf, std::size_t numBytes, ReadStatus* readStatus, std::chrono::milliseconds timeoutMs) {

    // Convert the current time in to an index in to m_receivedTotals (time since @c m_startTime
    // divided by 100 millisecond intervals).
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime);
    auto index = static_cast<size_t>(elapsed.count() / 100);
    index = std::min(index, m_receiveTotals.size() - 1);

    // Use the index to find the total number of bytes received by the @c MockAttachmentReader so far. Then subtract
    // the number of bytes consumed by @c read() so far to calculate the number of bytes available for @c read().
    auto receivedTotal = m_receiveTotals[index];
    EXPECT_LE(m_totalRead, receivedTotal);
    auto available = receivedTotal - m_totalRead;

    // Use the avaialable number of bytes to calculate how many bytes @c read() should return and the status
    // that should accompany them.  Also perform the actual read in to the output buffer.
    size_t result = 0;
    auto status = ReadStatus::ERROR_INTERNAL;
    if (available > 0) {
        auto sizeToRead = std::min(available, numBytes);
        result = receiveBytes(static_cast<char *>(buf), sizeToRead);
        if (result > 0) {
            m_totalRead += result;
            status = (result == numBytes) ? ReadStatus::OK : ReadStatus::OK_WOULDBLOCK;
        } else {
            status = ReadStatus::CLOSED;
        }
    } else {
        status = ReadStatus::OK_WOULDBLOCK;
    }
    if (readStatus) {
        *readStatus = status;
    }
    return result;
}

void MockAttachmentReader::close(ClosePoint closePoint) {
    if (m_stream) {
        m_stream->close();
    }
}

size_t MockAttachmentReader::receiveBytes(char* buf, std::size_t size) {
    auto pos = buf;
    auto end = buf + size;
    while (pos < end) {
        if (!m_stream || m_stream->eof()) {
            if (m_iterationsLeft-- > 0) {
                m_stream = make_unique<std::ifstream>(inputsDirPath + MP3_FILE_PATH);
                EXPECT_TRUE(m_stream);
                EXPECT_TRUE(m_stream->good());
            } else {
                break;
            }
        }
        m_stream->read(pos, end - pos);
        pos += m_stream->gcount();
    }
    auto result = pos - buf;
    return result;
}

class MockPlayerObserver: public MediaPlayerObserverInterface {
public:
    /**
     * Destructor.
     */
    ~MockPlayerObserver() {};

    void onPlaybackStarted() override;

    void onPlaybackFinished() override;

    void onPlaybackError(std::string error) override;

    void onPlaybackPaused() override;

    void onPlaybackResumed() override;

    /**
     * Wait for a message to be received.
     *
     * This function waits for a specified number of milliseconds for a message to arrive.
     * @param duration Number of milliseconds to wait before giving up.
     * @return true if a message was received within the specified duration, else false.
     */
    bool waitForPlaybackStarted(const std::chrono::milliseconds duration = std::chrono::milliseconds(5000));

    /**
     * Wait for a message to be received.
     *
     * This function waits for a specified number of milliseconds for a message to arrive.
     * @param duration Number of milliseconds to wait before giving up.
     * @return true if a message was received within the specified duration, else false.
     */
    bool waitForPlaybackFinished(const std::chrono::milliseconds duration = std::chrono::milliseconds(5000));

    /**
     * Wait for a message to be received.
     *
     * This function waits for a specified number of milliseconds for a message to arrive.
     * @param duration Number of milliseconds to wait before giving up.
     * @return true if a message was received within the specified duration, else false.
     */
    bool waitForPlaybackPaused(const std::chrono::milliseconds duration = std::chrono::milliseconds(5000));

    /**
     * Wait for a message to be received.
     *
     * This function waits for a specified number of milliseconds for a message to arrive.
     * @param duration Number of milliseconds to wait before giving up.
     * @return true if a message was received within the specified duration, else false.
     */
    bool waitForPlaybackResumed(const std::chrono::milliseconds duration = std::chrono::milliseconds(5000));

    /**
     * TODO: Make this class a mock and remove this.
     *
     * This gets the number of times onPlaybackStarted was called.
     */
    int getOnPlaybackStartedCallCount();

    /**
     * TODO: Make this class a mock and remove this.
     *
     * This gets the number of times onPlaybackFinished was called.
     */
    int getOnPlaybackFinishedCallCount();

private:
    /// Mutex to protect the flags @c m_playbackStarted and .@c m_playbackFinished.
    std::mutex m_mutex;
    /// Trigger to wake up m_wakePlaybackStarted calls.
    std::condition_variable m_wakePlaybackStarted;
    /// Trigger to wake up m_wakePlaybackStarted calls.
    std::condition_variable m_wakePlaybackFinished;
    /// Trigger to wake up m_wakePlaybackPaused calls.
    std::condition_variable m_wakePlaybackPaused;
    /// Trigger to wake up m_wakePlaybackResumed calls.
    std::condition_variable m_wakePlaybackResumed;

    // TODO: Make this class a mock and remove these.
    int m_onPlaybackStartedCallCount = 0;
    int m_onPlaybackFinishedCallCount = 0;

    /// Flag to set when a playback start message is received.
    bool m_playbackStarted;
    /// Flag to set when a playback finished message is received.
    bool m_playbackFinished;
    /// Flag to set when a playback paused message is received.
    bool m_playbackPaused;
    /// Flag to set when a playback paused message is received.
    bool m_playbackResumed;
};

void MockPlayerObserver::onPlaybackStarted() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_playbackStarted = true;
    m_playbackFinished = false;
    m_wakePlaybackStarted.notify_all();
    m_onPlaybackStartedCallCount++;
}

void MockPlayerObserver::onPlaybackFinished() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_playbackFinished = true;
    m_playbackStarted = false;
    m_wakePlaybackFinished.notify_all();
    m_onPlaybackFinishedCallCount++;
}

void MockPlayerObserver::onPlaybackError(std::string error) {
    ACSDK_ERROR(LX("onPlaybackError").d("error", error));
};

void MockPlayerObserver::onPlaybackPaused() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_playbackPaused = true;
    m_wakePlaybackPaused.notify_all();
};

void MockPlayerObserver::onPlaybackResumed() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_playbackResumed = true;
    m_playbackPaused = false;
    m_wakePlaybackResumed.notify_all();
};

bool MockPlayerObserver::waitForPlaybackStarted(const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakePlaybackStarted.wait_for(lock, duration, [this]() { return m_playbackStarted; } ))
    {
        return false;
    }
    return true;
}

bool MockPlayerObserver::waitForPlaybackFinished(const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakePlaybackFinished.wait_for(lock, duration, [this]() { return m_playbackFinished; } ))
    {
        return false;
    }
    return true;
}

bool MockPlayerObserver::waitForPlaybackPaused(const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakePlaybackPaused.wait_for(lock, duration, [this]() { return m_playbackPaused; } ))
    {
        return false;
    }
    return true;
}

bool MockPlayerObserver::waitForPlaybackResumed(const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakePlaybackResumed.wait_for(lock, duration, [this]() { return m_playbackResumed; } ))
    {
        return false;
    }
    return true;
}

int MockPlayerObserver::getOnPlaybackStartedCallCount() {
    return m_onPlaybackStartedCallCount;
}

int MockPlayerObserver::getOnPlaybackFinishedCallCount() {
    return m_onPlaybackFinishedCallCount;
}

class MediaPlayerTest: public ::testing::Test{
public:
    void SetUp() override;

    /**
     * Sets the audio source to play.
     *
     */
    void setAttachmentReaderSource(
            int iterations = 1,
            std::vector<size_t> receiveSizes = {std::numeric_limits<size_t>::max()});

    /**
     * Sets IStream source to play.
     *
     * @param repeat Whether to play the stream over and over until stopped.
     */
    void setIStreamSource(bool repeat = false);

    /// An instance of the @c MediaPlayer
    std::shared_ptr<MediaPlayer> m_mediaPlayer;

    /// An observer to whom the playback started and finished notifications need to be sent.
    std::shared_ptr<MockPlayerObserver> m_playerObserver;
};

void MediaPlayerTest::SetUp() {
    m_playerObserver = std::make_shared<MockPlayerObserver>();
    m_mediaPlayer = MediaPlayer::create();
    ASSERT_TRUE(m_mediaPlayer);
    m_mediaPlayer->setObserver(m_playerObserver);
}

void MediaPlayerTest::setAttachmentReaderSource(int iterations, std::vector<size_t> receiveSizes) {
    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->setSource(
            std::unique_ptr<AttachmentReader>(new MockAttachmentReader(iterations, receiveSizes))));
}

void MediaPlayerTest::setIStreamSource(bool repeat) {
    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->setSource(
            make_unique<std::ifstream>(inputsDirPath + MP3_FILE_PATH), repeat));
}

/**
 * Read an audio file into a buffer. Set the source of the @c MediaPlayer to the buffer. Playback audio till the end.
 * Check whether the playback started and playback finished notifications are received.
 */
TEST_F(MediaPlayerTest, testStartPlayWaitForEnd) {
    setAttachmentReaderSource();

    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());
}

/**
 * Set the source of the @c MediaPlayer to a url representing a single audio file. Playback audio till the end.
 * Check whether the playback started and playback finished notifications are received.
 */
TEST_F(MediaPlayerTest, testStartPlayForUrl) {

    std::string url_single(FILE_PREFIX + inputsDirPath + MP3_FILE_PATH);
    m_mediaPlayer->setSource(url_single);

    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());
}

/**
 * Set the source of the @c MediaPlayer to an empty url. The return should be a nullptr.
 */
TEST_F(MediaPlayerTest, testSetSourceEmptyUrl) {
    ASSERT_EQ(m_mediaPlayer->setSource(""), MediaPlayerStatus::FAILURE);
}

/**
 * Set the source of the @c MediaPlayer twice consecutively to a url representing a single audio file.
 * Playback audio till the end. Check whether the playback started and playback finished notifications
 * are received.
 *
 * Consecutive calls to setSource(const std::string url) without play() cause tests to occasionally fail: ACSDK-508.
 */
TEST_F(MediaPlayerTest, testConsecutiveSetSource) {

    std::string url_single(FILE_PREFIX + inputsDirPath + MP3_FILE_PATH);
    m_mediaPlayer->setSource("");
    m_mediaPlayer->setSource(url_single);

    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());
}

/**
 * Read an audio file into a buffer. Set the source of the @c MediaPlayer to the buffer. Playback audio till the end.
 * Check whether the playback started and playback finished notifications are received.
 * Call @c play. The audio should play again from the beginning. Wait till the end.
 */
TEST_F(MediaPlayerTest, testStartPlayWaitForEndStartPlayAgain) {
    setIStreamSource();

    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());

    setAttachmentReaderSource();
    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());
}

/**
 * Read an audio file into a buffer. Set the source of the @c MediaPlayer to the buffer. Playback audio for a few
 * seconds. Playback started notification should be received when the playback starts. Then call @c stop and expect
 * the playback finished notification is received.
 */
TEST_F(MediaPlayerTest, testStopPlay) {
    setIStreamSource(true);

    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    std::this_thread::sleep_for (std::chrono::seconds(5));
    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->stop());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());
}

/**
 * Read an audio file into a buffer. Set the source of the @c MediaPlayer to the buffer. Playback audio for a few
 * seconds. Playback started notification should be received when the playback starts. Call @c stop.
 * and wait for the playback finished notification is received. Call @c play again.
 */
TEST_F(MediaPlayerTest, testStartPlayCallAfterStopPlay) {
    setAttachmentReaderSource();

    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    std::this_thread::sleep_for (std::chrono::seconds(1));
    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->stop());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());

    setAttachmentReaderSource();

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    std::this_thread::sleep_for (std::chrono::seconds(1));
    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->stop());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());
}

/*
 * Pause an audio after playback has started.
 */
TEST_F(MediaPlayerTest, testPauseDuringPlay) {
    setIStreamSource(true);

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->pause());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused());
    // onPlaybackFinish should NOT be called during a pause.
    // TODO: Detect this via making the MediaPlayerObserverMock a mock object.
    ASSERT_EQ(m_playerObserver->getOnPlaybackFinishedCallCount(), 0);
}

/*
 * Play of a paused audio. This behavior is not supported, and will result in the
 * audio being stopped.
 */
TEST_F(MediaPlayerTest, testPlayAfterPause) {
    setIStreamSource(true);

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->pause());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());
    // TODO: Make the MediaPlayerObserverMock a mock object and ensure onPlaybackStarted should NOT be called
    // during a resume.
    ASSERT_EQ(m_playerObserver->getOnPlaybackStartedCallCount(), 1);
    ASSERT_EQ(m_playerObserver->getOnPlaybackFinishedCallCount(), 1);
}

/*
 * Stop of a paused audio after playback has started. An additional stop and play event should
 * be sent.
 */
TEST_F(MediaPlayerTest, testStopAfterPause) {
    setIStreamSource(true);

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->pause());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->stop());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());
    // TODO: Make the MediaPlayerObserverMock a mock object and ensure onPlaybackStarted should NOT be called
    // during a resume.
    ASSERT_EQ(m_playerObserver->getOnPlaybackStartedCallCount(), 1);
    ASSERT_EQ(m_playerObserver->getOnPlaybackFinishedCallCount(), 1);
}

/*
 * Pause of a paused audio after playback has started. The pause() should fail.
 */
TEST_F(MediaPlayerTest, testPauseAfterPause) {
    setIStreamSource(true);

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->pause());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused());

    ASSERT_EQ(MediaPlayerStatus::FAILURE, m_mediaPlayer->pause());

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->stop());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());
}

/*
 * Resume play of a paused audio after playback has started.
 */
TEST_F(MediaPlayerTest, testResumeAfterPause) {
    setIStreamSource(true);

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->pause());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->resume());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackResumed());
    // onPlaybackStarted should NOT be called during a pause.
    // TODO: Make the MediaPlayerObserverMock a mock object and ensure onPlaybackStarted should NOT be called
    // during a resume.
    ASSERT_EQ(m_playerObserver->getOnPlaybackStartedCallCount(), 1);
}

/*
 * Calling resume after playback has started. The resume operation should fail.
 */
TEST_F(MediaPlayerTest, testResumeAfterPlay) {
    setIStreamSource(true);

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());

    ASSERT_EQ(MediaPlayerStatus::FAILURE, m_mediaPlayer->resume());

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->stop());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());
}

/**
 * Read an audio file into a buffer. Set the source of the @c MediaPlayer to the buffer. Playback audio for a few
 * seconds. Playback started notification should be received when the playback starts. Call @c getOffsetInMilliseconds.
 * Check the offset value. Then call @c stop and expect the playback finished notification is received.
 * Call @c getOffsetInMilliseconds again. Check the offset value.
 */
TEST_F(MediaPlayerTest, testGetOffsetInMilliseconds) {
    setAttachmentReaderSource();

    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    std::this_thread::sleep_for (std::chrono::seconds(1));
    int64_t offset = m_mediaPlayer->getOffsetInMilliseconds();
    ASSERT_TRUE((offset > 0) && (offset <= MP3_FILE_LENGTH.count()));
    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->stop());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());
    ASSERT_EQ(-1, m_mediaPlayer->getOffsetInMilliseconds());
}

/**
 * Test getOffsetInMilliseconds with a null pipeline. Expect that -1 is returned.
 * This currently results in errors on shutdown. Will be fixed by ACSDK-446.
 */
TEST_F(MediaPlayerTest, testGetOffsetInMillisecondsNullPipeline) {
    ASSERT_EQ(-1, m_mediaPlayer->getOffsetInMilliseconds());
}

/**
 * Check playing two attachments back to back.
 * Read an audio file into a buffer. Set the source of the @c MediaPlayer to the buffer. Playback audio for a few
 * seconds. Playback started notification should be received when the playback starts. Call @c getOffsetInMilliseconds.
 * Check the offset value. Wait for playback to finish and expect the playback finished notification is received.
 * Repeat the above for a new source.
 */
TEST_F(MediaPlayerTest, testPlayingTwoAttachments) {
    setAttachmentReaderSource();

    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    std::this_thread::sleep_for (std::chrono::seconds(1));
    ASSERT_NE(-1, m_mediaPlayer->getOffsetInMilliseconds());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());

    setAttachmentReaderSource();

    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    std::this_thread::sleep_for (std::chrono::seconds(1));
    ASSERT_NE(-1, m_mediaPlayer->getOffsetInMilliseconds());
    std::this_thread::sleep_for (std::chrono::seconds(1));
    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->stop());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());
}

/**
 * Check playback of an attachment that is received sporadically. Playback started notification should be received
 * when the playback starts. Wait for playback to finish and expect the playback finished notification is received.
 * To a human ear the playback of this test is expected to sound reasonably smooth.
 */
TEST_F(MediaPlayerTest, testUnsteadyReads) {
    setAttachmentReaderSource(
            3, {
                    // Sporadic receive sizes averaging out to about 6000 bytes per second.
                    // Each element corresponds to a 100 millisecond time interval, so each
                    // row of 10 corresponds to a second's worth of sizes of data.
                    4000, 1000, 500, 500, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 500, 0, 500, 0, 1000, 0, 4000,
                    0, 100, 100, 100, 100, 100, 0, 2500, 0, 3000,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 6000, 0, 0, 0, 6000,
                    0, 0, 0, 3000, 0, 0, 0, 0, 0, 3000,
                    0, 2000, 0, 0, 2000, 0, 0, 0, 2000, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 12000,
                    0, 0, 0, 1000, 0, 0, 0, 1000, 0, 1000,
                    0, 0, 0, 0, 3000, 0, 0, 0, 0, 6000
            });
    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(std::chrono::milliseconds(15000)));
}

// TODO: ACSDK-300 This test fails frequently on the Raspberry Pi platform.
#ifdef RESOLVED_ACSDK_300
/**
 * Check playback of an attachment whose receipt is interrupted for about 3 seconds.  Playback started notification
 * should be received when the playback starts. Wait for playback to finish and expect the playback finished
 * notification is received. To a human ear the playback of this test is expected to sound reasonably smooth
 * initially, then be interrupted for a few seconds, and then continue fairly smoothly.
 */
TEST_F(MediaPlayerTest, testRecoveryFromPausedReads) {
    setAttachmentReaderSource(
            3, {
                    // Receive sizes averaging out to 6000 bytes per second with a 3 second gap.
                    // Each element corresponds to a 100 millisecond time interval, so each
                    // row of 10 corresponds to a second's worth of sizes of data.
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 6000,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 6000,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 6000,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 18000,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 6000,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 6000,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 6000,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 6000
            });
    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(std::chrono::milliseconds(20000)));
}

#endif

#ifdef TOTEM_PLPARSER
/**
 * Check playback of an URL identifying a playlist. Wait until the end.
 * Ensure that onPlaybackStarted and onPlaybackFinished are only called once each.
 */

TEST_F(MediaPlayerTest, testStartPlayWithUrlPlaylistWaitForEnd) {

    std::string url_playlist(FILE_PREFIX + inputsDirPath + M3U_FILE_PATH);
    m_mediaPlayer->setSource(url_playlist);

    ASSERT_NE(MediaPlayerStatus::FAILURE,m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(std::chrono::milliseconds(10000)));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(std::chrono::milliseconds(10000)));
    ASSERT_EQ(m_playerObserver->getOnPlaybackStartedCallCount(), 1);
    ASSERT_EQ(m_playerObserver->getOnPlaybackFinishedCallCount(), 1);
}
#endif

/**
 * Test setting the offset to a seekable source. Setting the offset should succeed and playback should start from the offset.
 */
TEST_F(MediaPlayerTest, testSetOffsetSeekableSource) {
    std::chrono::milliseconds offset(OFFSET);

    std::string url_single(FILE_PREFIX + inputsDirPath + MP3_FILE_PATH);
    m_mediaPlayer->setSource(url_single);
    ASSERT_EQ(MediaPlayerStatus::SUCCESS, m_mediaPlayer->setOffset(offset));

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    auto start = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());

    std::chrono::milliseconds timeElapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    ACSDK_INFO(LX("MediaPlayerTest").d("timeElapsed", timeElapsed.count()));

    // Time elapsed should be total file length minus the offset.
    ASSERT_TRUE(timeElapsed < (MP3_FILE_LENGTH - offset + TOLERANCE));
    ASSERT_EQ(m_playerObserver->getOnPlaybackStartedCallCount(), 1);
    ASSERT_EQ(m_playerObserver->getOnPlaybackFinishedCallCount(), 1);
}

/**
 * Test setting the offset to an un-seekable pipeline. Setting the offset should succeed, but
 * no seeking should occur. Playback will start from the beginning.
 */
TEST_F(MediaPlayerTest, testSetOffsetUnseekable) {
    std::chrono::milliseconds offset(OFFSET);

    setAttachmentReaderSource();
    // Ensure that source is set to not seekable.
    gst_app_src_set_stream_type(m_mediaPlayer->getAppSrc(), GST_APP_STREAM_TYPE_STREAM);

    ASSERT_EQ(MediaPlayerStatus::SUCCESS, m_mediaPlayer->setOffset(offset));

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    auto start = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());

    std::chrono::milliseconds timeElapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    ACSDK_INFO(LX("MediaPlayerTest").d("timeElapsed", timeElapsed.count()));

    // Time elapsed should be the length of the file.
    ASSERT_TRUE(timeElapsed >= (MP3_FILE_LENGTH));
    ASSERT_EQ(m_playerObserver->getOnPlaybackStartedCallCount(), 1);
    ASSERT_EQ(m_playerObserver->getOnPlaybackFinishedCallCount(), 1);
}

/**
 * Test setting the offset outside the bounds of the source. Playback will immediately end.
 */
TEST_F(MediaPlayerTest, testSetOffsetOutsideBounds) {
    std::chrono::milliseconds outOfBounds(MP3_FILE_LENGTH + PADDING);

    std::string url_single(FILE_PREFIX + inputsDirPath + MP3_FILE_PATH);
    m_mediaPlayer->setSource(url_single);
    ASSERT_EQ(MediaPlayerStatus::SUCCESS, m_mediaPlayer->setOffset(outOfBounds));

    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    auto start = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());

    std::chrono::milliseconds timeElapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    ACSDK_INFO(LX("MediaPlayerTest").d("timeElapsed", timeElapsed.count()));

    // Time elapsed should be zero.
    ASSERT_TRUE(timeElapsed < std::chrono::milliseconds::zero() + TOLERANCE);
    ASSERT_EQ(m_playerObserver->getOnPlaybackStartedCallCount(), 1);
    ASSERT_EQ(m_playerObserver->getOnPlaybackFinishedCallCount(), 1);
}

/**
 * Test calling setSource resets the offset.
 *
 * Consecutive calls to setSource(const std::string url) without play() cause tests to occasionally fail: ACSDK-508.
 */
TEST_F(MediaPlayerTest, testSetSourceResetsOffset) {
    std::chrono::milliseconds offset(OFFSET);

    std::string url_single(FILE_PREFIX + inputsDirPath + MP3_FILE_PATH);
    m_mediaPlayer->setSource(url_single);
    ASSERT_EQ(MediaPlayerStatus::SUCCESS, m_mediaPlayer->setOffset(offset));

    m_mediaPlayer->setSource(url_single);
    // Play, expect full file.
    ASSERT_NE(MediaPlayerStatus::FAILURE, m_mediaPlayer->play());
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted());
    auto start = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished());

    std::chrono::milliseconds timeElapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    ACSDK_INFO(LX("MediaPlayerTest").d("timeElapsed", timeElapsed.count()));

    // Time elapsed should be the full file.
    ASSERT_TRUE(timeElapsed >= MP3_FILE_LENGTH);
    ASSERT_EQ(m_playerObserver->getOnPlaybackStartedCallCount(), 1);
    ASSERT_EQ(m_playerObserver->getOnPlaybackFinishedCallCount(), 1);
}

} // namespace test
} // namespace mediaPlayer
} // namespace alexaClientSDK

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (argc < 2) {
        std::cerr << "Usage: MediaPlayerTest <absolute path to test inputs folder>" << std::endl;
    } else {
        alexaClientSDK::mediaPlayer::test::inputsDirPath = std::string(argv[1]);
        return RUN_ALL_TESTS();
    }
}
