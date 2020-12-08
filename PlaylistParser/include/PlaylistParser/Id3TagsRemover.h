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

#ifndef ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_ID3TAGSREMOVER_H_
#define ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_ID3TAGSREMOVER_H_

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

#include <AVSCommon/AVS/Attachment/InProcessAttachment.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachmentReader.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachmentWriter.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace playlistParser {

/**
 * Helper class to remove ID3v2 tags in media content.
 */
class Id3TagsRemover : public avsCommon::utils::RequiresShutdown {
public:
    /// Alias for bytes.
    using ByteVector = std::vector<unsigned char>;

    /**
     * Constructor
     *
     */
    Id3TagsRemover();

    /// @name RequiresShutdown methods.
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * A function that read from an attachment and remove ID3 tags from the stream and write the stream back to the
     * @c streamWriter.
     *
     * @param attachment The attachment that contains the stream content.
     * @param streamWriter The writer to write to the attachment after ID3 tags are removed.
     * @return @c true if succeeds and @c false otherwise.
     */
    bool removeTagsAndWrite(
        const std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment>& attachment,
        const std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter>& streamWriter);

    /**
     * A function that removes any ID3 tags from the buffer.  After the call of this function, all ID3 tags in @c
     * buffer is removed.  If there is no ID3 tag found, then the content in the @c buffer remains the same.
     *
     * @param[in,out] buffer The buffer in which the ID3 tags will be removed.
     */
    void stripID3Tags(ByteVector& buffer);

private:
    /// A struct that is used internally in @c Id3TagsRemover to keep track of states.
    struct Context {
        /// Buffer that matches partial ID3 tags from last buffer read.
        ByteVector remainingBuffer;

        /// Remaining bytes to strip that is part of the ID3 tag.
        std::size_t remainingBytesToStrip;

        /// A flag to indicate if the content in the buffer is complete.
        bool isBufferComplete;

        /// Constructor
        Context() : remainingBytesToStrip{0}, isBufferComplete{false} {};
    };

    /**
     * A function that removes any ID3 tags from the buffer.
     *
     * @param[in,out] buffer The buffer to remove ID3 tags.
     * @param[in,out] context A internally structure to keep track of states.
     */
    void stripID3Tags(ByteVector& buffer, Context& context);

    /**
     * A helper function to write a buffer to the writer.
     *
     * @param buffer The buffer to write to the writer.
     * @param writer The writer to use to write the buffer to the underlying attachment.
     * @return @c true if succeeds and @c false otherwise.
     */
    bool writeBufferToWriter(
        const ByteVector& buffer,
        const std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter>& writer);

    /// Flag to indicate if a shutdown is occurring.
    std::atomic<bool> m_shuttingDown;
};

}  // namespace playlistParser
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_ID3TAGSREMOVER_H_
