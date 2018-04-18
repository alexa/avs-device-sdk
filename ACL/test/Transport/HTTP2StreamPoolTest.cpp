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

/// @file HTTP2StreamPoolTest.cpp

#include <memory>
#include <vector>
#include <gtest/gtest.h>

#include <curl/curl.h>
#include <ACL/Transport/HTTP2StreamPool.h>
#include <ACL/Transport/HTTP2Transport.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include "Common/Common.h"
#include "TestableConsumer.h"
#include "MockMessageRequest.h"

namespace alexaClientSDK {
namespace acl {
namespace test {

using namespace alexaClientSDK::avsCommon::avs::initialization;
/// A test URL to initialize our object with
static const std::string TEST_LIBCURL_URL = "https://www.amazon.com/";
/// The maximum number of streams in the stream pool
static const int TEST_MAX_STREAMS = 10;
/// A test auth string with which to initialize our test stream object.
static const std::string LIBCURL_TEST_AUTH_STRING = "test_auth_string";

/**
 * Our GTest class.
 */
class HTTP2StreamPoolTest : public ::testing::Test {
public:
    void SetUp() override;

    void TearDown() override;

    /// A mock message request object.
    std::shared_ptr<MockMessageRequest> m_mockMessageRequest;
    /// An object that is required for constructing a stream.
    std::shared_ptr<TestableConsumer> m_testableConsumer;
    /// The actual stream we will be testing.
    std::shared_ptr<HTTP2StreamPool> m_testableStreamPool;
};

void HTTP2StreamPoolTest::SetUp() {
    AlexaClientSDKInit::initialize(std::vector<std::shared_ptr<std::istream>>());
    m_mockMessageRequest = std::make_shared<MockMessageRequest>();
    m_testableConsumer = std::make_shared<TestableConsumer>();
    m_testableStreamPool = std::make_shared<HTTP2StreamPool>(TEST_MAX_STREAMS, nullptr);
}

void HTTP2StreamPoolTest::TearDown() {
    AlexaClientSDKInit::uninitialize();
}

/**
 * Lets simulate sending more than max streams to GET requests
 */
TEST_F(HTTP2StreamPoolTest, GetStreamSendNullPtrForMoreThanMaxStreams) {
    std::shared_ptr<HTTP2Stream> stream;

    // Send TEST_MAX_STREAMS number of streams, stream should not be nullptr
    for (int numberOfStreamSent = 0; numberOfStreamSent < TEST_MAX_STREAMS; numberOfStreamSent++) {
        stream = m_testableStreamPool->createGetStream(TEST_LIBCURL_URL, LIBCURL_TEST_AUTH_STRING, m_testableConsumer);
        ASSERT_NE(stream, nullptr);
    }
    // Send another stream in addition to TEST_MAX_STREAMS, stream should be a nullptr
    stream = m_testableStreamPool->createGetStream(TEST_LIBCURL_URL, LIBCURL_TEST_AUTH_STRING, m_testableConsumer);
    ASSERT_EQ(stream, nullptr);
}

/**
 * This function tests failure of @c createGetStream on different types of failure on initGet()
 */
TEST_F(HTTP2StreamPoolTest, initGetFails) {
    std::shared_ptr<HTTP2Stream> stream;
    stream = m_testableStreamPool->createGetStream("", LIBCURL_TEST_AUTH_STRING, m_testableConsumer);
    ASSERT_EQ(stream, nullptr);
    stream = m_testableStreamPool->createGetStream(TEST_LIBCURL_URL, "", m_testableConsumer);
    ASSERT_EQ(stream, nullptr);
    stream = m_testableStreamPool->createGetStream(TEST_LIBCURL_URL, LIBCURL_TEST_AUTH_STRING, nullptr);
    ASSERT_EQ(stream, nullptr);
}

/**
 * This function tests failure of @c createPostStream on different types of failure on initPost()
 */
TEST_F(HTTP2StreamPoolTest, initPostFails) {
    std::shared_ptr<HTTP2Stream> stream;
    stream =
        m_testableStreamPool->createPostStream("", LIBCURL_TEST_AUTH_STRING, m_mockMessageRequest, m_testableConsumer);
    ASSERT_EQ(stream, nullptr);
    stream = m_testableStreamPool->createPostStream(TEST_LIBCURL_URL, "", m_mockMessageRequest, m_testableConsumer);
    ASSERT_EQ(stream, nullptr);
    stream =
        m_testableStreamPool->createPostStream(TEST_LIBCURL_URL, LIBCURL_TEST_AUTH_STRING, nullptr, m_testableConsumer);
    ASSERT_EQ(stream, nullptr);
    stream = m_testableStreamPool->createPostStream(
        TEST_LIBCURL_URL, LIBCURL_TEST_AUTH_STRING, m_mockMessageRequest, nullptr);
    ASSERT_EQ(stream, nullptr);
}

/**
 * Lets simulate sending more than max streams to POST requests
 */
TEST_F(HTTP2StreamPoolTest, PostStreamSendNullPtrForMoreThanMaxStreams) {
    std::shared_ptr<HTTP2Stream> stream;

    // Send TEST_MAX_STREAMS number of streams, stream should not be nullptr
    for (int numberOfStreamSent = 0; numberOfStreamSent < TEST_MAX_STREAMS; numberOfStreamSent++) {
        stream = m_testableStreamPool->createPostStream(
            TEST_LIBCURL_URL, LIBCURL_TEST_AUTH_STRING, m_mockMessageRequest, m_testableConsumer);
        ASSERT_NE(stream, nullptr);
    }
    // Send another stream in addition to TEST_MAX_STREAMS, stream should be a nullptr
    stream = m_testableStreamPool->createPostStream(
        TEST_LIBCURL_URL, LIBCURL_TEST_AUTH_STRING, m_mockMessageRequest, m_testableConsumer);
    ASSERT_EQ(stream, nullptr);
}

/**
 * Simulate sending more than max streams to GET requests; when it fails to add more,
 * release a few streams using @c releaseStream, then add more streams which should pass.
 */
TEST_F(HTTP2StreamPoolTest, ReleaseStreamTestSendMoreThanMaxStreams) {
    std::vector<std::shared_ptr<HTTP2Stream>> stream_pool;
    const int numOfRemovedStreams = 2;

    // Send max number of streams
    for (int count = 0; count < TEST_MAX_STREAMS; count++) {
        stream_pool.push_back(
            m_testableStreamPool->createGetStream(TEST_LIBCURL_URL, LIBCURL_TEST_AUTH_STRING, m_testableConsumer));
        ASSERT_NE(stream_pool.back(), nullptr);
    }
    // Send one more stream, it should fail
    stream_pool.push_back(
        m_testableStreamPool->createGetStream(TEST_LIBCURL_URL, LIBCURL_TEST_AUTH_STRING, m_testableConsumer));
    ASSERT_EQ(stream_pool.back(), nullptr);
    stream_pool.pop_back();

    // Release few streams
    for (int count = 0; count < numOfRemovedStreams; count++) {
        m_testableStreamPool->releaseStream(stream_pool.back());
        stream_pool.pop_back();
    }

    // Send more streams, now it should pass
    for (int count = 0; count < numOfRemovedStreams; count++) {
        stream_pool.push_back(
            m_testableStreamPool->createGetStream(TEST_LIBCURL_URL, LIBCURL_TEST_AUTH_STRING, m_testableConsumer));
        ASSERT_NE(stream_pool.back(), nullptr);
    }
}

/**
 * Try sending nullptr on @c releaseStream after sending max number of streams,
 * check for failure of sending more streams.
 */
TEST_F(HTTP2StreamPoolTest, ReleaseStreamTestAfterNullTest) {
    std::vector<std::shared_ptr<HTTP2Stream>> stream_pool;
    const int numOfRemovedStreams = 2;

    // Send max number of streams
    for (int count = 0; count < TEST_MAX_STREAMS; count++) {
        stream_pool.push_back(
            m_testableStreamPool->createGetStream(TEST_LIBCURL_URL, LIBCURL_TEST_AUTH_STRING, m_testableConsumer));
        ASSERT_NE(stream_pool.back(), nullptr);
    }

    // Pass nullptr on releaseStream
    for (int count = 0; count < numOfRemovedStreams; count++) {
        m_testableStreamPool->releaseStream(nullptr);
    }

    // Send more streams, it should still fail
    for (int count = 0; count < numOfRemovedStreams; count++) {
        stream_pool.push_back(
            m_testableStreamPool->createGetStream(TEST_LIBCURL_URL, LIBCURL_TEST_AUTH_STRING, m_testableConsumer));
        ASSERT_EQ(stream_pool.back(), nullptr);
    }
}
}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
