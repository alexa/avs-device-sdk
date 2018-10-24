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

#include "../include/AVSCommon/AVS/Attachment/InProcessAttachmentReader.h"
#include "../include/AVSCommon/AVS/Attachment/AttachmentWriter.h"
#include "../include/AVSCommon/Utils/Threading/Executor.h"

#include "Common/Common.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

using namespace ::testing;
using namespace alexaClientSDK::avsCommon::avs::attachment;
using namespace alexaClientSDK::avsCommon::utils::sds;

/// A test seek position, defined in terms of above variables.
static const int TEST_SDS_SEEK_POSITION = TEST_SDS_BUFFER_SIZE_IN_BYTES - (TEST_SDS_PARTIAL_READ_AMOUNT_IN_BYTES + 10);
/// A test seek position which is bad.
static const int TEST_SDS_BAD_SEEK_POSITION = TEST_SDS_BUFFER_SIZE_IN_BYTES + 1;
/// Timeout for how long attachment reader loop can run while it is checking for certain status from read call.
static const int ATTACHMENT_READ_LOOP_TIMEOUT_MS = 5 * 1000;
/// Time to wait between each read call in reader loop.
static const int ATTACHMENT_READ_LOOP_WAIT_BETWEEN_READS_MS = 20;

/**
 * A class which helps drive this unit test suite.
 */
class AttachmentReaderTest : public ::testing::Test {
public:
    /**
     * Constructor.
     */
    AttachmentReaderTest() :
            m_readerPolicy{InProcessSDS::Reader::Policy::NONBLOCKING},
            m_writerPolicy{InProcessSDS::Writer::Policy::ALL_OR_NOTHING} {
    }

    /**
     * Initialization function to set up and test the class members.
     *
     * @param createReader Whether the reader data member should be constructed from m_sds.
     * @param resetOnOverrun Whether to create @c InProcessAttachmentReader with @c resetOnOverrun policy or not.
     */
    void init(bool createReader = true, bool resetOnOverrun = false);

    /**
     * Utility function to test multiple reads from an SDS.
     *
     * @param closeWriterBeforeReading Allows the writer to be closed before the reader performs its multiple reads.
     */
    void testMultipleReads(bool closeWriterBeforeReading);

    /**
     * Utility function to read data from an SDS and verify its results againts m_pattern.
     *
     * @param reader The reader with which to read data.
     * @param resultSize The size of the data the reader should read.
     * @param dataOffset The offset into the pattern which is being read from.
     */
    void readAndVerifyResult(
        std::shared_ptr<InProcessAttachmentReader> reader,
        size_t resultSize,
        size_t dataOffset = 0);

    /// The commonly used AttachmentReader policy.
    ReaderPolicy m_readerPolicy;
    /// The commonly used SDSWriter policy.
    InProcessSDS::Writer::Policy m_writerPolicy;
    /// The commonly used SDS in these tests.
    std::shared_ptr<InProcessSDS> m_sds;
    /// The commonly used reader in these tests.
    std::unique_ptr<InProcessAttachmentReader> m_reader;
    /// The commonly used writer in these tests.
    std::unique_ptr<InProcessSDS::Writer> m_writer;
    /// The commonly used test pattern in these tests.
    std::vector<uint8_t> m_testPattern;
};

void AttachmentReaderTest::init(bool createReader, bool resetOnOverrun) {
    m_sds = createSDS(TEST_SDS_BUFFER_SIZE_IN_BYTES);
    ASSERT_NE(m_sds, nullptr);
    m_writer = m_sds->createWriter(m_writerPolicy);
    ASSERT_NE(m_writer, nullptr);
    if (createReader) {
        m_reader = InProcessAttachmentReader::create(
            m_readerPolicy, m_sds, 0, InProcessAttachmentReader::SDSTypeReader::Reference::ABSOLUTE, resetOnOverrun);
        ASSERT_NE(m_reader, nullptr);
    }
    m_testPattern = createTestPattern(TEST_SDS_BUFFER_SIZE_IN_BYTES);
}

void AttachmentReaderTest::testMultipleReads(bool closeWriterBeforeReading) {
    init();

    auto numWritten = m_writer->write(m_testPattern.data(), m_testPattern.size());
    ASSERT_EQ(numWritten, static_cast<ssize_t>(m_testPattern.size()));

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

    // Not only was all the data read, but the reader remained open.
    ASSERT_NE(iterations, iterationsMax);
    ASSERT_EQ(readStatus, terminalStatus);
    ASSERT_EQ(totalBytesRead, static_cast<ssize_t>(m_testPattern.size()));
}

void AttachmentReaderTest::readAndVerifyResult(
    std::shared_ptr<InProcessAttachmentReader> reader,
    size_t resultSize,
    size_t dataOffset) {
    std::vector<uint8_t> result(resultSize);
    auto readStatus = InProcessAttachmentReader::ReadStatus::OK;
    auto numRead = reader->read(result.data(), result.size(), &readStatus);
    ASSERT_EQ(numRead, result.size());
    ASSERT_EQ(readStatus, InProcessAttachmentReader::ReadStatus::OK);

    for (size_t i = 0; i < result.size(); ++i) {
        EXPECT_EQ(result[i], m_testPattern[i + dataOffset]);
    }
}

/**
 * Test reading an invalid SDS.
 */
TEST_F(AttachmentReaderTest, testAttachmentReaderWithInvalidSDS) {
    auto reader = InProcessAttachmentReader::create(m_readerPolicy, nullptr);
    ASSERT_EQ(reader, nullptr);
}

/**
 * Test reading an SDS with a bad seek position.
 */
TEST_F(AttachmentReaderTest, testAttachmentReaderWithBadSeekPosition) {
    auto reader = InProcessAttachmentReader::create(m_readerPolicy, m_sds, TEST_SDS_BAD_SEEK_POSITION);
    ASSERT_EQ(reader, nullptr);
}

/**
 * Test a one-pass write and read.
 */
TEST_F(AttachmentReaderTest, testAttachmentReaderReadInOnePass) {
    init();

    auto testPattern = createTestPattern(TEST_SDS_BUFFER_SIZE_IN_BYTES);
    auto numWritten = m_writer->write(m_testPattern.data(), m_testPattern.size());
    ASSERT_EQ(numWritten, static_cast<ssize_t>(m_testPattern.size()));

    readAndVerifyResult(std::shared_ptr<InProcessAttachmentReader>(std::move(m_reader)), TEST_SDS_BUFFER_SIZE_IN_BYTES);
}

/**
 * Test a partial read.
 */
TEST_F(AttachmentReaderTest, testAttachmentReaderPartialRead) {
    init();

    auto numWritten = m_writer->write(m_testPattern.data(), m_testPattern.size());
    ASSERT_EQ(numWritten, static_cast<ssize_t>(m_testPattern.size()));

    readAndVerifyResult(std::shared_ptr<InProcessAttachmentReader>(std::move(m_reader)), TEST_SDS_BUFFER_SIZE_IN_BYTES);
}

/**
 * Test a partial read with a seek.
 */
TEST_F(AttachmentReaderTest, testAttachmentReaderPartialReadWithSeek) {
    init(false);

    // test a single write & read.
    auto numWritten = m_writer->write(m_testPattern.data(), m_testPattern.size());
    ASSERT_EQ(numWritten, static_cast<ssize_t>(m_testPattern.size()));

    auto reader = InProcessAttachmentReader::create(m_readerPolicy, m_sds, TEST_SDS_SEEK_POSITION);
    ASSERT_NE(reader, nullptr);

    readAndVerifyResult(
        std::shared_ptr<InProcessAttachmentReader>(std::move(reader)),
        TEST_SDS_PARTIAL_READ_AMOUNT_IN_BYTES,
        TEST_SDS_SEEK_POSITION);
}

/**
 * Test multiple partial reads of complete data, where the writer closes.
 */
TEST_F(AttachmentReaderTest, testAttachmentReaderMultipleReads) {
    testMultipleReads(false);
}

/**
 * Test multiple partial reads of complete data, where the writer remains open.
 */
TEST_F(AttachmentReaderTest, testAttachmentReaderMultipleReadsOfUnfinishedData) {
    testMultipleReads(true);
}

/**
 * Test that reading at much slower pace than writing causes reader to eventually receive
 * overrun error.
 */
TEST_F(AttachmentReaderTest, testOverrunResultsInError) {
    m_writerPolicy = InProcessSDS::Writer::Policy::NONBLOCKABLE;
    init();

    auto continueWriting = std::make_shared<std::atomic<bool>>(true);
    auto writer = m_writer.get();

    std::thread writerThread([writer, continueWriting]() {
        auto testPattern = createTestPattern(TEST_SDS_BUFFER_SIZE_IN_BYTES);
        while (continueWriting->load()) {
            writer->write(testPattern.data(), testPattern.size());
        }
    });

    std::vector<uint8_t> result(TEST_SDS_BUFFER_SIZE_IN_BYTES);
    auto readStatus = InProcessAttachmentReader::ReadStatus::OK;

    uint32_t maxLoops = ATTACHMENT_READ_LOOP_TIMEOUT_MS / ATTACHMENT_READ_LOOP_WAIT_BETWEEN_READS_MS;
    uint32_t loopCounter = 0;
    while (readStatus != InProcessAttachmentReader::ReadStatus::ERROR_OVERRUN) {
        m_reader->read(result.data(), result.size(), &readStatus);
        std::this_thread::sleep_for(std::chrono::milliseconds(ATTACHMENT_READ_LOOP_WAIT_BETWEEN_READS_MS));
        if (++loopCounter == maxLoops) {
            break;
        }
    }
    continueWriting->store(false);
    writerThread.join();

    ASSERT_LT(loopCounter, maxLoops);
}

/**
 * Test that reading at much slower pace than writing causes reader cursor position to be reset
 * to writer cursor position.
 */
TEST_F(AttachmentReaderTest, testOverrunResultsInReaderReset) {
    m_writerPolicy = InProcessSDS::Writer::Policy::NONBLOCKABLE;
    init(true, true);

    auto continueWriting = std::make_shared<std::atomic<bool>>(true);
    auto writer = m_writer.get();

    std::thread writerThread([writer, continueWriting]() {
        auto testPattern = createTestPattern(TEST_SDS_BUFFER_SIZE_IN_BYTES);
        while (continueWriting->load()) {
            writer->write(testPattern.data(), testPattern.size());
        }
    });

    std::vector<uint8_t> result(TEST_SDS_BUFFER_SIZE_IN_BYTES);
    auto readStatus = InProcessAttachmentReader::ReadStatus::OK;

    uint32_t maxLoops = ATTACHMENT_READ_LOOP_TIMEOUT_MS / ATTACHMENT_READ_LOOP_WAIT_BETWEEN_READS_MS;
    uint32_t loopCounter = 0;
    while (readStatus != InProcessAttachmentReader::ReadStatus::OK_OVERRUN_RESET) {
        m_reader->read(result.data(), result.size(), &readStatus);
        std::this_thread::sleep_for(std::chrono::milliseconds(ATTACHMENT_READ_LOOP_WAIT_BETWEEN_READS_MS));
        if (++loopCounter == maxLoops) {
            break;
        }
    }

    // Quit writing
    continueWriting->store(false);
    writerThread.join();

    ASSERT_LT(loopCounter, maxLoops);

    // Drain the reader
    loopCounter = 0;
    while (readStatus != InProcessAttachmentReader::ReadStatus::OK_WOULDBLOCK) {
        m_reader->read(result.data(), result.size(), &readStatus);
        std::this_thread::sleep_for(std::chrono::milliseconds(ATTACHMENT_READ_LOOP_WAIT_BETWEEN_READS_MS));
        if (++loopCounter == maxLoops) {
            break;
        }
    }

    ASSERT_LT(loopCounter, maxLoops);

    // Write the bytes, read and verify that same pattern is read
    auto testPattern = createTestPattern(TEST_SDS_BUFFER_SIZE_IN_BYTES);
    writer->write(testPattern.data(), testPattern.size());
    m_reader->read(result.data(), result.size(), &readStatus);

    EXPECT_EQ(readStatus, InProcessAttachmentReader::ReadStatus::OK);
    EXPECT_EQ(testPattern, result);
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
