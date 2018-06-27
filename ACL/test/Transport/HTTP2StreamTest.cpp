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

/// @file HTTP2StreamTest.cpp

#include <memory>
#include <random>

#include <gtest/gtest.h>

#include <curl/curl.h>
#include <ACL/Transport/HTTP2Stream.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/SDS/InProcessSDS.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachmentReader.h>
#include "TestableConsumer.h"
#include "MockMessageRequest.h"
#include "Common/Common.h"

namespace alexaClientSDK {
namespace acl {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils::sds;
using namespace alexaClientSDK::avsCommon::avs::initialization;

/// A test url with which to initialize our test stream object.
static const std::string LIBCURL_TEST_URL = "http://example.com";
/// A test auth string with which to initialize our test stream object.
static const std::string LIBCURL_TEST_AUTH_STRING = "test_auth_string";
/// The length of the string we will test for the exception message.
static const int TEST_EXCEPTION_STRING_LENGTH = 200;
/// The number of iterations the multi-write test will perform.
static const int TEST_EXCEPTION_PARTITIONS = 7;
/// The maximum length of the exception message allowed.  Must be same as EXCEPTION_MESSAGE_MAX_SIZE.
static const size_t TEST_EXCEPTION_STRING_MAX_SIZE = 4096;
/// The length of the string we will test for the exception message that exceeded the maximum length.
static const size_t TEST_EXCEPTION_STRING_EXCEED_MAX_LENGTH = TEST_EXCEPTION_STRING_MAX_SIZE + 1024;
/// Number of bytes per word in the SDS circular buffer.
static const size_t SDS_WORDSIZE = 1;
/// Maximum number of readers to support in the SDS circular buffer.
static const size_t SDS_MAXREADERS = 1;
/// Number of words to hold in the SDS circular buffer.
static const size_t SDS_WORDS = 300;
/// Number of strings to read/write for the test
static const size_t NUMBER_OF_STRINGS = 1;

/// The field name for the user voice attachment
static const std::string AUDIO_ATTACHMENT_FIELD_NAME = "audio";

/// The field name for the wake word engine metadata
static const std::string KWD_METADATA_ATTACHMENT_FIELD_NAME = "WakwWordEngineMetadata";

/**
 * Our GTest class.
 */
class HTTP2StreamTest : public ::testing::Test {
public:
    void SetUp() override;

    void TearDown() override;

    /// A message request to initiate @c m_readTestableStream with
    std::shared_ptr<MessageRequest> m_MessageRequest;
    /// A mock message request object.
    std::shared_ptr<MockMessageRequest> m_mockMessageRequest;
    /// An object that is required for constructing a stream.
    std::shared_ptr<TestableConsumer> m_testableConsumer;
    /// The actual stream we will be testing.
    std::shared_ptr<HTTP2Stream> m_testableStream;
    /// The stream to test @c readCallback function
    std::shared_ptr<HTTP2Stream> m_readTestableStream;
    /// The attachment manager for the stream
    std::shared_ptr<AttachmentManager> m_attachmentManager;
    /// A Writer to write data to SDS buffer.
    std::unique_ptr<avsCommon::utils::sds::InProcessSDS::Writer> m_writer;
    /// The attachment reader for message request of @c m_readTestableStream
    std::shared_ptr<InProcessAttachmentReader> m_attachmentReader;
    /// A char pointer to data buffer to read or write from callbacks
    char* m_dataBegin;
    /// A string to which @c m_dataBegin is pointing to
    std::string m_testString;
};

void HTTP2StreamTest::SetUp() {
    AlexaClientSDKInit::initialize(std::vector<std::shared_ptr<std::istream>>());
    m_testableConsumer = std::make_shared<TestableConsumer>();

    m_testString = createRandomAlphabetString(TEST_EXCEPTION_STRING_LENGTH);
    m_dataBegin = const_cast<char*>(m_testString.c_str());

    /// Create a SDS buffer and using a @c Writer, write a string into the buffer
    size_t bufferSize = InProcessSDS::calculateBufferSize(SDS_WORDS, SDS_WORDSIZE, SDS_MAXREADERS);
    auto buffer = std::make_shared<avsCommon::utils::sds::InProcessSDS::Buffer>(bufferSize);
    std::shared_ptr<InProcessSDS> stream = InProcessSDS::create(buffer, SDS_WORDSIZE, SDS_MAXREADERS);
    ASSERT_NE(stream, nullptr);

    m_writer = stream->createWriter(InProcessSDS::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(m_writer, nullptr);
    ASSERT_EQ(TEST_EXCEPTION_STRING_LENGTH, m_writer->write(m_dataBegin, TEST_EXCEPTION_STRING_LENGTH));

    /// Create an attachment Reader for @c m_MessageRequest
    m_attachmentReader = InProcessAttachmentReader::create(InProcessSDS::Reader::Policy::NONBLOCKING, stream);
    m_MessageRequest = std::make_shared<MessageRequest>("");
    m_MessageRequest->addAttachmentReader(AUDIO_ATTACHMENT_FIELD_NAME, m_attachmentReader);
    m_MessageRequest->addAttachmentReader(KWD_METADATA_ATTACHMENT_FIELD_NAME, m_attachmentReader);

    ASSERT_NE(m_MessageRequest, nullptr);

    m_mockMessageRequest = std::make_shared<MockMessageRequest>();
    ASSERT_NE(m_mockMessageRequest, nullptr);
    m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);

    m_testableStream = std::make_shared<HTTP2Stream>(m_testableConsumer, m_attachmentManager);
    ASSERT_NE(m_testableStream, nullptr);
    ASSERT_TRUE(m_testableStream->initPost(LIBCURL_TEST_URL, LIBCURL_TEST_AUTH_STRING, m_mockMessageRequest));

    m_readTestableStream = std::make_shared<HTTP2Stream>(m_testableConsumer, m_attachmentManager);
    ASSERT_NE(m_readTestableStream, nullptr);
    ASSERT_TRUE(m_readTestableStream->initPost(LIBCURL_TEST_URL, LIBCURL_TEST_AUTH_STRING, m_MessageRequest));
}

void HTTP2StreamTest::TearDown() {
    AlexaClientSDKInit::uninitialize();
}
/**
 * Let's simulate that AVSConnectionManager->send() has been invoked, and the messageRequest object is
 * waiting to be notified on the response from AVS.  We will invoke the stream writeCallbacks directly to
 * simulate exception data returning from AVS, and verify that the stream passes the correct data back to
 * the request object.
 */
TEST_F(HTTP2StreamTest, testExceptionReceivedSingleWrite) {
    HTTP2Stream::writeCallback(m_dataBegin, TEST_EXCEPTION_STRING_LENGTH, NUMBER_OF_STRINGS, m_testableStream.get());

    EXPECT_CALL(*m_mockMessageRequest, exceptionReceived(_)).Times(1);
    EXPECT_CALL(*m_mockMessageRequest, sendCompleted(_)).Times(1);
    // This simulates stream cleanup, which flushes out the parsed exception message.
    m_testableStream->notifyRequestObserver();
}

/**
 * The same test as above, but now with multiple writes (simulating either a small buffer from libcurl, or a very
 * long exception message).
 */
TEST_F(HTTP2StreamTest, testExceptionReceivedMultiWrite) {
    int writeQuantum = TEST_EXCEPTION_STRING_LENGTH;
    if (TEST_EXCEPTION_PARTITIONS > 0) {
        writeQuantum = TEST_EXCEPTION_STRING_LENGTH / TEST_EXCEPTION_PARTITIONS;
    }
    int numberBytesWritten = 0;

    char* currBuffPosition = m_dataBegin;
    while (numberBytesWritten < TEST_EXCEPTION_STRING_LENGTH) {
        int bytesRemaining = TEST_EXCEPTION_STRING_LENGTH - numberBytesWritten;
        int bytesToWrite = bytesRemaining < writeQuantum ? bytesRemaining : writeQuantum;

        HTTP2Stream::writeCallback(currBuffPosition, bytesToWrite, NUMBER_OF_STRINGS, m_testableStream.get());
        currBuffPosition += bytesToWrite;
        numberBytesWritten += bytesToWrite;
    }

    EXPECT_CALL(*m_mockMessageRequest, exceptionReceived(_)).Times(1);
    EXPECT_CALL(*m_mockMessageRequest, sendCompleted(_)).Times(1);

    // This simulates stream cleanup, which flushes out the parsed exception message.
    m_testableStream->notifyRequestObserver();
}

/**
 * We will invoke the stream writeCallbacks directly to simulate exception data that exceeded maximum length allowed
 * returning from AVS, and verify that the stream passes up the correct data up to the maximum length back to the
 * request object.
 */
TEST_F(HTTP2StreamTest, testExceptionExceededMaximum) {
    HTTP2Stream::writeCallback(
        m_dataBegin, TEST_EXCEPTION_STRING_EXCEED_MAX_LENGTH, NUMBER_OF_STRINGS, m_testableStream.get());

    EXPECT_CALL(*m_mockMessageRequest, exceptionReceived(_)).WillOnce(Invoke([](const std::string& exceptionMessage) {
        EXPECT_EQ(exceptionMessage.size(), TEST_EXCEPTION_STRING_MAX_SIZE);
    }));
    EXPECT_CALL(*m_mockMessageRequest, sendCompleted(_)).Times(1);
    // This simulates stream cleanup, which flushes out the parsed exception message.
    m_testableStream->notifyRequestObserver();
}

TEST_F(HTTP2StreamTest, testHeaderCallback) {
    // Check if the length returned is as expected
    int headerLength = TEST_EXCEPTION_STRING_LENGTH * NUMBER_OF_STRINGS;
    int returnHeaderLength = HTTP2Stream::headerCallback(
        m_dataBegin, TEST_EXCEPTION_STRING_LENGTH, NUMBER_OF_STRINGS, m_testableStream.get());
    ASSERT_EQ(headerLength, returnHeaderLength);
    // Call the function with NULL HTTP2Stream and check if it fails
    returnHeaderLength =
        HTTP2Stream::headerCallback(m_dataBegin, TEST_EXCEPTION_STRING_LENGTH, NUMBER_OF_STRINGS, nullptr);
    ASSERT_EQ(0, returnHeaderLength);
}

TEST_F(HTTP2StreamTest, testReadCallBack) {
    // Check if the bytesRead are equal to length of data written in SDS buffer
    auto indexAndStream = std::make_pair<size_t, HTTP2Stream*>(0, m_readTestableStream.get());
    int bytesRead =
        HTTP2Stream::readCallback(m_dataBegin, TEST_EXCEPTION_STRING_LENGTH, NUMBER_OF_STRINGS, &indexAndStream);
    ASSERT_EQ(TEST_EXCEPTION_STRING_LENGTH, bytesRead);
    // Call the function with NULL HTTP2Stream and check if it fails
    bytesRead = HTTP2Stream::readCallback(m_dataBegin, TEST_EXCEPTION_STRING_LENGTH, NUMBER_OF_STRINGS, nullptr);
    ASSERT_EQ(0, bytesRead);
}
}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
