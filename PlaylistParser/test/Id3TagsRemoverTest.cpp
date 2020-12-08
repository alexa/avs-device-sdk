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

#include <string>
#include <thread>

#include <gtest/gtest.h>

#include "PlaylistParser/Id3TagsRemover.h"

namespace alexaClientSDK {
namespace playlistParser {
namespace test {

/// Alias for bytes.
using ByteVector = std::vector<unsigned char>;

/// A timeout for the attachment reader
static const std::chrono::milliseconds WAIT_FOR_READ_TIMEOUT{1000};

/// An valid ID3 tag with tag size 11 (header + 1).
static const ByteVector VALID_ID3_TAG{'I', 'D', '3', 4, 0, 0, 0, 0, 0, 1};

using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils::sds;
using namespace ::testing;

/// Test class for Id3TagsRemoverTest class.
class Id3TagsRemoverTest : public ::testing::Test {
protected:
    /// Configure test instance.
    void SetUp() override;

    /// Tear down test instance.
    void TearDown() override;

    /**
     * Helper method to write content1 and content2 (if exist) into @c m_contentAttachment, and read data from
     * @c m_removerAttachment and check if the result are expected.
     *
     * @param content1 The content to write to m_contentAttachment.
     * @param expectedResult The expected result after ID3 tags are removed.
     * @param content2 The content (if exist) to write to m_contentAttachment.
     */
    void readContentAftersRemoval(
        const ByteVector& content1,
        const ByteVector& expectedResult,
        const ByteVector& content2 = ByteVector{});

    /// Attachment used origin content.
    std::shared_ptr<InProcessAttachment> m_contentAttachment;

    /// Writer to write content with tags removed.
    std::shared_ptr<AttachmentWriter> m_contentWriter;

    /// Attachment used by ID3 tag remover.
    std::shared_ptr<InProcessAttachment> m_removerAttachment;

    /// Writer to write content with tags removed.
    std::shared_ptr<AttachmentWriter> m_removerWriter;

    /// Reader to read content with tags removed.
    std::shared_ptr<AttachmentReader> m_removerReader;

    /// Instance of the @c Id3TagsRemover.
    std::shared_ptr<Id3TagsRemover> m_id3TagsRemover;
};

void Id3TagsRemoverTest::SetUp() {
    m_contentAttachment = std::make_shared<InProcessAttachment>("content");
    m_contentWriter = m_contentAttachment->createWriter(WriterPolicy::BLOCKING);

    m_removerAttachment = std::make_shared<InProcessAttachment>("remover");
    m_removerWriter = m_removerAttachment->createWriter(WriterPolicy::BLOCKING);
    m_removerReader = m_removerAttachment->createReader(ReaderPolicy::BLOCKING);

    m_id3TagsRemover = std::make_shared<Id3TagsRemover>();
}

void Id3TagsRemoverTest::TearDown() {
    m_contentAttachment.reset();
    m_contentWriter.reset();

    m_removerAttachment.reset();
    m_removerWriter.reset();
    m_removerReader.reset();

    m_id3TagsRemover->shutdown();
    m_id3TagsRemover.reset();
}

void Id3TagsRemoverTest::readContentAftersRemoval(
    const ByteVector& content1,
    const ByteVector& expectedResult,
    const ByteVector& content2) {
    std::thread writerThread([this, &content1, &content2, &expectedResult]() {
        m_id3TagsRemover->removeTagsAndWrite(m_contentAttachment, m_removerWriter);

        ByteVector buffer(content1.size() + content2.size(), 0);
        auto readStatus = AttachmentReader::ReadStatus::OK;
        auto numRead = m_removerReader->read(buffer.data(), buffer.size(), &readStatus, WAIT_FOR_READ_TIMEOUT);
        buffer.resize(numRead);

        EXPECT_EQ(buffer.size(), expectedResult.size());
        EXPECT_EQ(buffer, expectedResult);
    });

    auto writeStatus = avsCommon::avs::attachment::AttachmentWriter::WriteStatus::OK;
    m_contentWriter->write(content1.data(), content1.size(), &writeStatus);
    if (content2.size() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        m_contentWriter->write(content2.data(), content2.size(), &writeStatus);
    }
    m_contentWriter->close();
    if (writerThread.joinable()) {
        writerThread.join();
    }
}

TEST_F(Id3TagsRemoverTest, test_validID3Tag) {
    ByteVector buffer{VALID_ID3_TAG};

    // valid ID3 tag says ID3 tag contains 1 byte of data, so after remover, it should contain 3 bytes
    ByteVector testData{'a', 'b', 'c', 'd'};
    buffer.insert(buffer.end(), testData.begin(), testData.end());

    m_id3TagsRemover->stripID3Tags(buffer);

    // Expect 'b', 'c', 'd'
    testData.erase(testData.begin());
    EXPECT_EQ(buffer.size(), testData.size());
    EXPECT_EQ(buffer, testData);
}

TEST_F(Id3TagsRemoverTest, test_validID3TagWithOffset) {
    ByteVector buffer{'a'};
    buffer.insert(buffer.end(), VALID_ID3_TAG.begin(), VALID_ID3_TAG.end());

    // valid ID3 tag says ID3 tag contains 1 byte of data, so after remover, it should contain 3 bytes
    ByteVector testData{'a', 'b', 'c', 'd'};
    buffer.insert(buffer.end(), testData.begin(), testData.end());

    m_id3TagsRemover->stripID3Tags(buffer);

    // Expect 'a', 'b', 'c', 'd'
    EXPECT_EQ(buffer.size(), testData.size());
    EXPECT_EQ(buffer, testData);
}

TEST_F(Id3TagsRemoverTest, test_twoValidID3Tag) {
    ByteVector buffer{VALID_ID3_TAG};

    // valid ID3 tag says ID3 tag contains 1 byte of data, so after remover, it should contain 3 bytes
    ByteVector testData{'a', 'b', 'c', 'd'};
    buffer.insert(buffer.end(), testData.begin(), testData.end());
    buffer.insert(buffer.end(), VALID_ID3_TAG.begin(), VALID_ID3_TAG.end());
    buffer.insert(buffer.end(), testData.begin(), testData.end());

    m_id3TagsRemover->stripID3Tags(buffer);

    // Expect 'b', 'c', 'd', 'b', 'c', 'd'
    testData.erase(testData.begin());
    testData.insert(testData.end(), testData.begin(), testData.end());
    EXPECT_EQ(buffer.size(), testData.size());
    EXPECT_EQ(buffer, testData);
}

TEST_F(Id3TagsRemoverTest, test_invalidID3Tag) {
    ByteVector buffer{'I', 'D', '3', 99, 88, 77, 66, 55, 44, 33, 22};
    ByteVector expectedResult{buffer};

    m_id3TagsRemover->stripID3Tags(buffer);

    // Expect 'I', 'D', '3', 99, 88, 77
    EXPECT_EQ(buffer.size(), expectedResult.size());
    EXPECT_EQ(buffer, expectedResult);
}

TEST_F(Id3TagsRemoverTest, test_partialID3Tag) {
    ByteVector bufferID3{'I', 'D', '3'};
    ByteVector expectedResultID3{bufferID3};

    m_id3TagsRemover->stripID3Tags(bufferID3);

    // Expect 'I', 'D', '3'
    EXPECT_EQ(bufferID3.size(), expectedResultID3.size());
    EXPECT_EQ(bufferID3, expectedResultID3);

    ByteVector bufferID{'I', 'D'};
    ByteVector expectedResultID{bufferID};

    m_id3TagsRemover->stripID3Tags(bufferID);

    // Expect 'I', 'D'
    EXPECT_EQ(bufferID.size(), expectedResultID.size());
    EXPECT_EQ(bufferID, expectedResultID);

    ByteVector bufferI{'I'};
    ByteVector expectedResultI{bufferI};

    m_id3TagsRemover->stripID3Tags(bufferI);

    // Expect 'I', 'D'
    EXPECT_EQ(bufferI.size(), expectedResultI.size());
    EXPECT_EQ(bufferI, expectedResultI);
}

TEST_F(Id3TagsRemoverTest, test_attachmentValidID3Tag) {
    ByteVector buffer{VALID_ID3_TAG};

    // valid ID3 tag says ID3 tag contains 1 byte of data, so after remover, it should contain 3 bytes
    ByteVector testData{'a', 'b', 'c', 'd'};
    buffer.insert(buffer.end(), testData.begin(), testData.end());

    ByteVector expectedResult{'b', 'c', 'd'};
    readContentAftersRemoval(buffer, expectedResult);
}

TEST_F(Id3TagsRemoverTest, test_attachmentTwoValidID3Tag) {
    ByteVector buffer{VALID_ID3_TAG};

    // valid ID3 tag says ID3 tag contains 1 byte of data, so after remover, it should contain 3 bytes
    ByteVector testData{'a', 'b', 'c', 'd'};
    buffer.insert(buffer.end(), testData.begin(), testData.end());
    buffer.insert(buffer.end(), VALID_ID3_TAG.begin(), VALID_ID3_TAG.end());
    buffer.insert(buffer.end(), testData.begin(), testData.end());

    // Expect 'b', 'c', 'd', 'b', 'c', 'd'
    testData.erase(testData.begin());
    testData.insert(testData.end(), testData.begin(), testData.end());

    readContentAftersRemoval(buffer, testData);
}

TEST_F(Id3TagsRemoverTest, test_attachmentPartialID3Tag) {
    ByteVector bufferID3{'I', 'D', '3'};
    ByteVector expectedResultID3{bufferID3};

    readContentAftersRemoval(bufferID3, expectedResultID3);
}

TEST_F(Id3TagsRemoverTest, test_attachmentCompleteID3Tag) {
    ByteVector bufferID3{VALID_ID3_TAG};
    ByteVector expectedResultID3{};

    readContentAftersRemoval(bufferID3, expectedResultID3);
}

TEST_F(Id3TagsRemoverTest, test_attachmentID3TagAcrossTwoWrites) {
    ByteVector content1{'a', 'b', 'c', 'I'};
    ByteVector content2{VALID_ID3_TAG};
    content2.erase(content2.begin());
    content2.insert(content2.end(), content1.begin(), content1.end());

    ByteVector expectedResult{'a', 'b', 'c', 'b', 'c', 'I'};

    readContentAftersRemoval(content1, expectedResult, content2);
}

TEST_F(Id3TagsRemoverTest, test_attachmentID3Tag9MatchingAcrossTwoWrites) {
    ByteVector content1{VALID_ID3_TAG};
    content1.erase(content1.end() - 1);
    ByteVector content2{1, 'b', 'c', 'I'};

    ByteVector expectedResult{'c', 'I'};

    readContentAftersRemoval(content1, expectedResult, content2);
}

TEST_F(Id3TagsRemoverTest, test_attachmentID3TagAcrossFullHeaderTwoWrites) {
    ByteVector content1{VALID_ID3_TAG};
    ByteVector content2{'a', 'b', 'c', 'I'};

    ByteVector expectedResult{'b', 'c', 'I'};

    readContentAftersRemoval(content1, expectedResult, content2);
}

TEST_F(Id3TagsRemoverTest, test_attachmentInvalidID3TagAcrossTwoWrites) {
    ByteVector content1{'a', 'b', 'c', 'I'};
    ByteVector content2{'D', 'a', 'b', 'c'};

    ByteVector expectedResult{'a', 'b', 'c', 'I', 'D', 'a', 'b', 'c'};

    readContentAftersRemoval(content1, expectedResult, content2);
}

TEST_F(Id3TagsRemoverTest, test_attachmentID3TagRemove10BytesAcrossTwoWrites) {
    ByteVector content1{{'I', 'D', '3', 4, 0, 0, 0, 0, 0, 10, '1', '2', '3', '4'}};
    ByteVector content2{'5', '6', '7', '8', '9', 'a', 'b', 'c'};

    ByteVector expectedResult{'b', 'c'};

    readContentAftersRemoval(content1, expectedResult, content2);
}

TEST_F(Id3TagsRemoverTest, test_attachmentID3TagBoundary1AcrossTwoWrites) {
    ByteVector content1{'I'};
    ByteVector content2{'D', '3', 4, 0, 0, 0, 0, 0, 1, '1', '2'};

    ByteVector expectedResult{'2'};

    readContentAftersRemoval(content1, expectedResult, content2);
}

TEST_F(Id3TagsRemoverTest, test_attachmentID3TagBoundary2AcrossTwoWrites) {
    ByteVector content1{'I', 'D'};
    ByteVector content2{'3', 3, 0, 0, 0, 0, 0, 1, '1', '2'};

    ByteVector expectedResult{'2'};

    readContentAftersRemoval(content1, expectedResult, content2);
}

TEST_F(Id3TagsRemoverTest, test_attachmentID3TagBoundary3AcrossTwoWrites) {
    ByteVector content1{'I', 'D', '3'};
    ByteVector content2{'1', '2'};

    ByteVector expectedResult{'I', 'D', '3', '1', '2'};

    readContentAftersRemoval(content1, expectedResult, content2);
}

TEST_F(Id3TagsRemoverTest, test_attachmentID3TagRemoveTagAcrossTwoBoundaries) {
    ByteVector content1{'1', '2', 'I', 'D', '3', 4, 0, 0, 0, 0, 0, 5, '1', '2'};

    ByteVector content2{'3', '4', '5', '6', '7'};

    ByteVector expectedResult{'1', '2', '6', '7'};

    readContentAftersRemoval(content1, expectedResult, content2);
}

}  // namespace test
}  // namespace playlistParser
}  // namespace alexaClientSDK
