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

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <PlaylistParser/PlaylistParser.h>

#include "MediaPlayer/MediaPlayer.h"
#include "AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h"

namespace alexaClientSDK {
namespace mediaPlayer {
namespace test {

using namespace avsCommon::avs::attachment;
using namespace avsCommon::avs::speakerConstants;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::utils::memory;
using namespace ::testing;

/// String to identify log entries originating from this file.
static const std::string TAG("MediaPlayerTest");

static const MediaPlayer::SourceId ERROR_SOURCE_ID = MediaPlayer::ERROR;

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

static const std::string TEST_M3U_PLAYLIST_URL("fox_dog_playlist.m3u");

static std::string TEST_M3U_PLAYLIST_CONTENT;

/// file URI Prefix
static const std::string FILE_PREFIX("file://");

/// File length for the MP3 test file.
static const std::chrono::milliseconds MP3_FILE_LENGTH(2688);

// setOffset timing constants.

/// Offset to start playback at.
static const std::chrono::milliseconds OFFSET(2000);

#ifdef RESOLVED_ACSDK_627

/// Tolerance when setting expectations.
static const std::chrono::milliseconds TOLERANCE(200);

#endif

/// Padding to add to offsets when necessary.
static const std::chrono::milliseconds PADDING(10);

static std::unordered_map<std::string, std::string> urlsToContentTypes;

static std::unordered_map<std::string, std::string> urlsToContent;

/// A mock content fetcher
class MockContentFetcher : public avsCommon::sdkInterfaces::HTTPContentFetcherInterface {
public:
    MockContentFetcher(const std::string& url) : m_url{url} {
    }

    std::unique_ptr<avsCommon::utils::HTTPContent> getContent(
        FetchOptions fetchOption,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> writer) override {
        if (fetchOption == FetchOptions::CONTENT_TYPE) {
            auto urlAndContentType = urlsToContentTypes.find(m_url);
            if (urlAndContentType == urlsToContentTypes.end()) {
                return nullptr;
            } else {
                std::promise<long> statusPromise;
                auto statusFuture = statusPromise.get_future();
                statusPromise.set_value(200);
                std::promise<std::string> contentTypePromise;
                auto contentTypeFuture = contentTypePromise.get_future();
                contentTypePromise.set_value(urlAndContentType->second);
                return avsCommon::utils::memory::make_unique<avsCommon::utils::HTTPContent>(
                    avsCommon::utils::HTTPContent{std::move(statusFuture), std::move(contentTypeFuture), nullptr});
            }
        } else {
            auto urlAndContent = urlsToContent.find(m_url);
            if (urlAndContent == urlsToContent.end()) {
                return nullptr;
            }
            std::promise<long> statusPromise;
            auto statusFuture = statusPromise.get_future();
            statusPromise.set_value(200);
            std::promise<std::string> contentTypePromise;
            auto contentTypeFuture = contentTypePromise.get_future();
            contentTypePromise.set_value("");
            auto attachment = writeStringIntoAttachment(urlAndContent->second, writer);
            if (!attachment) {
                return nullptr;
            }
            return avsCommon::utils::memory::make_unique<avsCommon::utils::HTTPContent>(
                avsCommon::utils::HTTPContent{std::move(statusFuture), std::move(contentTypeFuture), attachment});
        }
    }

private:
    std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> writeStringIntoAttachment(
        const std::string& string,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> writer) {
        static int id = 0;
        std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> stream =
            std::make_shared<avsCommon::avs::attachment::InProcessAttachment>(std::to_string(id++));
        if (!stream) {
            return nullptr;
        }

        if (!writer) {
            writer = stream->createWriter();
        }
        avsCommon::avs::attachment::AttachmentWriter::WriteStatus writeStatus;
        writer->write(string.data(), string.size(), &writeStatus);
        return stream;
    };

    std::string m_url;
};

/// A mock factory that creates mock content fetchers
class MockContentFetcherFactory : public avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface {
    std::unique_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> create(const std::string& url) {
        return avsCommon::utils::memory::make_unique<MockContentFetcher>(url);
    }
};

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

    size_t read(void* buf, std::size_t numBytes, ReadStatus* readStatus, std::chrono::milliseconds timeoutMs) override;

    void close(ClosePoint closePoint) override;

    bool seek(uint64_t offset) override {
        return true;
    }

    uint64_t getNumUnreadBytes() override {
        return 0;
    }

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
    void* buf,
    std::size_t numBytes,
    ReadStatus* readStatus,
    std::chrono::milliseconds timeoutMs) {
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
        result = receiveBytes(static_cast<char*>(buf), sizeToRead);
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

class MockPlayerObserver : public MediaPlayerObserverInterface {
public:
    /**
     * Destructor.
     */
    ~MockPlayerObserver(){};

    // void onPlaybackStarted() override;
    void onPlaybackStarted(SourceId id) override;

    void onPlaybackFinished(SourceId id) override;

    void onPlaybackError(SourceId id, const ErrorType& type, std::string error) override;

    void onPlaybackPaused(SourceId id) override;

    void onPlaybackResumed(SourceId id) override;

    void onPlaybackStopped(SourceId id) override;

    void onTags(SourceId id, std::unique_ptr<const VectorOfTags> vectorOfTags) override;

    /**
     * Wait for a message to be received.
     *
     * This function waits for a specified number of milliseconds for a message to arrive.
     * @param id The @c SourceId expected in a callback.
     * @param duration Number of milliseconds to wait before giving up.
     * @return true if a message was received within the specified duration, else false.
     */
    bool waitForPlaybackStarted(
        SourceId id,
        const std::chrono::milliseconds duration = std::chrono::milliseconds(5000));

    /**
     * Wait for a message to be received.
     *
     * This function waits for a specified number of milliseconds for a message to arrive.
     * @param id The @c SourceId expected in a callback.
     * @param duration Number of milliseconds to wait before giving up.
     * @return true if a message was received within the specified duration, else false.
     */
    bool waitForPlaybackFinished(
        SourceId id,
        const std::chrono::milliseconds duration = std::chrono::milliseconds(5000));

    /**
     * Wait for a message to be received.
     *
     * This function waits for a specified number of milliseconds for a message to arrive.
     * @param id The @c SourceId expected in a callback.
     * @param duration Number of milliseconds to wait before giving up.
     * @return true if a message was received within the specified duration, else false.
     */
    bool waitForPlaybackPaused(SourceId id, const std::chrono::milliseconds duration = std::chrono::milliseconds(5000));

    /**
     * Wait for a message to be received.
     *
     * This function waits for a specified number of milliseconds for a message to arrive.
     * @param id The @c SourceId expected in a callback.
     * @param duration Number of milliseconds to wait before giving up.
     * @return true if a message was received within the specified duration, else false.
     */
    bool waitForPlaybackResumed(
        SourceId id,
        const std::chrono::milliseconds duration = std::chrono::milliseconds(5000));

    /**
     * Wait for a message to be received.
     *
     * This function waits for a specified number of milliseconds for a message to arrive.
     * @param id The @c SourceId expected in a callback.
     * @param duration Number of milliseconds to wait before giving up.
     * @return true if a message was received within the specified duration, else false.
     */
    bool waitForPlaybackStopped(
        SourceId id,
        const std::chrono::milliseconds duration = std::chrono::milliseconds(5000));

    /**
     * Wait for a playback error message to be received.
     *
     * This function waits for a specified number of milliseconds for a message to arrive.
     * @param id The @c SourceId expected in a callback.
     * @param duration Number of milliseconds to wait before giving up.
     * @return true if a message was received within the specified duration, else false.
     */
    bool waitForPlaybackError(SourceId id, const std::chrono::milliseconds duration = std::chrono::milliseconds(5000));

    /**
     * Wait for a message to be received.
     * This function waits for a specified number of milliseconds for a message to arrive.
     *
     * @param id The @c SourceId expected in a callback.
     * @param duration Number of milliseconds to wait before giving up.
     * @return true if a message was received within the specified duration, else false.
     */
    bool waitForTags(SourceId id, const std::chrono::milliseconds duration = std::chrono::milliseconds(5000));

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

    /**
     * TODO: Make this class a mock and remove this.
     *
     * This gets the number of times onTags was called.
     */
    int getOnTagsCallCount();

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
    std::condition_variable m_wakePlaybackStopped;
    std::condition_variable m_wakePlaybackError;
    /// Trigger to wake up m_wakeTags calls.
    std::condition_variable m_wakeTags;

    // TODO: Make this class a mock and remove these.
    int m_onPlaybackStartedCallCount = 0;
    int m_onPlaybackFinishedCallCount = 0;
    int m_onTagsCallCount = 0;

    /// Flag to set when a playback start message is received.
    bool m_playbackStarted;
    /// Flag to set when a playback finished message is received.
    bool m_playbackFinished;
    /// Flag to set when a playback paused message is received.
    bool m_playbackPaused;
    /// Flag to set when a playback paused message is received.
    bool m_playbackResumed;
    /// Flag to set when a playback stopped message is received.
    bool m_playbackStopped;
    /// Flag to set when a playback error message is received.
    bool m_playbackError;
    /// Flag to set when a tags message is received.
    bool m_tags;

    SourceId m_lastId = 0;
};

void MockPlayerObserver::onPlaybackStarted(SourceId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastId = id;
    m_playbackStarted = true;
    m_playbackFinished = false;
    m_playbackStopped = false;
    m_wakePlaybackStarted.notify_all();
    m_onPlaybackStartedCallCount++;
}

void MockPlayerObserver::onPlaybackFinished(SourceId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastId = id;
    m_playbackFinished = true;
    m_playbackStarted = false;
    m_wakePlaybackFinished.notify_all();
    m_onPlaybackFinishedCallCount++;
}

void MockPlayerObserver::onPlaybackError(SourceId id, const ErrorType& type, std::string error) {
    m_lastId = id;
    m_playbackError = true;
    m_wakePlaybackError.notify_all();
};

void MockPlayerObserver::onPlaybackPaused(SourceId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastId = id;
    m_playbackPaused = true;
    m_wakePlaybackPaused.notify_all();
};

void MockPlayerObserver::onPlaybackResumed(SourceId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastId = id;
    m_playbackResumed = true;
    m_playbackPaused = false;
    m_wakePlaybackResumed.notify_all();
};

void MockPlayerObserver::onPlaybackStopped(SourceId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastId = id;
    m_playbackStopped = true;
    m_playbackStarted = false;
    m_wakePlaybackStopped.notify_all();
};

bool MockPlayerObserver::waitForPlaybackStarted(SourceId id, const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakePlaybackStarted.wait_for(lock, duration, [this, id]() { return m_playbackStarted && id == m_lastId; })) {
        return false;
    }
    return true;
}

bool MockPlayerObserver::waitForPlaybackFinished(SourceId id, const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakePlaybackFinished.wait_for(
            lock, duration, [this, id]() { return m_playbackFinished && id == m_lastId; })) {
        return false;
    }
    return true;
}

bool MockPlayerObserver::waitForPlaybackPaused(SourceId id, const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakePlaybackPaused.wait_for(lock, duration, [this, id]() { return m_playbackPaused && id == m_lastId; })) {
        return false;
    }
    return true;
}

bool MockPlayerObserver::waitForPlaybackResumed(SourceId id, const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakePlaybackResumed.wait_for(lock, duration, [this, id]() { return m_playbackResumed && id == m_lastId; })) {
        return false;
    }
    return true;
}

bool MockPlayerObserver::waitForPlaybackStopped(SourceId id, const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakePlaybackStopped.wait_for(lock, duration, [this, id]() { return m_playbackStopped && id == m_lastId; })) {
        return false;
    }
    return true;
}

bool MockPlayerObserver::waitForPlaybackError(SourceId id, const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakePlaybackError.wait_for(lock, duration, [this, id]() { return m_playbackError && id == m_lastId; })) {
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

int MockPlayerObserver::getOnTagsCallCount() {
    return m_onTagsCallCount;
}

void MockPlayerObserver::onTags(SourceId id, std::unique_ptr<const VectorOfTags> vectorOfTags) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastId = id;
    m_tags = true;
    m_onTagsCallCount++;
    m_wakeTags.notify_all();
}

bool MockPlayerObserver::waitForTags(SourceId id, const std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTags.wait_for(lock, duration, [this, id]() { return m_tags && id == m_lastId; })) {
        return false;
    }
    return true;
}

class MediaPlayerTest : public ::testing::Test {
public:
    void SetUp() override;

    void TearDown() override {
        m_mediaPlayer->shutdown();
    }

    /**
     * Sets the audio source to play.
     *
     */
    void setAttachmentReaderSource(
        MediaPlayer::SourceId* id,
        int iterations = 1,
        std::vector<size_t> receiveSizes = {std::numeric_limits<size_t>::max()});

    /**
     * Sets IStream source to play.
     *
     * @param repeat Whether to play the stream over and over until stopped.
     */
    void setIStreamSource(MediaPlayer::SourceId* id, bool repeat = false);

    /// An instance of the @c MediaPlayer
    std::shared_ptr<MediaPlayer> m_mediaPlayer;

    /// An observer to whom the playback started and finished notifications need to be sent.
    std::shared_ptr<MockPlayerObserver> m_playerObserver;
};

void MediaPlayerTest::SetUp() {
    m_playerObserver = std::make_shared<MockPlayerObserver>();
    m_mediaPlayer = MediaPlayer::create(std::make_shared<MockContentFetcherFactory>());
    ASSERT_TRUE(m_mediaPlayer);
    m_mediaPlayer->setObserver(m_playerObserver);
}

void MediaPlayerTest::setAttachmentReaderSource(
    MediaPlayer::SourceId* id,
    int iterations,
    std::vector<size_t> receiveSizes) {
    auto returnId =
        m_mediaPlayer->setSource(std::unique_ptr<AttachmentReader>(new MockAttachmentReader(iterations, receiveSizes)));
    ASSERT_NE(ERROR_SOURCE_ID, returnId);
    if (id) {
        *id = returnId;
    }
}

void MediaPlayerTest::setIStreamSource(MediaPlayer::SourceId* id, bool repeat) {
    auto returnId = m_mediaPlayer->setSource(make_unique<std::ifstream>(inputsDirPath + MP3_FILE_PATH), repeat);
    ASSERT_NE(ERROR_SOURCE_ID, returnId);
    if (id) {
        *id = returnId;
    }
}

/**
 * Read an audio file into a buffer. Set the source of the @c MediaPlayer to the buffer. Playback audio till the end.
 * Check whether the playback started and playback finished notifications are received.
 */
TEST_F(MediaPlayerTest, testStartPlayWaitForEnd) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));
}

/**
 * Set the source of the @c MediaPlayer to a url representing a single audio file. Playback audio till the end.
 * Check whether the playback started and playback finished notifications are received.
 */
TEST_F(MediaPlayerTest, testStartPlayForUrl) {
    std::string url_single(FILE_PREFIX + inputsDirPath + MP3_FILE_PATH);
    auto sourceId = m_mediaPlayer->setSource(url_single);
    ASSERT_NE(ERROR_SOURCE_ID, sourceId);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));
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
    auto id = m_mediaPlayer->setSource(url_single);
    ASSERT_TRUE(m_mediaPlayer->play(id));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(id));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(id));
}

/**
 * Plays a second different type of source after one source has finished playing.
 */
TEST_F(MediaPlayerTest, testStartPlayWaitForEndStartPlayAgain) {
    MediaPlayer::SourceId sourceId;
    setIStreamSource(&sourceId);

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));

    setAttachmentReaderSource(&sourceId);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));
}

/**
 * Read an audio file into a buffer. Set the source of the @c MediaPlayer to the buffer. Playback audio for a few
 * seconds. Playback started notification should be received when the playback starts. Then call @c stop and expect
 * the playback finished notification is received.
 */
TEST_F(MediaPlayerTest, testStopPlay) {
    MediaPlayer::SourceId sourceId;
    setIStreamSource(&sourceId, true);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    std::this_thread::sleep_for(std::chrono::seconds(5));
    ASSERT_TRUE(m_mediaPlayer->stop(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStopped(sourceId));
}

/**
 * Read an audio file into a buffer. Set the source of the @c MediaPlayer to the buffer. Playback audio for a few
 * seconds. Playback started notification should be received when the playback starts. Then call @c stop and expect
 * the playback finished notification is received.
 */
TEST_F(MediaPlayerTest, testStartPlayCallAfterStopPlay) {
    MediaPlayer::SourceId sourceId;
    setIStreamSource(&sourceId, true);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    std::this_thread::sleep_for(std::chrono::seconds(2));
    ASSERT_TRUE(m_mediaPlayer->stop(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStopped(sourceId));
    ASSERT_FALSE(m_mediaPlayer->play(sourceId));
}

/**
 * Read an audio file into a buffer. Set the source of the @c MediaPlayer to the buffer. Playback audio for a few
 * seconds. Playback started notification should be received when the playback starts. Call @c stop.
 * and wait for the playback finished notification is received. Call @c play again.
 */
TEST_F(MediaPlayerTest, testStartPlayCallAfterStopPlayDifferentSource) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(m_mediaPlayer->stop(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStopped(sourceId));

    setAttachmentReaderSource(&sourceId);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(m_mediaPlayer->stop(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStopped(sourceId));
}

/*
 * Pause an audio after playback has started.
 */
TEST_F(MediaPlayerTest, testPauseDuringPlay) {
    MediaPlayer::SourceId sourceId;
    setIStreamSource(&sourceId, true);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(m_mediaPlayer->pause(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused(sourceId));
    ASSERT_EQ(m_playerObserver->getOnPlaybackFinishedCallCount(), 0);
}

/*
 * Resume paused audio.
 */
TEST_F(MediaPlayerTest, testResumeAfterPauseThenStop) {
    MediaPlayer::SourceId sourceId;
    setIStreamSource(&sourceId);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_mediaPlayer->pause(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused(sourceId));
    ASSERT_TRUE(m_mediaPlayer->resume(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackResumed(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));
}

/*
 * Stop of a paused audio after playback has started. An additional stop event should
 * be sent.
 */
TEST_F(MediaPlayerTest, testStopAfterPause) {
    MediaPlayer::SourceId sourceId;
    setIStreamSource(&sourceId);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_TRUE(m_mediaPlayer->pause(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused(sourceId));
    ASSERT_TRUE(m_mediaPlayer->stop(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStopped(sourceId));
}

/*
 * Pause of a paused audio after playback has started. The pause() should fail.
 */
TEST_F(MediaPlayerTest, testPauseAfterPause) {
    MediaPlayer::SourceId sourceId;
    setIStreamSource(&sourceId);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_mediaPlayer->pause(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused(sourceId));
    ASSERT_FALSE(m_mediaPlayer->pause(sourceId));
    ASSERT_TRUE(m_mediaPlayer->stop(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStopped(sourceId));
}

/*
 * Calling resume after playback has started. The resume operation should fail.
 */
TEST_F(MediaPlayerTest, testResumeAfterPlay) {
    MediaPlayer::SourceId sourceId;
    setIStreamSource(&sourceId);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_FALSE(m_mediaPlayer->resume(sourceId));
    ASSERT_TRUE(m_mediaPlayer->stop(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStopped(sourceId));
}

/**
 * Read an audio file into a buffer. Set the source of the @c MediaPlayer to the buffer. Playback audio for a few
 * seconds. Playback started notification should be received when the playback starts. Call @c getOffset.
 * Check the offset value. Then call @c stop and expect the playback finished notification is received.
 * Call @c getOffset again. Check the offset value.
 */
TEST_F(MediaPlayerTest, testGetOffsetInMilliseconds) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::chrono::milliseconds offset = m_mediaPlayer->getOffset(sourceId);
    ASSERT_TRUE((offset > std::chrono::milliseconds::zero()) && (offset <= MP3_FILE_LENGTH));
    ASSERT_NE(MEDIA_PLAYER_INVALID_OFFSET, offset);
    ASSERT_TRUE(m_mediaPlayer->stop(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStopped(sourceId));
    ASSERT_EQ(MEDIA_PLAYER_INVALID_OFFSET, m_mediaPlayer->getOffset(sourceId));
}

/**
 * Test getOffset with a null pipeline. Expect that MEDIA_PLAYER_INVALID_OFFSET is returned.
 * This currently results in errors on shutdown. Will be fixed by ACSDK-446.
 */
TEST_F(MediaPlayerTest, testGetOffsetInMillisecondsNullPipeline) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);
    ASSERT_EQ(MEDIA_PLAYER_INVALID_OFFSET, m_mediaPlayer->getOffset(sourceId + 1));
}

/// Tests that calls to getOffset fail when the pipeline is in a stopped state.
TEST_F(MediaPlayerTest, testGetOffsetWhenStoppedFails) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(m_mediaPlayer->stop(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStopped(sourceId));

    std::chrono::milliseconds offset = m_mediaPlayer->getOffset(sourceId);
    ASSERT_EQ(MEDIA_PLAYER_INVALID_OFFSET, offset);
}

/// Tests that calls to getOffset succeed when the pipeline is in a paused state.
TEST_F(MediaPlayerTest, testGetOffsetWhenPaused) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(m_mediaPlayer->pause(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused(sourceId));

    std::chrono::milliseconds offset = m_mediaPlayer->getOffset(sourceId);
    ASSERT_GE(offset, std::chrono::milliseconds::zero());
    ASSERT_LE(offset, MP3_FILE_LENGTH);
    ASSERT_NE(MEDIA_PLAYER_INVALID_OFFSET, offset);
}

// /**
//  * Check playing two attachments back to back.
//  * Read an audio file into a buffer. Set the source of the @c MediaPlayer to the buffer. Playback audio for a few
//  * seconds. Playback started notification should be received when the playback starts. Call @c getOffset.
//  * Check the offset value. Wait for playback to finish and expect the playback finished notification is received.
//  * Repeat the above for a new source.
//  */
TEST_F(MediaPlayerTest, testPlayingTwoAttachments) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_NE(MEDIA_PLAYER_INVALID_OFFSET, m_mediaPlayer->getOffset(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));

    setAttachmentReaderSource(&sourceId);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_NE(MEDIA_PLAYER_INVALID_OFFSET, m_mediaPlayer->getOffset(sourceId));
    ASSERT_TRUE(m_mediaPlayer->stop(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStopped(sourceId));
}

/**
 * Check playback of an attachment that is received sporadically. Playback started notification should be received
 * when the playback starts. Wait for playback to finish and expect the playback finished notification is received.
 * To a human ear the playback of this test is expected to sound reasonably smooth.
 */
TEST_F(MediaPlayerTest, testUnsteadyReads) {
    // clang-format off
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId,
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
    // clang-format on

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId, std::chrono::milliseconds(15000)));
}

/**
 * Check playback of an attachment whose receipt is interrupted for about 3 seconds.  Playback started notification
 * should be received when the playback starts. Wait for playback to finish and expect the playback finished
 * notification is received. To a human ear the playback of this test is expected to sound reasonably smooth
 * initially, then be interrupted for a few seconds, and then continue fairly smoothly.
 */
TEST_F(MediaPlayerTest, testRecoveryFromPausedReads) {
    MediaPlayer::SourceId sourceId;
    // clang-format off
    setAttachmentReaderSource(&sourceId,
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
    // clang-format on

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId, std::chrono::milliseconds(20000)));
}

/// Tests playing a dummy playlist
TEST_F(MediaPlayerTest, testStartPlayWithUrlPlaylistWaitForEnd) {
    MediaPlayer::SourceId sourceId = m_mediaPlayer->setSource(TEST_M3U_PLAYLIST_URL);
    ASSERT_NE(ERROR_SOURCE_ID, sourceId);
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId, std::chrono::milliseconds(10000)));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId, std::chrono::milliseconds(10000)));
    ASSERT_EQ(m_playerObserver->getOnPlaybackStartedCallCount(), 1);
    ASSERT_EQ(m_playerObserver->getOnPlaybackFinishedCallCount(), 1);
}

// TODO: ACSDK-627 This test fails frequently on Ubuntu Linux platforms.
#ifdef RESOLVED_ACSDK_627
/**
 * Test setting the offset to a seekable source. Setting the offset should succeed and playback should start from the
 * offset.
 */
TEST_F(MediaPlayerTest, testSetOffsetSeekableSource) {
    std::chrono::milliseconds offset(OFFSET);

    std::string url_single(FILE_PREFIX + inputsDirPath + MP3_FILE_PATH);
    auto sourceId = m_mediaPlayer->setSource(url_single);
    ASSERT_NE(ERROR_SOURCE_ID, sourceId);
    ASSERT_TRUE(m_mediaPlayer->setOffset(sourceId, offset));
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    auto start = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));
    ACSDK_INFO(LX("MediaPlayerTest").d("timeElapsed", timeElapsed.count()));
    // Time elapsed should be total file length minus the offset.
    ASSERT_TRUE(timeElapsed < (MP3_FILE_LENGTH - offset + TOLERANCE));
    ASSERT_EQ(m_playerObserver->getOnPlaybackStartedCallCount(), 1);
    ASSERT_EQ(m_playerObserver->getOnPlaybackFinishedCallCount(), 1);

    std::chrono::milliseconds timeElapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
}
#endif

// TODO: ACSDK-1024 MediaPlayerTest.testSetOffsetOutsideBounds is flaky
/**
 * Test setting the offset outside the bounds of the source. Playback will immediately end.
 */
TEST_F(MediaPlayerTest, DISABLED_testSetOffsetOutsideBounds) {
    std::chrono::milliseconds outOfBounds(MP3_FILE_LENGTH + PADDING);

    std::string url_single(FILE_PREFIX + inputsDirPath + MP3_FILE_PATH);
    auto sourceId = m_mediaPlayer->setSource(url_single, outOfBounds);
    ASSERT_NE(ERROR_SOURCE_ID, sourceId);

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackError(sourceId));
}

// TODO: ACSDK-828: this test ends up with a shorter playback time than the actual file length.
/**
 * Test calling setSource resets the offset.
 *
 * Consecutive calls to setSource(const std::string url) without play() cause tests to occasionally fail: ACSDK-508.
 */
TEST_F(MediaPlayerTest, DISABLED_testSetSourceResetsOffset) {
    std::chrono::milliseconds offset(OFFSET);

    std::string url_single(FILE_PREFIX + inputsDirPath + MP3_FILE_PATH);
    auto sourceId = m_mediaPlayer->setSource(url_single, offset);
    ASSERT_NE(ERROR_SOURCE_ID, sourceId);

    sourceId = m_mediaPlayer->setSource(url_single);
    ASSERT_NE(ERROR_SOURCE_ID, sourceId);
    // Play, expect full file.
    auto start = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));

    std::chrono::milliseconds timeElapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    ACSDK_INFO(LX("MediaPlayerTest").d("timeElapsed", timeElapsed.count()));

    // Time elapsed should be the full file.
    ASSERT_TRUE(timeElapsed >= MP3_FILE_LENGTH);
    ASSERT_EQ(m_playerObserver->getOnPlaybackStartedCallCount(), 1);
    ASSERT_EQ(m_playerObserver->getOnPlaybackFinishedCallCount(), 1);
}

/**
 * Test consecutive setSource() and play() calls.  Expect the PlaybackStarted and PlaybackFinished will be received
 * before the timeout.
 */
TEST_F(MediaPlayerTest, testRepeatAttachment) {
    MediaPlayer::SourceId sourceId;
    for (int i = 0; i < 10; i++) {
        setAttachmentReaderSource(&sourceId);
        ASSERT_NE(ERROR_SOURCE_ID, sourceId);
        ASSERT_TRUE(m_mediaPlayer->play(sourceId));
        ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
        ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));
    }
}

/**
 * Test that the media plays after a volume change.
 */
TEST_F(MediaPlayerTest, testSetVolumePlays) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    SpeakerInterface::SpeakerSettings settings;

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));

    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(m_mediaPlayer->pause(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused(sourceId));
    m_mediaPlayer->setVolume(10);
    ASSERT_TRUE(m_mediaPlayer->resume(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));

    m_mediaPlayer->getSpeakerSettings(&settings);
    ASSERT_EQ(settings.volume, 10);
}

/**
 * Test that the media plays after a volume adjustment.
 */
TEST_F(MediaPlayerTest, testAdjustVolumePlaysDuringPlay) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    SpeakerInterface::SpeakerSettings settings;

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));

    m_mediaPlayer->adjustVolume(-90);
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));

    m_mediaPlayer->getSpeakerSettings(&settings);
    ASSERT_EQ(settings.volume, 10);
}

/**
 * Test that the media plays after a volume adjustment.
 */
TEST_F(MediaPlayerTest, testAdjustVolumePlays) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    SpeakerInterface::SpeakerSettings settings;

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));

    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(m_mediaPlayer->pause(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused(sourceId));

    m_mediaPlayer->adjustVolume(-90);
    ASSERT_TRUE(m_mediaPlayer->resume(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));

    m_mediaPlayer->getSpeakerSettings(&settings);
    ASSERT_EQ(settings.volume, 10);
}

/**
 * Test the play behavior when the media is adjusted out of bounds. The volume should be capped
 * at the bounds. The media should play to completion.
 */
TEST_F(MediaPlayerTest, testAdjustVolumeOutOfBounds) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));

    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(m_mediaPlayer->pause(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused(sourceId));

    m_mediaPlayer->setVolume(10);
    m_mediaPlayer->adjustVolume(-100);

    ASSERT_TRUE(m_mediaPlayer->resume(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));

    SpeakerInterface::SpeakerSettings settings;
    m_mediaPlayer->getSpeakerSettings(&settings);
    ASSERT_EQ(settings.volume, 0);
}

/**
 * Test the media plays to completion even if it's muted.
 */
TEST_F(MediaPlayerTest, testSetMutePlays) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    SpeakerInterface::SpeakerSettings settings;

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));

    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(m_mediaPlayer->pause(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused(sourceId));

    m_mediaPlayer->setMute(true);
    ASSERT_TRUE(m_mediaPlayer->resume(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));

    m_mediaPlayer->getSpeakerSettings(&settings);
    ASSERT_TRUE(settings.mute);
}

/**
 * Test that the speaker settings can be retrieved.
 */
TEST_F(MediaPlayerTest, testGetSpeakerSettings) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);
    SpeakerInterface::SpeakerSettings settings;

    m_mediaPlayer->setMute(true);
    m_mediaPlayer->setVolume(15);
    m_mediaPlayer->getSpeakerSettings(&settings);
    ASSERT_TRUE(settings.mute);
    ASSERT_EQ(settings.volume, 15);
}

/**
 * Tests this rounding edgecase. Calling adjustVolume(-10) with volume at 90 resulted in a value of 79
 * when not rounded properly.
 */
TEST_F(MediaPlayerTest, testRoundingEdgeCase) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    SpeakerInterface::SpeakerSettings settings;

    m_mediaPlayer->setVolume(90);
    m_mediaPlayer->adjustVolume(-10);
    m_mediaPlayer->getSpeakerSettings(&settings);
    ASSERT_EQ(settings.volume, 80);
}

/**
 * Read an audio file into a buffer. Set the source of the @c MediaPlayer to the buffer. Playback audio till the end.
 * Check whether the playback started and playback finished notifications are received.
 * Check whether the tags were read from the file.
 */
TEST_F(MediaPlayerTest, testReadTags) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForTags(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));
    /*
     * fox_dog.mp3 returns 3 sets of tags.
     */
    ASSERT_EQ(m_playerObserver->getOnTagsCallCount(), 3);
}

/// Tests that consecutive calls to the same public API fails
TEST_F(MediaPlayerTest, testConsecutiveSameApiCalls) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_FALSE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));

    ASSERT_TRUE(m_mediaPlayer->pause(sourceId));
    ASSERT_FALSE(m_mediaPlayer->pause(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused(sourceId));

    ASSERT_TRUE(m_mediaPlayer->resume(sourceId));
    ASSERT_FALSE(m_mediaPlayer->resume(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackResumed(sourceId));
}

/// Tests that pausing immediately before waiting for a callback is valid.
TEST_F(MediaPlayerTest, testImmediatePause) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_mediaPlayer->pause(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused(sourceId));
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));
}

/// Tests setting multiple set source calls and observing onPlaybackStopped and onPlaybackFinished calls
TEST_F(MediaPlayerTest, multiplePlayAndSetSource) {
    MediaPlayer::SourceId sourceId;
    MediaPlayer::SourceId secondSourceId;
    MediaPlayer::SourceId thirdSourceId;
    setAttachmentReaderSource(&sourceId);

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));

    std::chrono::milliseconds offset = m_mediaPlayer->getOffset(sourceId);
    ASSERT_NE(MEDIA_PLAYER_INVALID_OFFSET, offset);

    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));
    ASSERT_FALSE(m_playerObserver->waitForPlaybackStopped(sourceId));

    setAttachmentReaderSource(&secondSourceId);
    ASSERT_FALSE(m_playerObserver->waitForPlaybackStopped(sourceId));
    ASSERT_TRUE(m_mediaPlayer->play(secondSourceId));

    setAttachmentReaderSource(&thirdSourceId);
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStopped(secondSourceId));
}

/// Tests passing an invalid source id to a play() call
TEST_F(MediaPlayerTest, invalidSourceId) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    ASSERT_FALSE(m_mediaPlayer->play(sourceId + 1));
}

/// Tests that two consecutive calls to pause fails.
TEST_F(MediaPlayerTest, doublePause) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_mediaPlayer->pause(sourceId));
    ASSERT_FALSE(m_mediaPlayer->pause(sourceId));
}

/// Tests that a resume when already playing fails
TEST_F(MediaPlayerTest, resumeWhenPlaying) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_FALSE(m_mediaPlayer->resume(sourceId));
}

/// Tests that a resume when stopped (not paused) fails
TEST_F(MediaPlayerTest, resumeWhenStopped) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_mediaPlayer->stop(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStopped(sourceId));
    ASSERT_FALSE(m_mediaPlayer->resume(sourceId));
}

/// Tests that a stop callback when playing leads to an onPlaybackStopped callback
TEST_F(MediaPlayerTest, newSetSourceLeadsToStoppedCallback) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));

    MediaPlayer::SourceId secondSourceId;
    setAttachmentReaderSource(&secondSourceId);

    ASSERT_TRUE(m_playerObserver->waitForPlaybackStopped(sourceId));
}

/// Tests that resuming after a pause with a pending play leads to an onPlaybackResumed callback.
TEST_F(MediaPlayerTest, resumeAfterPauseWithPendingPlay) {
    MediaPlayer::SourceId sourceId;
    setAttachmentReaderSource(&sourceId);

    /*
     * Set up the situation where a play is followed immediately by a pause.
     * The pause() needs to happen before the onPlaybackStarted call is received to
     * properly test this case. The assumption here is that the play() call should always return
     * before the actual start of audio playback.
     */
    ASSERT_TRUE(m_mediaPlayer->play(sourceId));
    ASSERT_TRUE(m_mediaPlayer->pause(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackStarted(sourceId));
    ASSERT_TRUE(m_playerObserver->waitForPlaybackPaused(sourceId));

    // Expect onPlaybackResumed call.
    m_mediaPlayer->resume(sourceId);
    ASSERT_TRUE(m_playerObserver->waitForPlaybackResumed(sourceId));

    ASSERT_TRUE(m_playerObserver->waitForPlaybackFinished(sourceId));
}

}  // namespace test
}  // namespace mediaPlayer
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (argc < 2) {
        std::cerr << "Usage: " << std::string(argv[0]) << " <absolute path to test inputs folder>" << std::endl;
    } else {
        alexaClientSDK::mediaPlayer::test::inputsDirPath = std::string(argv[1]);
        alexaClientSDK::mediaPlayer::test::urlsToContentTypes.insert(
            {alexaClientSDK::mediaPlayer::test::FILE_PREFIX + alexaClientSDK::mediaPlayer::test::inputsDirPath +
                 alexaClientSDK::mediaPlayer::test::MP3_FILE_PATH,
             "audio/mpeg"});
        std::ifstream fileStream(
            alexaClientSDK::mediaPlayer::test::inputsDirPath + alexaClientSDK::mediaPlayer::test::MP3_FILE_PATH);
        std::stringstream fileData;
        fileData << fileStream.rdbuf();
        alexaClientSDK::mediaPlayer::test::urlsToContent.insert({alexaClientSDK::mediaPlayer::test::FILE_PREFIX +
                                                                     alexaClientSDK::mediaPlayer::test::inputsDirPath +
                                                                     alexaClientSDK::mediaPlayer::test::MP3_FILE_PATH,
                                                                 fileData.str()});
        alexaClientSDK::mediaPlayer::test::urlsToContentTypes.insert(
            {alexaClientSDK::mediaPlayer::test::TEST_M3U_PLAYLIST_URL, "audio/mpegurl"});
        alexaClientSDK::mediaPlayer::test::TEST_M3U_PLAYLIST_CONTENT =
            std::string("EXTINF:2,fox_dog.mp3\n") +
            std::string(
                alexaClientSDK::mediaPlayer::test::FILE_PREFIX + alexaClientSDK::mediaPlayer::test::inputsDirPath +
                alexaClientSDK::mediaPlayer::test::MP3_FILE_PATH) +
            '\n' + std::string("EXTINF:2,fox_dog.mp3\n") +
            std::string(
                alexaClientSDK::mediaPlayer::test::FILE_PREFIX + alexaClientSDK::mediaPlayer::test::inputsDirPath +
                alexaClientSDK::mediaPlayer::test::MP3_FILE_PATH) +
            '\n';
        alexaClientSDK::mediaPlayer::test::urlsToContent.insert(
            {alexaClientSDK::mediaPlayer::test::TEST_M3U_PLAYLIST_URL,
             alexaClientSDK::mediaPlayer::test::TEST_M3U_PLAYLIST_CONTENT});
// ACSDK-1141 - Some tests fail on Windows.
#if defined(_WIN32) && !defined(RESOLVED_ACSDK_1141)
        ::testing::GTEST_FLAG(filter) = "-MediaPlayerTest*";
#endif
        return RUN_ALL_TESTS();
    }
}
