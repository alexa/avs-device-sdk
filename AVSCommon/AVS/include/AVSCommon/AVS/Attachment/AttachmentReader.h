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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTREADER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTREADER_H_

#include <chrono>
#include <cstddef>
#include <ostream>

#include "AVSCommon/Utils/SDS/ReaderPolicy.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

/**
 * A class that provides functionality to read data from an @c Attachment.
 */
class AttachmentReader {
public:
    /**
     * An enum class to communicate the possible states following a @c read() call.
     */
    enum class ReadStatus {
        /// Things are ok.
        OK,
        /// On a request for n bytes, less than n bytes were available on a non-blocking read.
        OK_WOULDBLOCK,
        /// On a request for n bytes, less than n bytes were available on a blocking read.
        OK_TIMEDOUT,
        /// The writer has overwritten the new data on reader's current position. Reader position is reset
        /// to current writer position.
        OK_OVERRUN_RESET,
        /// The underlying data representation is no longer readable.
        CLOSED,
        /// The writer has corrupted the reader data.  The attachment is no longer valid.
        ERROR_OVERRUN,
        /// The number of bytes in the request is smaller than the word-size of the underlying data representation.
        ERROR_BYTES_LESS_THAN_WORD_SIZE,
        /// A non-specified error occurred.
        ERROR_INTERNAL
    };

    /// An enum class to indicate when the @c read() function should stop returning data after a call to @c close().
    enum class ClosePoint {
        /// Stop returning data immediately after the @c close() call.
        IMMEDIATELY,
        /// Stop returning data when all of the data in the buffer at the time @c close() was called has been read.
        AFTER_DRAINING_CURRENT_BUFFER
    };

    /*
     * Destructor.
     */
    virtual ~AttachmentReader() = default;

    /**
     * The read function.
     *
     * @param buf The buffer where data should be copied to.
     * @param numBytes The size of the buffer in bytes.
     * @param[out] readStatus The out-parameter where the resulting state of the read will be expressed.
     * @param timeoutMs The timeout for this read call in milliseconds.  This value is only used for the @c BLOCKING
     * reader policy.  If this parameter is zero, there is no timeout and blocking reads will wait forever.
     * @return The number of bytes read as a result of this call.
     */
    virtual std::size_t read(
        void* buf,
        std::size_t numBytes,
        ReadStatus* readStatus,
        std::chrono::milliseconds timeoutMs = std::chrono::milliseconds(0)) = 0;

    /**
     * The seek function.
     *
     * @param offset The offset to seek to within the @c Attachment.
     * @return @c true if the specified position points at unexpired data, or @c false otherwise. Note that it is valid
     * to seek into a future index that has not been written to yet.
     */
    virtual bool seek(uint64_t offset) = 0;

    /**
     * Utility function to return the number of bytes in an attachment.
     *
     * @return Number of unread bytes in the attachment by this attachment reader.
     */
    virtual uint64_t getNumUnreadBytes() = 0;

    /**
     * The close function.  An implementation will take care of any resource management when a reader no longer
     * needs to use an attachment.
     *
     * @param closePoint The point at which the reader should stop reading from the attachment.
     */
    virtual void close(ClosePoint closePoint = ClosePoint::AFTER_DRAINING_CURRENT_BUFFER) = 0;
};

/**
 * Write an @c Attachment::ReadStatus value to the given stream.
 *
 * @param stream The stream to write the value to.
 * @param status The value to write to the stream as a string.
 * @return The stream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const AttachmentReader::ReadStatus& status) {
    switch (status) {
        case AttachmentReader::ReadStatus::OK:
            stream << "OK";
            break;
        case AttachmentReader::ReadStatus::OK_WOULDBLOCK:
            stream << "OK_WOULDBLOCK";
            break;
        case AttachmentReader::ReadStatus::OK_TIMEDOUT:
            stream << "OK_TIMEDOUT";
            break;
        case AttachmentReader::ReadStatus::OK_OVERRUN_RESET:
            stream << "OK_OVERRUN_RESET";
            break;
        case AttachmentReader::ReadStatus::CLOSED:
            stream << "CLOSED";
            break;
        case AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
            stream << "ERROR_BYTES_LESS_THAN_WORD_SIZE";
            break;
        case AttachmentReader::ReadStatus::ERROR_OVERRUN:
            stream << "ERROR_OVERRUN";
            break;
        case AttachmentReader::ReadStatus::ERROR_INTERNAL:
            stream << "ERROR_INTERNAL";
            break;
    }
    return stream;
}

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTREADER_H_
