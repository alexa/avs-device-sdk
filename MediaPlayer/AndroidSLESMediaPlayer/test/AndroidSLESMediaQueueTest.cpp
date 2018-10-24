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

#include <fstream>
#include <thread>
#include <tuple>

#include <SLES/OpenSLES_Android.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/AVS/Attachment/AttachmentWriter.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachment.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AndroidUtilities/AndroidLogger.h>
#include <AndroidUtilities/MockAndroidSLESObject.h>

#include "AndroidSLESMediaPlayer/AndroidSLESMediaQueue.h"

/// String to identify log entries originating from this file.
static const std::string TAG{"AndroidSLESMediaQueueTest"};

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
using namespace applicationUtilities::androidUtilities;
using namespace applicationUtilities::androidUtilities::test;
using namespace avsCommon::avs;

using MockSlSimpleBufferQueue = MockInterfaceImpl<SLAndroidSimpleBufferQueueItf_>;

/// The path to the input Dir containing the test audio files.
static std::string inputsDirPath;

/// MP3 test file path.
static const std::string MP3_FILE_PATH("/fox_dog.mp3");

/// Timeout for wait callback
static const std::chrono::milliseconds TIMEOUT{100};

/// Number of buffers used by the buffer queue.
static const auto NUMBER_OF_BUFFERS = AndroidSLESMediaQueue::NUMBER_OF_BUFFERS;

/// Mock buffer count.
static std::atomic<int> counter;

/// Mock buffer index.
static int index;

/// Mutex used to synchronize the enqueueBuffer and onBufferFree
static std::mutex bufferMutex;

/// Condition variable used to guarantee that onBufferFree only happens after counter > 0;
static std::condition_variable bufferCondition;

/// Mock decoder.
class MockDecoder : public DecoderInterface {
public:
    /// Mock read call.
    MOCK_METHOD2(read, std::pair<Status, size_t>(Byte*, size_t));

    /// Mock abort call.
    MOCK_METHOD0(abort, void());
};

/// Mock status callback.
class MockCallback {
public:
    /// Method that expectations should be based on.
    void callback(AndroidSLESMediaQueue::QueueEvent event, const std::string& reason);

    /// Method used to wait for the callback call. It returns the status it received.
    bool waitCallback(const AndroidSLESMediaQueue::QueueEvent& expectedEvent);

private:
    /// Mutex to synchronize calls.
    std::mutex m_mutex;

    /// Condition variable used to wake-up callback.
    std::condition_variable m_condition;

    /// The queue status received;
    AndroidSLESMediaQueue::QueueEvent m_event{AndroidSLESMediaQueue::QueueEvent::ERROR};
};

void MockCallback::callback(AndroidSLESMediaQueue::QueueEvent event, const std::string& reason) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_event = event;
    m_condition.notify_one();
}

bool MockCallback::waitCallback(const AndroidSLESMediaQueue::QueueEvent& expectedEvent) {
    std::unique_lock<std::mutex> lock{m_mutex};
    return m_condition.wait_for(lock, TIMEOUT, [this, &expectedEvent] { return m_event == expectedEvent; });
}

/// Mock register callback succeeds.
static SLresult mockRegisterCallbackOk(
    SLAndroidSimpleBufferQueueItf self,
    slAndroidSimpleBufferQueueCallback callback,
    void* pCallbackContext) {
    return SL_RESULT_SUCCESS;
}

/// Mock register callback fails.
static SLresult mockRegisterCallbackFailed(
    SLAndroidSimpleBufferQueueItf self,
    slAndroidSimpleBufferQueueCallback callback,
    void* pCallbackContext) {
    return SL_RESULT_INTERNAL_ERROR;
}

/// Mock enqueue call succeeds.
static SLresult mockEnqueueOk(SLAndroidSimpleBufferQueueItf self, const void* pBuffer, SLuint32 size) {
    std::lock_guard<std::mutex> lock{bufferMutex};
    bufferCondition.notify_one();
    counter++;
    return SL_RESULT_SUCCESS;
}

/// Mock enqueue call fails.
SLresult mockEnqueueFailed(SLAndroidSimpleBufferQueueItf self, const void* pBuffer, SLuint32 size) {
    return SL_RESULT_INTERNAL_ERROR;
}

/// Mock get state.
SLresult mockGetState(SLAndroidSimpleBufferQueueItf self, SLAndroidSimpleBufferQueueState* pState) {
    pState->count = counter;
    pState->index = index;
    return SL_RESULT_SUCCESS;
}

/// Mock clear call succeeds.
static SLresult mockClearOk(SLAndroidSimpleBufferQueueItf self) {
    return SL_RESULT_SUCCESS;
}

/// Mock buffer free call. Increment index and decrement counter.
static void bufferFree(AndroidSLESMediaQueue& mediaQueue) {
    if (!counter) {
        std::unique_lock<std::mutex> lock{bufferMutex};
        bufferCondition.wait_for(lock, TIMEOUT);
    }
    counter--;
    index++;
    mediaQueue.onBufferFree();
}

class AndroidSLESMediaQueueTest : public Test {
protected:
    void SetUp() override;

    void TearDown() override;

    /// Mock the OpenSL ES buffer queue object wrapper.
    std::shared_ptr<MockAndroidSLESObject> m_objectMock;

    /// Mock the actuall OpenSL ES buffer queue object.
    std::shared_ptr<AndroidSLESObject> m_slObject;

    /// Mock the OpenSL ES buffer queue interface.
    std::shared_ptr<MockSlSimpleBufferQueue> m_queueMock;

    /// The status callback mock.
    MockCallback m_callback;
};

void AndroidSLESMediaQueueTest::SetUp() {
    counter = 0;
    index = 0;

    m_objectMock = std::make_shared<MockAndroidSLESObject>();
    m_queueMock = std::make_shared<MockSlSimpleBufferQueue>();
    m_slObject = AndroidSLESObject::create(m_objectMock->getObject());
    m_objectMock->mockGetInterface(SL_IID_ANDROIDSIMPLEBUFFERQUEUE, m_queueMock);
    m_queueMock->get().RegisterCallback = mockRegisterCallbackOk;
    m_queueMock->get().Enqueue = mockEnqueueOk;
    m_queueMock->get().GetState = mockGetState;
    m_queueMock->get().Clear = mockClearOk;
}

void AndroidSLESMediaQueueTest::TearDown() {
    m_queueMock.reset();
    m_objectMock.reset();
}

/// Test buffer queue create.
TEST_F(AndroidSLESMediaQueueTest, testCreateSucceed) {
    std::pair<DecoderInterface::Status, size_t> done = {DecoderInterface::Status::DONE, 0};
    auto decoder = avsCommon::utils::memory::make_unique<NiceMock<MockDecoder>>();
    EXPECT_CALL(*decoder, read(_, _)).WillOnce(Return(done));

    auto mediaQueue = AndroidSLESMediaQueue::create(
        m_slObject,
        std::move(decoder),
        [this](AndroidSLESMediaQueue::QueueEvent queueStatus, const std::string& reason) {
            m_callback.callback(queueStatus, reason);
        },
        PlaybackConfiguration());
    EXPECT_NE(mediaQueue, nullptr);
}

/// Check that create fail if we are missing the OpenSL ES object.
TEST_F(AndroidSLESMediaQueueTest, testCreateFailMissingSlObject) {
    auto decoder = avsCommon::utils::memory::make_unique<NiceMock<MockDecoder>>();
    auto mediaQueue = AndroidSLESMediaQueue::create(
        nullptr,
        std::move(decoder),
        [this](AndroidSLESMediaQueue::QueueEvent queueStatus, const std::string& reason) {
            m_callback.callback(queueStatus, reason);
        },
        PlaybackConfiguration());
    EXPECT_EQ(mediaQueue, nullptr);
}

/// Check that create fail if the @c SL_IID_ANDROIDSIMPLEBUFFERQUEUE is missing.
TEST_F(AndroidSLESMediaQueueTest, testCreateFailMissingInterface) {
    m_objectMock->mockGetInterface(SL_IID_ANDROIDSIMPLEBUFFERQUEUE, nullptr);
    auto decoder = avsCommon::utils::memory::make_unique<NiceMock<MockDecoder>>();
    auto mediaQueue = AndroidSLESMediaQueue::create(
        m_slObject,
        std::move(decoder),
        [this](AndroidSLESMediaQueue::QueueEvent queueStatus, const std::string& reason) {
            m_callback.callback(queueStatus, reason);
        },
        PlaybackConfiguration());
    EXPECT_EQ(mediaQueue, nullptr);
}

/// Check that create fail if the decoder is missing.
TEST_F(AndroidSLESMediaQueueTest, testCreateFailMissingDecoder) {
    auto mediaQueue = AndroidSLESMediaQueue::create(
        m_slObject,
        nullptr,
        [this](AndroidSLESMediaQueue::QueueEvent queueStatus, const std::string& reason) {
            m_callback.callback(queueStatus, reason);
        },
        PlaybackConfiguration());
    EXPECT_EQ(mediaQueue, nullptr);
}

/// Check that create fail if the callback function is missing.
TEST_F(AndroidSLESMediaQueueTest, testCreateFailMissingCallback) {
    auto decoder = avsCommon::utils::memory::make_unique<NiceMock<MockDecoder>>();
    auto mediaQueue = AndroidSLESMediaQueue::create(m_slObject, std::move(decoder), nullptr, PlaybackConfiguration());
    EXPECT_EQ(mediaQueue, nullptr);
}

/// Check that create fail if the callback function cannot be registered.
TEST_F(AndroidSLESMediaQueueTest, testCreateFailRegisterCallback) {
    m_queueMock->get().RegisterCallback = mockRegisterCallbackFailed;
    auto decoder = avsCommon::utils::memory::make_unique<NiceMock<MockDecoder>>();
    auto mediaQueue = AndroidSLESMediaQueue::create(m_slObject, std::move(decoder), nullptr, PlaybackConfiguration());
    EXPECT_EQ(mediaQueue, nullptr);
}

/// Test buffer queue events when the media queue succeeds to read data from the decoder.
TEST_F(AndroidSLESMediaQueueTest, testOnBufferQueueSucceed) {
    // Always return valid read.
    // Arbitrary number of bytes that is > 0.
    constexpr size_t bytesRead{1000};
    std::pair<DecoderInterface::Status, size_t> ok = {DecoderInterface::Status::OK, bytesRead};
    auto decoder = avsCommon::utils::memory::make_unique<NiceMock<MockDecoder>>();
    EXPECT_CALL(*decoder, read(_, _)).WillRepeatedly(Return(ok));

    auto mediaQueue = AndroidSLESMediaQueue::create(
        m_slObject,
        std::move(decoder),
        [this](AndroidSLESMediaQueue::QueueEvent queueStatus, const std::string& reason) {
            m_callback.callback(queueStatus, reason);
        },
        PlaybackConfiguration());
    EXPECT_NE(mediaQueue, nullptr);

    // This first buffer free is for the silence buffer workaround.
    bufferFree(*mediaQueue);

    bufferFree(*mediaQueue);
    size_t buffersPlayed = 1;
    EXPECT_FALSE(m_callback.waitCallback(AndroidSLESMediaQueue::QueueEvent::FINISHED_PLAYING));
    EXPECT_EQ(counter, static_cast<int>(NUMBER_OF_BUFFERS));
    EXPECT_EQ(mediaQueue->getNumBytesBuffered(), NUMBER_OF_BUFFERS * bytesRead);
    EXPECT_EQ(mediaQueue->getNumBytesPlayed(), buffersPlayed * bytesRead);
}

/// Test buffer queue events when the media queue succeeds to read data from the decoder till the end of the stream.
TEST_F(AndroidSLESMediaQueueTest, testEnqueueTillDone) {
    // Arbitrary number of bytes that is > 0.
    constexpr size_t bytesRead{1000};
    std::pair<DecoderInterface::Status, size_t> ok = {DecoderInterface::Status::OK, bytesRead};
    std::pair<DecoderInterface::Status, size_t> done = {DecoderInterface::Status::DONE, 0};

    // Return valid read for NUMBER_OF_BUFFERS times, then return done.
    auto decoder = avsCommon::utils::memory::make_unique<NiceMock<MockDecoder>>();
    EXPECT_CALL(*decoder, read(_, _)).WillOnce(Return(done));
    EXPECT_CALL(*decoder, read(_, _))
        .Times(AndroidSLESMediaQueue::NUMBER_OF_BUFFERS)
        .WillRepeatedly(Return(ok))
        .RetiresOnSaturation();

    auto mediaQueue = AndroidSLESMediaQueue::create(
        m_slObject,
        std::move(decoder),
        [this](AndroidSLESMediaQueue::QueueEvent queueStatus, const std::string& reason) {
            m_callback.callback(queueStatus, reason);
        },
        PlaybackConfiguration());
    EXPECT_NE(mediaQueue, nullptr);

    // This first buffer free is for the silence buffer workaround.
    bufferFree(*mediaQueue);

    for (size_t i = 0; i <= AndroidSLESMediaQueue::NUMBER_OF_BUFFERS; ++i) {
        bufferFree(*mediaQueue);
    }
    EXPECT_TRUE(m_callback.waitCallback(AndroidSLESMediaQueue::QueueEvent::FINISHED_PLAYING));
    EXPECT_EQ(mediaQueue->getNumBytesPlayed(), AndroidSLESMediaQueue::NUMBER_OF_BUFFERS * bytesRead);
    EXPECT_EQ(mediaQueue->getNumBytesBuffered(), 0u);
}

/// Test that the buffer queue emits an error event when the decoder fails.
TEST_F(AndroidSLESMediaQueueTest, testDecoderFailure) {
    std::pair<DecoderInterface::Status, size_t> error = {DecoderInterface::Status::ERROR, 0};
    auto decoder = avsCommon::utils::memory::make_unique<NiceMock<MockDecoder>>();
    EXPECT_CALL(*decoder, read(_, _)).WillOnce(Return(error));

    auto mediaQueue = AndroidSLESMediaQueue::create(
        m_slObject,
        std::move(decoder),
        [this](AndroidSLESMediaQueue::QueueEvent queueStatus, const std::string& reason) {
            m_callback.callback(queueStatus, reason);
        },
        PlaybackConfiguration());
    EXPECT_NE(mediaQueue, nullptr);
    EXPECT_TRUE(m_callback.waitCallback(AndroidSLESMediaQueue::QueueEvent::ERROR));
}

/// Test buffer queue events emits an error event when it fails to enqueue a buffer
TEST_F(AndroidSLESMediaQueueTest, testEnqueueFailure) {
    // Always return valid read.
    // Arbitrary number of bytes that is > 0.
    constexpr size_t bytesRead{1000};
    std::pair<DecoderInterface::Status, size_t> ok = {DecoderInterface::Status::OK, bytesRead};
    auto decoder = avsCommon::utils::memory::make_unique<NiceMock<MockDecoder>>();
    EXPECT_CALL(*decoder, read(_, _)).WillRepeatedly(Return(ok));

    auto mediaQueue = AndroidSLESMediaQueue::create(
        m_slObject,
        std::move(decoder),
        [this](AndroidSLESMediaQueue::QueueEvent queueStatus, const std::string& reason) {
            m_callback.callback(queueStatus, reason);
        },
        PlaybackConfiguration());
    EXPECT_NE(mediaQueue, nullptr);

    m_queueMock->get().Enqueue = mockEnqueueFailed;
    bufferFree(*mediaQueue);
    EXPECT_TRUE(m_callback.waitCallback(AndroidSLESMediaQueue::QueueEvent::ERROR));
}

}  // namespace test
}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
