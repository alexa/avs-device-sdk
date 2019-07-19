/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_CONTENTDECRYPTER_H_
#define ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_CONTENTDECRYPTER_H_

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <AVSCommon/AVS/Attachment/AttachmentWriter.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include "PlaylistParser/PlaylistParser.h"

namespace alexaClientSDK {
namespace playlistParser {

/// Alias for bytes.
typedef std::vector<unsigned char> ByteVector;

/**
 * Helper class to decrypt downloaded media content.
 */
class ContentDecrypter : public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Constructor
     *
     */
    ContentDecrypter();

    /**
     * Initializes media initialization section.
     *
     * @param mediaInitSection The Media initialization section.
     * @param streamWriter The writer to write media init section.
     */
    void writeMediaInitSection(
        const ByteVector& mediaInitSection,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> streamWriter);

    /**
     * Decrypts contents and writes to stream.
     *
     * @param encryptedContent The content that needs to be decrypted.
     * @param key The encryption key.
     * @param encryptionInfo The @c EncryptionInfo of the encrypted content.
     * @param streamWriter The writer to write decrypted content.
     * @return @c true if decryption and write to stream is successful or @c false otherwise.
     */
    bool decryptAndWrite(
        const ByteVector& encryptedContent,
        const ByteVector& key,
        const avsCommon::utils::playlistParser::EncryptionInfo& encryptionInfo,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> streamWriter);

    /**
     * Converts initialization vector from hex to byte array.
     *
     * @param hexIV The initialization vector in HEX.
     * @param[out] ivByteArray Pointer to result byte array if successful.
     * @return @c true if conversion is successful or @c false if failed.
     */
    static bool convertIVToByteArray(const std::string& hexIV, ByteVector* ivByteArray);

    /// @name RequiresShutdown methods.
    /// @{
    void doShutdown() override;
    /// @}

private:
    /**
     * Prepends media initialization section to content.
     *
     * @param bytes The bytes that needs to be added after media initialization section.
     */
    ByteVector prependMediaInitSection(const ByteVector& bytes) const;

    /**
     * Decrypts AES-128 encrypted content.
     *
     * @param encryptedContent The content that needs to be decrypted.
     * @param key The encryption key.
     * @param iv The initialization vector of the encryption content.
     * @param[out] decryptedContent The result of decryption is written to this pointer if successful.
     * @param usePadding Flag to set if padding is required, default is @c true.
     * @return number of decrypted bytes if successful or 0 otherwise.
     */
    int decryptAES(
        const ByteVector& encryptedContent,
        const ByteVector& key,
        const ByteVector& iv,
        ByteVector* decryptedContent,
        bool usePadding = true);

    /**
     * Decrypted SAMPLE-AES encrypted content.
     *
     * @param encryptedContent The content that needs to be decrypted.
     * @param key The encryption key.
     * @param iv The initialization vector of the encryption content.
     * @param[out] decryptedContent The result of decryption is written to this pointer if successful.
     * @return number of decrypted bytes if successful or 0 otherwise.
     */
    bool decryptSampleAES(
        const ByteVector& encryptedContent,
        const ByteVector& key,
        const ByteVector& iv,
        ByteVector* decryptedContent);

    /**
     * Writes result to stream.
     *
     * @param content The content to be written to stream.
     * @param streamWriter The writer to write content.
     * @return @c true if conversion is successful or @c false if failed.
     */
    bool writeToStream(
        const ByteVector& content,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> streamWriter);

    /**
     *  Helper function to get more descriptive lib av errors
     *
     *  @param event The name of the log event.
     *  @param reason The reason for logging.
     *  @param averror The error code reported by FFMpeg library.
     */
    static void logAVError(const char* event, const char* reason, int averror);

    /// Byte buffer for FFMpeg library.
    struct ByteBuffer {
        /// Pointer to input data.
        uint8_t* data;

        /// Length of input data.
        int len;

        /// Current position for accessing data.
        int pos;
    };

    /**
     * Helper function to calculate bytes remaining
     *
     * @param pos The position of the pointer in buffer.
     * @param end The end position of the pointer in buffer.
     * @return The number of bytes between end position and current position.
     */
    static int bytesRemaining(uint8_t* pos, uint8_t* end);

    /// Media initialization section
    ByteVector m_mediaInitSection;

    /// Flag to indicate if a shutdown is occurring.
    std::atomic<bool> m_shuttingDown;
};

}  // namespace playlistParser
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_CONTENTDECRYPTER_H_
