/*
 * HTTP2StreamTest.cpp
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

/// @file HTTP2StreamTest.cpp

#include <memory>
#include <random>

#include <gtest/gtest.h>

#include <curl/curl.h>
#include <ACL/Transport/HTTP2Stream.h>

#include "TestableConsumer.h"
#include "MockMessageRequest.h"
#include "Common/Common.h"

namespace alexaClientSDK {
namespace acl {

/// A test url with which to initialize our test stream object.
static const std::string LIBCURL_TEST_URL = "http://example.com";
/// A test auth string with which to initialize our test stream object.
static const std::string LIBCURL_TEST_AUTH_STRING = "test_auth_string";
/// The lenth of the string we will test for the exception message.
static const int TEST_EXCEPTION_STRING_LENGTH = 200;
/// The number of iterations the multi-write test will perform.
static const int TEST_EXCEPTION_PARTITIONS = 7;

/**
 * Our GTest class.
 */
class HTTP2StreamTest : public ::testing::Test {
public:
    /**
     * Construct the objects we will use across tests.
     */
    void SetUp() override {
        // We won't be using the network explictly, but a HTTP2Stream object requires curl to be initialized
        // to work correctly.
        curl_global_init(CURL_GLOBAL_ALL);

        m_mockMessageRequest = std::make_shared<MockMessageRequest>();
        m_testableStream = std::make_shared<HTTP2Stream>(&m_testableConsumer, nullptr);
        m_testableStream->initPost(LIBCURL_TEST_URL, LIBCURL_TEST_AUTH_STRING, m_mockMessageRequest);
    }

    /**
     * Clean up.
     */
    void TearDown() override {
        curl_global_cleanup();
    }

    /// A mock message request object.
    std::shared_ptr<MockMessageRequest> m_mockMessageRequest;
    /// An object that is required for constructing a stream.
    TestableConsumer m_testableConsumer;
    /// The actual stream we will be testing.
    std::shared_ptr<HTTP2Stream> m_testableStream;
};

/**
 * Let's simulate that AVSConnectionManager->send() has been invoked, and the messageRequest object is
 * waiting to be notified on the response from AVS.  We will invoke the stream writeCallbacks directly to
 * simulate exception data returning from AVS, and verify that the stream passes the correct data back to
 * the request object.
 */
TEST_F(HTTP2StreamTest, testExceptionReceivedSingleWrite) {
    auto testString = createRandomAlphabetString(TEST_EXCEPTION_STRING_LENGTH);
    char* dataBegin = reinterpret_cast<char*>(&testString[0]);

    HTTP2Stream::writeCallback(dataBegin, TEST_EXCEPTION_STRING_LENGTH, 1, m_testableStream.get());

    EXPECT_CALL(*m_mockMessageRequest, onExceptionReceived(testString)).Times(1);

    // This simulates stream cleanup, which flushes out the parsed exception message.
    m_testableStream->notifyRequestObserver();
}

/**
 * The same test as above, but now with multiple writes (simulating either a small buffer from libcurl, or a very
 * long exception message).
 */
TEST_F(HTTP2StreamTest, testExceptionReceivedMultiWrite) {
    auto testString = createRandomAlphabetString(TEST_EXCEPTION_STRING_LENGTH);
    char* dataBegin = reinterpret_cast<char*>(&testString[0]);

    int writeQuantum = TEST_EXCEPTION_STRING_LENGTH;
    if (TEST_EXCEPTION_PARTITIONS > 0) {
        writeQuantum = TEST_EXCEPTION_STRING_LENGTH / TEST_EXCEPTION_PARTITIONS;
    }

    int numberBytesWritten = 0;

    char* currBuffPosition = dataBegin;
    while (numberBytesWritten < TEST_EXCEPTION_STRING_LENGTH) {

        int bytesRemaining = testString.length() - numberBytesWritten;
        int bytesToWrite = bytesRemaining < writeQuantum ? bytesRemaining : writeQuantum;

        HTTP2Stream::writeCallback(currBuffPosition, bytesToWrite, 1, m_testableStream.get());
        currBuffPosition += bytesToWrite;
        numberBytesWritten += bytesToWrite;
    }

    EXPECT_CALL(*m_mockMessageRequest, onExceptionReceived(testString)).Times(1);

    // This simulates stream cleanup, which flushes out the parsed exception message.
    m_testableStream->notifyRequestObserver();
}

} // namespace acl
} // namespace alexaClientSDK

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}