/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <gtest/gtest.h>
#include "AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2Connection.h"
#include "AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2Request.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {
using namespace ::testing;

/**
 * Derived test class for LibCurlHTTP2Connection that is a friend class to the google test class
 */
class LibCurlHTTP2Connection_Test : public LibcurlHTTP2Connection {
public:
    LibCurlHTTP2Connection_Test() = default;
    friend class GTEST_TEST_CLASS_NAME_(LibCurlHTTP2ConnectionTest, releaseStream_delete_ok);
};
/**
 * Test fixture class for LibCurlHTTP2Connection
 */

class LibCurlHTTP2ConnectionTest : public ::testing::Test {
protected:
    void SetUp() override;
    /// The LibCurlHTTP2Connection to test.
    std::shared_ptr<LibCurlHTTP2Connection_Test> m_libCurlHTTP2Connection;
};

void LibCurlHTTP2ConnectionTest::SetUp() {
    m_libCurlHTTP2Connection = std::make_shared<LibCurlHTTP2Connection_Test>();
}

TEST_F(LibCurlHTTP2ConnectionTest, releaseStream_delete_ok) {
    // Setting IsStopping bool to true because we do not want network loop to process this request
    m_libCurlHTTP2Connection->setIsStopping();
    m_libCurlHTTP2Connection->createMultiHandle();
    http2::HTTP2RequestConfig config{http2::HTTP2RequestType::GET, "www.foo.com", "xyz"};
    config.setConnectionTimeout(std::chrono::seconds(60));
    config.setIntermittentTransferExpected();
    auto req = std::make_shared<LibcurlHTTP2Request>(config, nullptr, config.getId());
    auto handle = req->getCurlHandle();
    m_libCurlHTTP2Connection->m_activeStreams[handle] = req;
    ASSERT_TRUE(m_libCurlHTTP2Connection->m_activeStreams.size());
    LibCurlHTTP2Connection_Test::ActiveStreamMap::iterator iter = m_libCurlHTTP2Connection->m_activeStreams.begin();
    m_libCurlHTTP2Connection->releaseStream(iter);
    ASSERT_FALSE(m_libCurlHTTP2Connection->m_activeStreams.size());
}

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK