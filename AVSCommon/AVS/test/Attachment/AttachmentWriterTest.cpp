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

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

using namespace ::testing;
using namespace alexaClientSDK::avsCommon::avs::attachment;
using namespace alexaClientSDK::avsCommon::utils::sds;

/**
 * A class which helps drive this unit test suite.
 */
class AttachmentWriterTest : public ::testing::Test {
public:
    /**
     * Constructor.
     */
    AttachmentWriterTest() = default;

    /**
     * A function to nitilialize class data structures as needed.
     */
    void init();

    /**
     * A utility function to wrap up an important test for reading test data in multiple passes, and testing
     * various points of progress.
     *
     * @param closeWriterBeforeReading Indicating if the test should close the writer before performing reads.
     */
    void testMultipleReads(bool closeWriterBeforeReading);

    /// The commonly used SDS in these tests.
    std::shared_ptr<InProcessSDS> m_sds;
    /// The commonly used reader in these tests.
    std::unique_ptr<InProcessAttachmentReader> m_reader;
    /// The commonly used writer in these tests.
    std::unique_ptr<InProcessAttachmentWriter> m_writer;
    /// The commonly used test pattern in these tests.
    std::vector<uint8_t> m_testPattern;
};

void AttachmentWriterTest::init() {
    m_sds = createSDS(TEST_SDS_BUFFER_SIZE_IN_BYTES);
    ASSERT_NE(m_sds, nullptr);
    m_writer = InProcessAttachmentWriter::create(m_sds);
    ASSERT_NE(m_writer, nullptr);
    m_reader = InProcessAttachmentReader::create(ReaderPolicy::NONBLOCKING, m_sds);
    ASSERT_NE(m_reader, nullptr);
    m_testPattern = createTestPattern(TEST_SDS_BUFFER_SIZE_IN_BYTES);
}

void AttachmentWriterTest::testMultipleReads(bool closeWriterBeforeReading) {
    init();

    auto writeStatus = InProcessAttachmentWriter::WriteStatus::OK;
    auto numWritten = m_writer->write(m_testPattern.data(), m_testPattern.size(), &writeStatus);
    ASSERT_EQ(numWritten, m_testPattern.size());
    ASSERT_EQ(writeStatus, InProcessAttachmentWriter::WriteStatus::OK);

    AttachmentReader::ReadStatus terminalStatus = AttachmentReader::ReadStatus::OK_WOULDBLOCK;
    if (closeWriterBeforeReading) {
        m_writer->close();
        terminalStatus = AttachmentReader::ReadStatus::CLOSED;
    }

    std::vector<uint8_t> result(TEST_SDS_PARTIAL_READ_AMOUNT_IN_BYTES);
    auto readStatus = InProcessAttachmentReader::ReadStatus::OK;

    int totalBytesRead = 0;
    bool done = false;
    int iterations = 0;
    int iterationsMax = 10;

    while (!done && iterations < iterationsMax) {
        auto bytesRead = m_reader->read(result.data(), result.size(), &readStatus);

        if (terminalStatus == readStatus) {
            done = true;
        }

        for (size_t i = 0; i < bytesRead; ++i) {
            EXPECT_EQ(result[i], m_testPattern[i + totalBytesRead]);
        }

        totalBytesRead += bytesRead;
        iterations++;
    }

    ASSERT_NE(iterations, iterationsMax);
    ASSERT_EQ(readStatus, terminalStatus);
    ASSERT_EQ(totalBytesRead, static_cast<ssize_t>(m_testPattern.size()));
}

/**
 * Test writing to an invalid SDS.
 */
TEST_F(AttachmentWriterTest, testAttachmentWriterWithInvalidSDS) {
    auto writer = InProcessAttachmentWriter::create(nullptr);
    ASSERT_EQ(writer, nullptr);
}

/**
 * Test writing to a closed writer.
 */
TEST_F(AttachmentWriterTest, testAttachmentWriterOnClosedWriter) {
    init();

    m_writer->close();

    AttachmentWriter::WriteStatus writeStatus = AttachmentWriter::WriteStatus::OK;
    auto numWritten = m_writer->write(m_testPattern.data(), TEST_SDS_PARTIAL_WRITE_AMOUNT_IN_BYTES, &writeStatus);
    ASSERT_EQ(numWritten, 0U);
    ASSERT_EQ(writeStatus, InProcessAttachmentWriter::WriteStatus::CLOSED);
}

/**
 * Test writing a single pass of data.
 */
TEST_F(AttachmentWriterTest, testAttachmentWriterWriteSinglePass) {
    init();

    AttachmentWriter::WriteStatus writeStatus = AttachmentWriter::WriteStatus::OK;
    auto numWritten = m_writer->write(m_testPattern.data(), TEST_SDS_PARTIAL_WRITE_AMOUNT_IN_BYTES, &writeStatus);
    ASSERT_EQ(static_cast<ssize_t>(numWritten), TEST_SDS_PARTIAL_WRITE_AMOUNT_IN_BYTES);
    ASSERT_EQ(writeStatus, InProcessAttachmentWriter::WriteStatus::OK);
}

/**
 * Test a one-pass write and read with both wrapper classes.
 */
TEST_F(AttachmentWriterTest, testAttachmentWriterAndReadInOnePass) {
    init();

    auto writeStatus = InProcessAttachmentWriter::WriteStatus::OK;
    auto numWritten = m_writer->write(m_testPattern.data(), m_testPattern.size(), &writeStatus);
    ASSERT_EQ(numWritten, m_testPattern.size());
    ASSERT_EQ(writeStatus, InProcessAttachmentWriter::WriteStatus::OK);

    std::vector<uint8_t> result(m_testPattern.size());
    auto readStatus = InProcessAttachmentReader::ReadStatus::OK;
    auto numRead = m_reader->read(result.data(), result.size(), &readStatus);
    ASSERT_EQ(numRead, m_testPattern.size());
    ASSERT_EQ(readStatus, InProcessAttachmentReader::ReadStatus::OK);

    for (size_t i = 0; i < m_testPattern.size(); ++i) {
        EXPECT_EQ(result[i], m_testPattern[i]);
    }
}

/**
 * Test multiple partial reads of complete data, where the writer is closed.
 */
TEST_F(AttachmentWriterTest, testAttachmentReaderAndWriterMultipleReads) {
    testMultipleReads(true);
}

/**
 * Test multiple partial reads of complete data, where the writer remains open.
 */
TEST_F(AttachmentWriterTest, testAttachmentWriterAndReaderMultipleReadsOfUnfinishedData) {
    testMultipleReads(false);
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
