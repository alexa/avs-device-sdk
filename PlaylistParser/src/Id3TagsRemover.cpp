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

#include <chrono>

#include "PlaylistParser/Id3TagsRemover.h"

#include <AVSCommon/Utils/ID3Tags/ID3v2Tags.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace playlistParser {

using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils::id3Tags;
using namespace avsCommon::utils::sds;

#define TAG "Id3TagsRemover"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The number of bytes read from the attachment with each read in the read loop.
static const std::size_t CHUNK_SIZE(1024);

/// Timeout for polling loops that check activity between read or write.
static const std::chrono::milliseconds WAIT_FOR_ACTIVITY_TIMEOUT{100};

Id3TagsRemover::Id3TagsRemover() : RequiresShutdown{"Id3TagsRemover"}, m_shuttingDown{false} {
}

bool Id3TagsRemover::removeTagsAndWrite(
    const std::shared_ptr<InProcessAttachment>& attachment,
    const std::shared_ptr<AttachmentWriter>& streamWriter) {
    if (!attachment) {
        ACSDK_ERROR(LX("removeTagsAndWriteFailed").d("reason", "nullattachment"));
        return false;
    }

    if (!streamWriter) {
        ACSDK_ERROR(LX("removeTagsAndWriteFailed").d("reason", "nullWriter"));
        return false;
    }

    auto reader = attachment->createReader(ReaderPolicy::BLOCKING);
    if (!reader) {
        ACSDK_ERROR(LX("removeTagsAndWriteFailed").d("reason", "nullReader"));
        return false;
    }

    auto readStatus = AttachmentReader::ReadStatus::OK;
    bool streamClosed = false;
    Context context;
    while (!streamClosed && !m_shuttingDown) {
        ByteVector buffer(CHUNK_SIZE, 0);
        auto bytesRead = reader->read(buffer.data(), buffer.size(), &readStatus, WAIT_FOR_ACTIVITY_TIMEOUT);
        buffer.resize(bytesRead);

        switch (readStatus) {
            case AttachmentReader::ReadStatus::CLOSED:
                streamClosed = true;
                context.isBufferComplete = true;
                if (0 == bytesRead && context.remainingBuffer.size() == 0) {
                    break;
                }
                /* FALL THROUGH - to add any data received even if closed */
            case AttachmentReader::ReadStatus::OK:
            case AttachmentReader::ReadStatus::OK_WOULDBLOCK:
            case AttachmentReader::ReadStatus::OK_TIMEDOUT:
                stripID3Tags(buffer, context);
                if (!writeBufferToWriter(buffer, streamWriter)) {
                    ACSDK_ERROR(LX("removeTagsAndWriteFailed").d("reason", "writeBufferToWriterFailed"));
                    return false;
                }
                break;
            case AttachmentReader::ReadStatus::OK_OVERRUN_RESET:
                // Current AttachmentReader policy renders this outcome impossible.
                ACSDK_ERROR(LX("removeTagsAndWriteFailed").d("reason", readStatus));
                break;
            case AttachmentReader::ReadStatus::ERROR_OVERRUN:
            case AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
            case AttachmentReader::ReadStatus::ERROR_INTERNAL:
                ACSDK_ERROR(LX("removeTagsAndWriteFailed").d("reason", readStatus));
                return false;
        }
    }
    return true;
}

void Id3TagsRemover::stripID3Tags(ByteVector& buffer) {
    Context context;
    context.isBufferComplete = true;
    stripID3Tags(buffer, context);
}

void Id3TagsRemover::stripID3Tags(ByteVector& buffer, Context& context) {
    // If there were remainingBuffer, prepend them to the buffer
    if (context.remainingBuffer.size() > 0) {
        ACSDK_DEBUG9(LX("Prepend remaining Buffer")
                         .d("remainingBuffer", context.remainingBuffer.size())
                         .d("buffer", buffer.size()));
        context.remainingBuffer.insert(context.remainingBuffer.end(), buffer.begin(), buffer.end());
        buffer.clear();
        buffer.swap(context.remainingBuffer);
    }

    std::size_t startPosition = 0;
    while (buffer.size() > 0 && !m_shuttingDown) {
        if (context.remainingBytesToStrip == 0) {
            auto it = std::search(
                buffer.begin() + startPosition,
                buffer.end(),
                std::begin(ID3V2TAG_IDENTIFIER),
                std::end(ID3V2TAG_IDENTIFIER));
            if (it != buffer.end()) {
                if (!context.isBufferComplete &&
                    std::distance(it, buffer.end()) <= static_cast<int>(ID3V2TAG_HEADER_SIZE)) {
                    ACSDK_DEBUG9(LX("Partial ID3 tags").d("distanceFromEnd", std::distance(it, buffer.end())));
                    context.remainingBuffer.insert(context.remainingBuffer.end(), it, buffer.end());
                    buffer.erase(it, buffer.end());
                    break;
                }

                auto index = std::distance(buffer.begin(), it) + startPosition;
                auto id3TagSize = getID3v2TagSize(buffer.data() + index, buffer.size() - index);
                if (id3TagSize > 0) {
                    context.remainingBytesToStrip = id3TagSize;
                    startPosition = index;
                } else {
                    // it doesn't match, shift one and search again
                    startPosition = index + sizeof(ID3V2TAG_IDENTIFIER);
                    // make sure the startPosition is still inbound, if not, break from loop.
                    if (startPosition >= buffer.size()) {
                        break;
                    }
                    continue;
                }
            } else {
                if (!context.isBufferComplete) {
                    // check if last characters are "ID" or "I" and put them in remainingBuffer if it's the case.
                    auto remainingIdentifier = sizeof(ID3V2TAG_IDENTIFIER) - 1;
                    for (auto i = remainingIdentifier; i > 0; --i) {
                        if (buffer.size() >= i) {
                            auto lastIt = std::search(
                                buffer.end() - i,
                                buffer.end(),
                                std::begin(ID3V2TAG_IDENTIFIER),
                                std::begin(ID3V2TAG_IDENTIFIER) + i);
                            if (lastIt != buffer.end()) {
                                ACSDK_DEBUG9(LX("Partial ID3 tags").d("i", i));
                                context.remainingBuffer.insert(context.remainingBuffer.end(), lastIt, buffer.end());
                                buffer.erase(lastIt, buffer.end());
                                break;
                            }
                        }
                    }
                }
                break;
            }
        }

        // Strip ID3 header if it exists
        if (context.remainingBytesToStrip > 0) {
            std::size_t strippedSize = 0;
            if (context.remainingBytesToStrip + startPosition >= buffer.size()) {
                context.remainingBytesToStrip -= (buffer.size() - startPosition);
                strippedSize = buffer.size() - startPosition;
            } else {
                strippedSize = context.remainingBytesToStrip;
                context.remainingBytesToStrip = 0;
            }
            ACSDK_DEBUG9(LX("ID3 header stripped")
                             .d("startPosition", startPosition)
                             .d("strippedSize", strippedSize)
                             .d("remainingBytesToStrip", context.remainingBytesToStrip)
                             .d("bytesRead", buffer.size()));
            if (strippedSize == 0) {
                break;
            }
            buffer.erase(buffer.begin() + startPosition, buffer.begin() + startPosition + strippedSize);
        }
    }
}

bool Id3TagsRemover::writeBufferToWriter(
    const ByteVector& buffer,
    const std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter>& writer) {
    if (buffer.size() == 0) {
        return true;
    }

    std::size_t targetNumBytes = buffer.size();
    std::size_t totalBytesWritten = 0;
    const unsigned char* data = buffer.data();

    while ((totalBytesWritten < targetNumBytes) && !m_shuttingDown) {
        auto writeStatus = avsCommon::avs::attachment::AttachmentWriter::WriteStatus::OK;

        std::size_t numBytesWritten =
            writer->write(data, targetNumBytes - totalBytesWritten, &writeStatus, WAIT_FOR_ACTIVITY_TIMEOUT);
        totalBytesWritten += numBytesWritten;
        data += numBytesWritten;

        switch (writeStatus) {
            case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::CLOSED:
            case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
            case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::ERROR_INTERNAL:
                return false;
            case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::TIMEDOUT:
            case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::OK:
                // might still have bytes to write
                continue;
            case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::OK_BUFFER_FULL:
                ACSDK_ERROR(LX(__func__).d("unexpected return code", writeStatus));
                return false;
        }
        ACSDK_ERROR(LX("UnexpectedWriteStatus").d("writeStatus", writeStatus));
        return false;
    }
    return true;
}

void Id3TagsRemover::doShutdown() {
    m_shuttingDown = true;
}

}  // namespace playlistParser
}  // namespace alexaClientSDK
