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

#include "AVSCommon/AVS/Attachment/AttachmentManager.h"
#include "AVSCommon/AVS/Attachment/InProcessAttachment.h"

#include "Common/Common.h"

using namespace ::testing;
using namespace alexaClientSDK::avsCommon::avs::attachment;

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

/// Empty string used for testing.
static const std::string TEST_EMPTY_STRING = "";
/// A test ContextId string.
static const std::string TEST_CONTEXT_ID_STRING = "testContextId";
/// A test ContentId string.
static const std::string TEST_CONTENT_ID_STRING = "testContentId";
/// A second test ContextId string.
static const std::string TEST_CONTENT_ID_ALTERNATE_STRING = "testContentId2";
/// A test timeout.
static const std::chrono::minutes TIMEOUT_REGULAR = std::chrono::minutes(60);
/// A test zero timeout.
static const std::chrono::minutes TIMEOUT_ZERO = std::chrono::minutes(0);
/// A test negative timeout.
static const std::chrono::minutes TIMEOUT_NEGATIVE = std::chrono::minutes(-1);

/**
 * A class which helps drive this unit test suite.
 */
class AttachmentManagerTest : public ::testing::Test {
public:
    /**
     * Constructor.
     */
    AttachmentManagerTest() : m_manager{AttachmentManager::AttachmentType::IN_PROCESS} {
    }

    /// Local type aliases.
    using ReaderVec = std::vector<std::unique_ptr<AttachmentReader>>;
    using WriterVec = std::vector<std::unique_ptr<AttachmentWriter>>;

    /**
     * Utility function for creating three writer futures, and inserting them into the vector.
     */
    void createWriters(WriterVec* writers);

    /**
     * Utility function for testing writer futures.
     */
    void testWriters(const WriterVec& writers, bool expectedValid);

    /**
     * Utility function for creating three reader futures, and inserting them into the vector.
     */
    void createReaders(ReaderVec* readers);

    /**
     * Utility function for testing reader futures.
     */
    void testReaders(const ReaderVec& readers, bool expectedValid);

    /// A local @c AttachmentManager object.
    AttachmentManager m_manager;
};

void AttachmentManagerTest::createWriters(WriterVec* writers) {
    writers->push_back(m_manager.createWriter(TEST_ATTACHMENT_ID_STRING_ONE));
    writers->push_back(m_manager.createWriter(TEST_ATTACHMENT_ID_STRING_TWO));
    writers->push_back(m_manager.createWriter(TEST_ATTACHMENT_ID_STRING_THREE));
}

void AttachmentManagerTest::testWriters(const WriterVec& writers, bool expectedValid) {
    for (auto& writer : writers) {
        if (expectedValid) {
            ASSERT_NE(writer, nullptr);
        } else {
            ASSERT_EQ(writer, nullptr);
        }
    }
}

void AttachmentManagerTest::createReaders(ReaderVec* readers) {
    readers->push_back(m_manager.createReader(TEST_ATTACHMENT_ID_STRING_ONE, utils::sds::ReaderPolicy::BLOCKING));
    readers->push_back(m_manager.createReader(TEST_ATTACHMENT_ID_STRING_TWO, utils::sds::ReaderPolicy::BLOCKING));
    readers->push_back(m_manager.createReader(TEST_ATTACHMENT_ID_STRING_THREE, utils::sds::ReaderPolicy::BLOCKING));
}

void AttachmentManagerTest::testReaders(const ReaderVec& readers, bool expectedValid) {
    for (auto& reader : readers) {
        if (expectedValid) {
            ASSERT_NE(reader, nullptr);
        } else {
            ASSERT_EQ(reader, nullptr);
        }
    }
}

/**
 * Test that the AttachmentManager's generate attachment id function works as expected.
 */
TEST_F(AttachmentManagerTest, testAttachmentManagerGenerateAttachmentId) {
    // Normal use cases.
    auto id1 = m_manager.generateAttachmentId(TEST_CONTEXT_ID_STRING, TEST_CONTENT_ID_STRING);
    auto id2 = m_manager.generateAttachmentId(TEST_CONTEXT_ID_STRING, TEST_CONTENT_ID_STRING);
    auto id3 = m_manager.generateAttachmentId(TEST_CONTEXT_ID_STRING, TEST_CONTENT_ID_ALTERNATE_STRING);
    ASSERT_EQ(id1, id2);
    ASSERT_NE(id1, id3);
    ASSERT_NE(id2, id3);

    // Both strings empty.
    auto id4 = m_manager.generateAttachmentId(TEST_EMPTY_STRING, TEST_EMPTY_STRING);
    ASSERT_TRUE(id4.empty());

    // ContentId string is empty.
    auto id5 = m_manager.generateAttachmentId(TEST_CONTEXT_ID_STRING, TEST_EMPTY_STRING);
    ASSERT_EQ(id5, TEST_CONTEXT_ID_STRING);

    // ContextId string is empty.
    auto id6 = m_manager.generateAttachmentId(TEST_EMPTY_STRING, TEST_CONTENT_ID_STRING);
    ASSERT_EQ(id6, TEST_CONTENT_ID_STRING);
}

/**
 * Test that the AttachmentManager's set timeout function works as expected.
 */
TEST_F(AttachmentManagerTest, testAttachmentManagerSetTimeout) {
    ASSERT_TRUE(m_manager.setAttachmentTimeoutMinutes(TIMEOUT_REGULAR));
    ASSERT_TRUE(m_manager.setAttachmentTimeoutMinutes(AttachmentManager::ATTACHMENT_MANAGER_TIMOUT_MINUTES_MINIMUM));
    ASSERT_FALSE(m_manager.setAttachmentTimeoutMinutes(TIMEOUT_ZERO));
    ASSERT_FALSE(m_manager.setAttachmentTimeoutMinutes(TIMEOUT_NEGATIVE));
}

/**
 * Test that the AttachmentManager's create* functions work in this particular order.
 */
TEST_F(AttachmentManagerTest, testAttachmentManagerCreateWriterThenReader) {
    // Create the writer before the reader.
    auto writer = m_manager.createWriter(TEST_ATTACHMENT_ID_STRING_ONE);
    auto reader = m_manager.createReader(TEST_ATTACHMENT_ID_STRING_ONE, utils::sds::ReaderPolicy::BLOCKING);
    ASSERT_NE(writer, nullptr);
    ASSERT_NE(reader, nullptr);
}

/**
 * Test that the AttachmentManager's create* functions work in this particular order.
 */
TEST_F(AttachmentManagerTest, testAttachmentManagerCreateReaderThenWriter) {
    // Create the reader before the writer.
    auto reader = m_manager.createReader(TEST_ATTACHMENT_ID_STRING_ONE, utils::sds::ReaderPolicy::BLOCKING);
    auto writer = m_manager.createWriter(TEST_ATTACHMENT_ID_STRING_ONE);
    ASSERT_NE(writer, nullptr);
    ASSERT_NE(reader, nullptr);
}

/**
 * Test that the AttachmentManager's create reader function works as expected.
 */
TEST_F(AttachmentManagerTest, testAttachmentManagerCreateReader) {
    // Create the reader.
    auto reader = m_manager.createReader(TEST_ATTACHMENT_ID_STRING_ONE, utils::sds::ReaderPolicy::BLOCKING);
    ASSERT_NE(reader, nullptr);
}

/**
 * Test that a reader created from an attachment that doesn't have a writer will wait for the writer.
 */
TEST_F(AttachmentManagerTest, testAttachmentManagerReadAttachmentWithoutWriter) {
    auto testPattern = createTestPattern(TEST_SDS_BUFFER_SIZE_IN_BYTES);
    std::vector<uint8_t> result(testPattern.size());

    // Create the reader.
    auto reader = m_manager.createReader(TEST_ATTACHMENT_ID_STRING_ONE, utils::sds::ReaderPolicy::NONBLOCKING);
    ASSERT_NE(reader, nullptr);

    // Verify that read indicates an empty (but not closed) buffer.
    auto readStatus = InProcessAttachmentReader::ReadStatus::OK;
    reader->read(result.data(), result.size(), &readStatus);
    ASSERT_EQ(readStatus, InProcessAttachmentReader::ReadStatus::OK_WOULDBLOCK);

    // Add the writer and verify read still indicates an empty (but not closed) buffer.
    auto writer = m_manager.createWriter(TEST_ATTACHMENT_ID_STRING_ONE);
    ASSERT_NE(writer, nullptr);
    readStatus = InProcessAttachmentReader::ReadStatus::OK;
    reader->read(result.data(), result.size(), &readStatus);
    ASSERT_EQ(readStatus, InProcessAttachmentReader::ReadStatus::OK_WOULDBLOCK);

    // Write some data and verify read succeeds.
    auto writeStatus = InProcessAttachmentWriter::WriteStatus::OK;
    auto numWritten = writer->write(testPattern.data(), testPattern.size(), &writeStatus);
    ASSERT_EQ(numWritten, testPattern.size());
    ASSERT_EQ(writeStatus, InProcessAttachmentWriter::WriteStatus::OK);
    readStatus = InProcessAttachmentReader::ReadStatus::OK;
    auto numRead = reader->read(result.data(), result.size(), &readStatus);
    ASSERT_EQ(readStatus, InProcessAttachmentReader::ReadStatus::OK);
    ASSERT_EQ(numRead, result.size());
}

/**
 * Test that the AttachmentManager's cleanup logic works as expected, and that it does not impact readers and
 * writers that are returned before the cleanup.
 */
TEST_F(AttachmentManagerTest, testAttachmentManagerTestCreateReadersThenWriters) {
    WriterVec writers;
    ReaderVec readers;

    createReaders(&readers);
    testReaders(readers, true);

    createWriters(&writers);
    testWriters(writers, true);
}

/**
 * Test that the AttachmentManager's cleanup logic works as expected, and that it does not impact readers and
 * writers that are returned before the cleanup.
 */
TEST_F(AttachmentManagerTest, testAttachmentManagerTestCreateWritersThenReaders) {
    WriterVec writers;
    ReaderVec readers;

    createWriters(&writers);
    testWriters(writers, true);

    createReaders(&readers);
    testReaders(readers, true);
}

/**
 * Verify an AttachmentManager can't create multiple writers.
 */
TEST_F(AttachmentManagerTest, testAttachmentManagerCreateMultipleWriters) {
    auto writer1 = m_manager.createWriter(TEST_ATTACHMENT_ID_STRING_ONE);
    auto writer2 = m_manager.createWriter(TEST_ATTACHMENT_ID_STRING_ONE);
    ASSERT_NE(writer1, nullptr);
    ASSERT_EQ(writer2, nullptr);
}

/**
 * Verify an AttachmentManager can't create multiple readers.
 */
TEST_F(AttachmentManagerTest, testAttachmentManagerCreateMultipleReaders) {
    auto reader1 = m_manager.createReader(TEST_ATTACHMENT_ID_STRING_ONE, utils::sds::ReaderPolicy::BLOCKING);
    auto reader2 = m_manager.createReader(TEST_ATTACHMENT_ID_STRING_ONE, utils::sds::ReaderPolicy::BLOCKING);
    ASSERT_NE(reader1, nullptr);
    ASSERT_EQ(reader2, nullptr);
}

/**
 * Test a one-pass write and read with both Attachment wrapper classes.
 */
TEST_F(AttachmentManagerTest, testAttachmentWriterAndReaderInOnePass) {
    auto writer = m_manager.createWriter(TEST_ATTACHMENT_ID_STRING_ONE);
    auto reader = m_manager.createReader(TEST_ATTACHMENT_ID_STRING_ONE, utils::sds::ReaderPolicy::BLOCKING);
    ASSERT_NE(writer, nullptr);
    ASSERT_NE(reader, nullptr);

    auto testPattern = createTestPattern(TEST_SDS_BUFFER_SIZE_IN_BYTES);

    auto writeStatus = InProcessAttachmentWriter::WriteStatus::OK;
    auto numWritten = writer->write(testPattern.data(), testPattern.size(), &writeStatus);
    ASSERT_EQ(numWritten, testPattern.size());
    ASSERT_EQ(writeStatus, InProcessAttachmentWriter::WriteStatus::OK);

    std::vector<uint8_t> result(testPattern.size());
    auto readStatus = InProcessAttachmentReader::ReadStatus::OK;
    auto numRead = reader->read(result.data(), result.size(), &readStatus);
    ASSERT_EQ(numRead, testPattern.size());
    ASSERT_EQ(readStatus, InProcessAttachmentReader::ReadStatus::OK);

    for (size_t i = 0; i < testPattern.size(); ++i) {
        EXPECT_EQ(result[i], testPattern[i]);
    }
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
