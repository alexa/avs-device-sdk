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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "AVSCommon/AVS/Attachment/InProcessAttachment.h"

#include "Common/Common.h"

using namespace ::testing;
using namespace alexaClientSDK::avsCommon::avs::attachment;
using namespace alexaClientSDK::avsCommon::utils::sds;

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

/**
 * A class which helps drive this unit test suite.
 */
class AttachmentTest : public ::testing::Test {
public:
    /**
     * Construtor.
     */
    AttachmentTest() : m_attachment{std::make_shared<InProcessAttachment>(TEST_ATTACHMENT_ID_STRING_ONE)} {
    }

    /**
     * Utility function to test creating a reader with a parameterized policy.
     *
     * @param policy The policy for the reader to be created.
     */
    void testCreateReader(ReaderPolicy policy);

    /// A local attachment object to simplify test code.
    std::shared_ptr<InProcessAttachment> m_attachment;
};

void AttachmentTest::testCreateReader(ReaderPolicy policy) {
    // Test create reader where there is no writer.
    auto reader = m_attachment->createReader(policy);
    ASSERT_NE(reader, nullptr);

    // Verify a second reader can't be created.
    reader = m_attachment->createReader(policy);
    ASSERT_EQ(reader, nullptr);

    // Start with a fresh attachment - this time test where there is a writer
    m_attachment = std::make_shared<InProcessAttachment>(TEST_ATTACHMENT_ID_STRING_ONE);

    m_attachment->createWriter();

    reader = m_attachment->createReader(policy);
    ASSERT_NE(reader, nullptr);

    // Verify a second reader can't be created.
    reader = m_attachment->createReader(policy);
    ASSERT_EQ(reader, nullptr);
}

/**
 * Verify the ID is correctly stored and accessed from an Attachment.
 */
TEST_F(AttachmentTest, testGetAttachmentId) {
    ASSERT_EQ(TEST_ATTACHMENT_ID_STRING_ONE, m_attachment->getId());
}

/**
 * Verify that an Attachment can create a blocking reader in various scenarios.
 */
TEST_F(AttachmentTest, testAttachmentCreateBlockingReader) {
    testCreateReader(ReaderPolicy::BLOCKING);
}

/**
 * Verify that an Attachment can create a non-blocking reader in various scenarios.
 */
TEST_F(AttachmentTest, testAttachmentCreateNonBlockingReader) {
    testCreateReader(ReaderPolicy::NONBLOCKING);
}

/**
 * Verify that an Attachment can create a writer in various scenarios.
 */
TEST_F(AttachmentTest, testAttachmentCreateWriter) {
    // Test create writer where there is no reader.
    auto writer = m_attachment->createWriter();
    ASSERT_NE(writer, nullptr);

    // Verify a second writer can't be created.
    writer = m_attachment->createWriter();
    ASSERT_EQ(writer, nullptr);

    // Once again - this time test where there is a reader.
    m_attachment = std::make_shared<InProcessAttachment>(TEST_ATTACHMENT_ID_STRING_ONE);

    m_attachment->createReader(ReaderPolicy::NONBLOCKING);

    writer = m_attachment->createWriter();
    ASSERT_NE(writer, nullptr);

    // Verify a second writer can't be created.
    writer = m_attachment->createWriter();
    ASSERT_EQ(writer, nullptr);
}

/**
 * Test creating an Attachment with an existing SDS.
 */
TEST_F(AttachmentTest, testCreateAttachmentWithSDS) {
    auto sds = createSDS(TEST_SDS_BUFFER_SIZE_IN_BYTES);
    auto attachment = std::make_shared<InProcessAttachment>(TEST_ATTACHMENT_ID_STRING_ONE, std::move(sds));

    // Test member functions, ensure they appear to work correctly.
    ASSERT_EQ(TEST_ATTACHMENT_ID_STRING_ONE, attachment->getId());

    auto reader = attachment->createReader(ReaderPolicy::NONBLOCKING);
    ASSERT_NE(reader, nullptr);

    auto writer = attachment->createWriter();
    ASSERT_NE(writer, nullptr);
}

/**
 * Verify an Attachment can't create multiple writers.
 */
TEST_F(AttachmentTest, testAttachmentCreateMultipleWriters) {
    auto writer1 = m_attachment->createWriter();
    auto writer2 = m_attachment->createWriter();
    ASSERT_NE(writer1, nullptr);
    ASSERT_EQ(writer2, nullptr);
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
