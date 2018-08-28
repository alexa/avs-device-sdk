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

#include <AndroidUtilities/AndroidSLESBufferQueue.h>
#include <AVSCommon/AVS/AudioInputStream.h>

#include <AndroidUtilities/MockAndroidSLESObject.h>
#include <AndroidUtilities/MockAndroidSLESInterface.h>

using namespace ::testing;

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {
namespace test {

/// The sample rate of microphone audio data.
static constexpr size_t SAMPLE_RATE_HZ{16000};

/// The amount of audio data to keep in the ring buffer.
static constexpr std::chrono::seconds AMOUNT_OF_AUDIO_DATA_IN_BUFFER{10};

/// The number of buffers used by the @c AndroidSLESBufferQueue object. Redefine it here because of the ODR (one
/// definition rule) use by EXPECT.
static const auto NUMBER_OF_BUFFERS = AndroidSLESBufferQueue::NUMBER_OF_BUFFERS;

/// The size of the ring buffer.
static constexpr size_t SDS_BUFFER_SIZE = (SAMPLE_RATE_HZ)*AMOUNT_OF_AUDIO_DATA_IN_BUFFER.count();

using MockSlSimpleBufferQueue = MockInterfaceImpl<SLAndroidSimpleBufferQueueItf_>;

class AndroidSLESBufferQueueTest : public Test {
public:
    /// Mock buffer count.
    static SLuint32 m_count;

    /// Mock buffer index.
    static SLuint32 m_index;

protected:
    /**
     * Reset all shared pointers.
     */
    virtual void TearDown();

    /**
     * Setup all objects.
     */
    virtual void SetUp();

    /**
     * Create the buffer under test.
     * @return A pointer to the buffer.
     */
    std::unique_ptr<AndroidSLESBufferQueue> createBuffer();

    /// Pointer to the audio input stream.
    std::shared_ptr<avsCommon::avs::AudioInputStream> m_stream;

    /// Pointer to an audio input stream reader used to check writting results.
    std::shared_ptr<avsCommon::avs::AudioInputStream::Reader> m_reader;

    /// Mock the openSlObject.
    std::shared_ptr<MockAndroidSLESObject> m_recorderMock;

    /// Mock sl simple queue.
    std::shared_ptr<MockSlSimpleBufferQueue> m_queueMock;
};

SLuint32 AndroidSLESBufferQueueTest::m_count{0};
SLuint32 AndroidSLESBufferQueueTest::m_index{0};

static SLresult mockRegisterCallbackOk(
    SLAndroidSimpleBufferQueueItf self,
    slAndroidSimpleBufferQueueCallback callback,
    void* pCallbackContext) {
    return SL_RESULT_SUCCESS;
}

static SLresult mockRegisterCallbackFailure(
    SLAndroidSimpleBufferQueueItf self,
    slAndroidSimpleBufferQueueCallback callback,
    void* pCallbackContext) {
    return SL_RESULT_INTERNAL_ERROR;
}

static SLresult mockClear(SLAndroidSimpleBufferQueueItf self) {
    return SL_RESULT_SUCCESS;
}

/**
 * Mock enqueue success.
 *
 * @param self ignored.
 * @param pBuffer ignored.
 * @param size ignored.
 * @return SL_RESULT_SUCCESS.
 */
SLresult mockEnqueue(SLAndroidSimpleBufferQueueItf self, const void* pBuffer, SLuint32 size) {
    AndroidSLESBufferQueueTest::m_count++;
    return SL_RESULT_SUCCESS;
}

/**
 * Mock enqueue failure.
 *
 * @param self ignored.
 * @param pBuffer ignored.
 * @param size ignored.
 * @return Internal error.
 */
SLresult mockEnqueueFailed(SLAndroidSimpleBufferQueueItf self, const void* pBuffer, SLuint32 size) {
    return SL_RESULT_INTERNAL_ERROR;
}

/**
 * Mock partial enqueue.
 *
 * @param self ignored.
 * @param pBuffer ignored.
 * @param size ignored.
 * @return Success for half of the buffers, and false after.
 */
SLresult mockEnqueueHalf(SLAndroidSimpleBufferQueueItf self, const void* pBuffer, SLuint32 size) {
    if (AndroidSLESBufferQueueTest::m_count < (NUMBER_OF_BUFFERS / 2)) {
        return mockEnqueue(self, pBuffer, size);
    }
    return SL_RESULT_INTERNAL_ERROR;
}

SLresult mockGetState(SLAndroidSimpleBufferQueueItf self, SLAndroidSimpleBufferQueueState* pState) {
    pState->count = AndroidSLESBufferQueueTest::m_count;
    pState->index = AndroidSLESBufferQueueTest::m_index;
    return SL_RESULT_SUCCESS;
}

void AndroidSLESBufferQueueTest::TearDown() {
    m_recorderMock.reset();
    m_reader.reset();
    m_stream.reset();
    m_queueMock.reset();
}

void AndroidSLESBufferQueueTest::SetUp() {
    auto buffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>(SDS_BUFFER_SIZE);
    m_stream = avsCommon::avs::AudioInputStream::create(buffer);
    m_reader = m_stream->createReader(avsCommon::avs::AudioInputStream::Reader::Policy::BLOCKING);
    m_recorderMock = std::make_shared<MockAndroidSLESObject>();
    m_queueMock = std::make_shared<MockSlSimpleBufferQueue>();
    m_queueMock->get().Clear = mockClear;
    m_recorderMock->mockGetInterface(SL_IID_ANDROIDSIMPLEBUFFERQUEUE, m_queueMock);
    m_count = 0;
    m_index = 0;
}

std::unique_ptr<AndroidSLESBufferQueue> AndroidSLESBufferQueueTest::createBuffer() {
    m_queueMock->get().RegisterCallback = mockRegisterCallbackOk;
    std::shared_ptr<AndroidSLESObject> slObject = AndroidSLESObject::create(m_recorderMock->getObject());
    return AndroidSLESBufferQueue::create(
        slObject, m_stream->createWriter(avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE));
}

/**
 * Test successful creation.
 */
TEST_F(AndroidSLESBufferQueueTest, TestRegisterCallbackSucceeded) {
    auto buffer = createBuffer();
    EXPECT_NE(buffer, nullptr);
}

/**
 * Test the callback register failure.
 */
TEST_F(AndroidSLESBufferQueueTest, TestRegisterCallbackFailed) {
    m_queueMock->get().RegisterCallback = mockRegisterCallbackFailure;
    std::shared_ptr<AndroidSLESObject> slObject = AndroidSLESObject::create(m_recorderMock->getObject());
    auto buffer = AndroidSLESBufferQueue::create(
        slObject, m_stream->createWriter(avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE));
    EXPECT_EQ(buffer, nullptr);
}

/**
 * Test enqueue succeeded.
 */
TEST_F(AndroidSLESBufferQueueTest, TestEnqueueOK) {
    m_queueMock->get().Enqueue = mockEnqueue;
    m_queueMock->get().GetState = mockGetState;
    auto buffer = createBuffer();
    EXPECT_NE(buffer, nullptr);

    EXPECT_EQ(buffer->enqueueBuffers(), true);
    EXPECT_EQ(AndroidSLESBufferQueueTest::m_count, NUMBER_OF_BUFFERS);
}

/**
 * Test enqueue failed.
 */
TEST_F(AndroidSLESBufferQueueTest, TestEnqueueFailed) {
    m_queueMock->get().Enqueue = mockEnqueueFailed;
    m_queueMock->get().GetState = mockGetState;
    auto buffer = createBuffer();
    EXPECT_NE(buffer, nullptr);

    EXPECT_EQ(buffer->enqueueBuffers(), false);
}

/**
 * Test enqueue succeeded to enqueue a few buffers.
 */
TEST_F(AndroidSLESBufferQueueTest, TestEnqueuePartial) {
    m_queueMock->get().Enqueue = mockEnqueueHalf;
    m_queueMock->get().GetState = mockGetState;
    auto buffer = createBuffer();
    EXPECT_NE(buffer, nullptr);

    EXPECT_EQ(buffer->enqueueBuffers(), true);
    EXPECT_EQ(AndroidSLESBufferQueueTest::m_count, (NUMBER_OF_BUFFERS / 2));
}

/**
 * Test onBufferCompleted.
 */
TEST_F(AndroidSLESBufferQueueTest, TestOnBufferCompleted) {
    AndroidSLESBufferQueueTest::m_count = NUMBER_OF_BUFFERS - 1;
    m_queueMock->get().Enqueue = mockEnqueue;
    auto buffer = createBuffer();
    EXPECT_NE(buffer, nullptr);

    const std::chrono::milliseconds timeout{200};  // read timeout
    constexpr size_t size{4096u};                  // Buffer size that should be filled with data after one read.

    int16_t data[size];
    buffer->onBufferCompleted();
    EXPECT_EQ(AndroidSLESBufferQueueTest::m_count, NUMBER_OF_BUFFERS);
    EXPECT_EQ(m_reader->read(data, size, timeout), static_cast<ssize_t>(size));
}

}  // namespace test
}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
