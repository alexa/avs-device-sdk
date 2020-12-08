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

#include <gtest/gtest.h>

#include "PlaylistParser/ContentDecrypter.h"

namespace alexaClientSDK {
namespace playlistParser {
namespace test {

using namespace avsCommon::utils::playlistParser;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils::sds;
using namespace ::testing;

/// Excepted decrypted content.
static const std::string DECRYPTED_STRING = "HelloWorld!\n";

/// HelloWorld!\n encrypted with below key and iv.
static const ByteVector AES_ENCRYPTED_CONTENT =
    {0xe8, 0xc2, 0x17, 0xa0, 0xa6, 0x95, 0x88, 0x39, 0xa3, 0x05, 0xa4, 0xfa, 0x42, 0x91, 0x52, 0x19};

/// Test key: aaaaaaaaaaaaaaaa.
static const ByteVector KEY(16, 0x61);

/// Test initialization vector: AAAAAAAAAAAAAAAA.
static const std::string HEX_IV = "0x41414141414141414141414141414141";

/// AES-128 EncryptionInfo with above key and iv.
static const auto AES_ENCRYPTION_INFO =
    EncryptionInfo(EncryptionInfo::Method::AES_128, "https://wwww.amazon.com/key.txt", HEX_IV);

/// Test class for ContentDecrypter class.
class ContentDecrypterTest : public ::testing::Test {
protected:
    /// Configure test instance.
    void SetUp() override;

    /// Tear down test instance.
    void TearDown() override;

    /**
     * Helper method to read a string from attachment reader.
     *
     * @param readSize The size of content that needs to be read.
     * @return The string that has been read from the attachment reader.
     */
    std::string readDecryptedContent(size_t readSize);

    /// Attachment used for writing decrypted content.
    std::shared_ptr<InProcessAttachment> m_attachment;

    /// Writer to write decrypted content.
    std::shared_ptr<AttachmentWriter> m_writer;

    /// Reader to read decrypted content.
    std::shared_ptr<AttachmentReader> m_reader;

    /// Instance of the @c IterativePlaylistParser.
    std::shared_ptr<ContentDecrypter> m_decrypter;

    /// Instance of the @c ID3TagsRemover.
    std::shared_ptr<Id3TagsRemover> m_id3TagsRemover;
};

void ContentDecrypterTest::SetUp() {
    m_attachment = std::make_shared<InProcessAttachment>("decryption");
    m_writer = m_attachment->createWriter(WriterPolicy::BLOCKING);
    m_reader = m_attachment->createReader(ReaderPolicy::NONBLOCKING);

    m_decrypter = std::make_shared<ContentDecrypter>();
    m_id3TagsRemover = std::make_shared<Id3TagsRemover>();
}

void ContentDecrypterTest::TearDown() {
    m_attachment.reset();
    m_writer.reset();
    m_reader.reset();

    m_decrypter->shutdown();
    m_decrypter.reset();

    m_id3TagsRemover->shutdown();
    m_id3TagsRemover.reset();
}

std::string ContentDecrypterTest::readDecryptedContent(size_t readSize) {
    std::vector<char> buffer(readSize, 0);
    auto readStatus = AttachmentReader::ReadStatus::OK;
    auto numRead = m_reader->read(buffer.data(), buffer.size(), &readStatus);

    EXPECT_EQ(readSize, numRead);
    return std::string(buffer.begin(), buffer.end());
}

TEST_F(ContentDecrypterTest, test_unsupportedEncryption) {
    auto noEncryption = EncryptionInfo();

    auto result = m_decrypter->decryptAndWrite(AES_ENCRYPTED_CONTENT, KEY, noEncryption, m_writer, m_id3TagsRemover);

    EXPECT_FALSE(result);
}

TEST_F(ContentDecrypterTest, test_invalidKeyEncryption) {
    /// Test key: aaaaaaaaaaaaaaa. Length is invalid.
    static const ByteVector INVALID_KEY(15, 0x61);

    auto result = m_decrypter->decryptAndWrite(
        AES_ENCRYPTED_CONTENT, INVALID_KEY, AES_ENCRYPTION_INFO, m_writer, m_id3TagsRemover);

    EXPECT_FALSE(result);
}

TEST_F(ContentDecrypterTest, test_invalidIVEncryption) {
    /// Test initialization vector: AAAAAAAAAAAAAAA. Length is invalid.
    static const std::string INVALID_HEX_IV = "0x414141414141414141414141414141";

    /// AES-128 EncryptionInfo with KEY key and INVALID_IV iv.
    static const auto INVALID_AES_ENCRYPTION_INFO =
        EncryptionInfo(EncryptionInfo::Method::AES_128, "https://wwww.amazon.com/key.txt", INVALID_HEX_IV);

    auto result = m_decrypter->decryptAndWrite(
        AES_ENCRYPTED_CONTENT, KEY, INVALID_AES_ENCRYPTION_INFO, m_writer, m_id3TagsRemover);

    EXPECT_FALSE(result);
}

TEST_F(ContentDecrypterTest, test_aESDecryption) {
    auto result =
        m_decrypter->decryptAndWrite(AES_ENCRYPTED_CONTENT, KEY, AES_ENCRYPTION_INFO, m_writer, m_id3TagsRemover);

    auto decryptedString = readDecryptedContent(DECRYPTED_STRING.size());
    EXPECT_TRUE(result);
    EXPECT_EQ(DECRYPTED_STRING, decryptedString);
}

TEST_F(ContentDecrypterTest, test_convertIVNullByteArray) {
    auto result = ContentDecrypter::convertIVToByteArray(HEX_IV, nullptr);

    EXPECT_FALSE(result);
}

TEST_F(ContentDecrypterTest, test_convertIVIncorrectLength) {
    ByteVector actualIV;

    auto result = ContentDecrypter::convertIVToByteArray("0x01", &actualIV);

    EXPECT_FALSE(result);
    EXPECT_TRUE(actualIV.empty());
}

TEST_F(ContentDecrypterTest, test_convertIVNotHex) {
    const std::string nonHEX_IV = "0101010101010101010101010101010101";
    ByteVector actualIV;

    auto result = ContentDecrypter::convertIVToByteArray(nonHEX_IV, &actualIV);

    EXPECT_FALSE(result);
    EXPECT_TRUE(actualIV.empty());
}

TEST_F(ContentDecrypterTest, test_convertIV) {
    ByteVector actualIV;

    auto result = ContentDecrypter::convertIVToByteArray(HEX_IV, &actualIV);

    EXPECT_TRUE(result);
    EXPECT_EQ("AAAAAAAAAAAAAAAA", std::string(actualIV.begin(), actualIV.end()));
}

}  // namespace test
}  // namespace playlistParser
}  // namespace alexaClientSDK
