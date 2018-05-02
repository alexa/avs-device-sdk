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

#include <cstring>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "AVSCommon/AVS/Attachment/AttachmentUtils.h"

using namespace ::testing;
using namespace alexaClientSDK::avsCommon::avs::attachment;

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

const static std::string sampleBuffer = "example buffer";

class AttachmentUtilsTest : public ::testing::Test {
public:
    /// Setup called before each test.
    void SetUp();

protected:
    /// Example buffer @ AttachmentReader.
    std::shared_ptr<AttachmentReader> m_attachmentReader;
};

void AttachmentUtilsTest::SetUp() {
    std::vector<char> srcBuffer;
    srcBuffer.assign(sampleBuffer.begin(), sampleBuffer.end());

    m_attachmentReader = AttachmentUtils::createAttachmentReader(srcBuffer);
    ASSERT_TRUE(m_attachmentReader);
}

/**
 * Test read until end of buffer
 */
TEST_F(AttachmentUtilsTest, testReadCompleteBuffer) {
    char dstBuffer[sampleBuffer.length() + 10];
    memset(dstBuffer, 0, sampleBuffer.length() + 10);

    AttachmentReader::ReadStatus status;
    size_t bytesRead = m_attachmentReader->read(dstBuffer, sampleBuffer.length(), &status);

    ASSERT_EQ(bytesRead, sampleBuffer.length());
    ASSERT_EQ(status, AttachmentReader::ReadStatus::OK);

    bytesRead = m_attachmentReader->read(dstBuffer, sampleBuffer.length(), &status);
    ASSERT_EQ(bytesRead, 0U);
    ASSERT_EQ(status, AttachmentReader::ReadStatus::CLOSED);
    ASSERT_EQ(strcmp(sampleBuffer.data(), dstBuffer), 0);
}

/*
 * Test empty buffer
 */
TEST_F(AttachmentUtilsTest, testEmptyBuffer) {
    std::vector<char> emptySrcBuffer;
    auto emptyAttachmentReader = AttachmentUtils::createAttachmentReader(emptySrcBuffer);
    ASSERT_FALSE(emptyAttachmentReader);
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
