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

#include <future>
#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "AVSCommon/Utils/HTTPContent.h"
#include <AVSCommon/Utils/Memory/Memory.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {
namespace test {

using namespace ::testing;

/// A status code that represents success.
const static long SUCCESS_STATUS_CODE{200};

/// A status code that represents partial content.
const static long SUCCESS_PARTIAL_CONTENT_STATUS_CODE{206};

/// A status code that represents failure.
const static long BAD_STATUS_CODE{0};

/// A content type.
const static std::string TEST_CONTENT_TYPE{"unknown"};

/**
 * Class for testing the HTTPContent structure.
 */
class HTTPContentTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

protected:
    /// A promise to the caller of @c getContent() that the HTTP status code will be set.
    std::promise<long> m_statusCodePromise;

    /// A promise to the caller of @c getContent() that the HTTP content type will be set.
    std::promise<std::string> m_contentTypePromise;

    /// The @c HTTPContent for testing.
    std::unique_ptr<HTTPContent> m_httpContent;
};

void HTTPContentTest::SetUp() {
    auto httpStatusCodeFuture = m_statusCodePromise.get_future();
    auto contentTypeFuture = m_contentTypePromise.get_future();

    m_httpContent =
        memory::make_unique<HTTPContent>(std::move(httpStatusCodeFuture), std::move(contentTypeFuture), nullptr);
}

/// Test that isStatusCodeSuccess returns true for @c SUCCESS_STATUS_CODE.
TEST_F(HTTPContentTest, readStatusCodeSuccess) {
    m_statusCodePromise.set_value(SUCCESS_STATUS_CODE);
    m_contentTypePromise.set_value(TEST_CONTENT_TYPE);

    EXPECT_TRUE(m_httpContent->isStatusCodeSuccess());
}

/// Test that isStatusCodeSuccess returns true for @c SUCCESS_PARTIAL_CONTENT_STATUS_CODE.
TEST_F(HTTPContentTest, readStatusCodePartialContentSuccess) {
    m_statusCodePromise.set_value(SUCCESS_PARTIAL_CONTENT_STATUS_CODE);
    m_contentTypePromise.set_value(TEST_CONTENT_TYPE);

    EXPECT_TRUE(m_httpContent->isStatusCodeSuccess());
}

/// Test that isStatusCodeSuccess returns false for @c BAD_STATUS_CODE.
TEST_F(HTTPContentTest, readStatusCodeNotSuccess) {
    m_statusCodePromise.set_value(BAD_STATUS_CODE);
    m_contentTypePromise.set_value(TEST_CONTENT_TYPE);

    EXPECT_FALSE(m_httpContent->isStatusCodeSuccess());
}

/// Test that we can use @c getStatusCode() to get the status code after using @c isStatusCodeSuccess().
TEST_F(HTTPContentTest, readStatusCodeMoreThanOnce) {
    m_statusCodePromise.set_value(BAD_STATUS_CODE);
    m_contentTypePromise.set_value(TEST_CONTENT_TYPE);

    EXPECT_FALSE(m_httpContent->isStatusCodeSuccess());

    EXPECT_EQ(m_httpContent->getStatusCode(), BAD_STATUS_CODE);
}

/// Test that we can use @c getContentType() to get the status code after using @c isStatusCodeSuccess().
TEST_F(HTTPContentTest, readContentTypeMoreThanOnce) {
    m_statusCodePromise.set_value(BAD_STATUS_CODE);
    m_contentTypePromise.set_value(TEST_CONTENT_TYPE);

    EXPECT_EQ(m_httpContent->getContentType(), TEST_CONTENT_TYPE);
    EXPECT_EQ(m_httpContent->getContentType(), TEST_CONTENT_TYPE);
}

/// Test that we can retrieve the attachment reader, even if it's nullptr.
TEST_F(HTTPContentTest, getDataStream) {
    EXPECT_EQ(m_httpContent->getDataStream(), nullptr);
}

}  // namespace test
}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
