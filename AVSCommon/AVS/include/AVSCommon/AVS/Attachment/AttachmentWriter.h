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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTWRITER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTWRITER_H_

#include <chrono>
#include <cstddef>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

/**
 * A class that provides functionality to write data to an @c Attachment.
 */
class AttachmentWriter {
public:
    /**
     * An enum class to communicate the possible states following a @c write() call.
     */
    enum class WriteStatus {
        /// Things are ok.
        OK,
        /// The underlying data representation is no longer writeable.
        CLOSED,
        /// The write could not succeed due to the underlying buffer being full.
        OK_BUFFER_FULL,
        /// The number of bytes in the request is smaller than the word-size of the underlying data representation. This
        /// is only posible if the policy is ALL_OR_NOTHING.
        ERROR_BYTES_LESS_THAN_WORD_SIZE,
        /// A non-specified error occurred.
        ERROR_INTERNAL,
        /// The write timed out. This is only possible if the writer policy is BLOCKING.
        TIMEDOUT
    };

    /**
     * Destructor.
     */
    virtual ~AttachmentWriter() = default;

    /**
     * The write function.
     *
     * @param buf The buffer where data should be copied from.
     * @param numBytes The size of the buffer in bytes.
     * @param[out] writeStatus The out-parameter where the resulting state of the write will be expressed.
     * @param timeout The maximum time to wait (if @c policy is @c BLOCKING) for space to write into.  If this parameter
     *     is zero, there is no timeout and blocking writes will wait forever.  If @c policy is not @C BLOCKING, this
     *     parameter is ignored.
     * @return The number of bytes written as a result of this call.
     */
    virtual std::size_t write(
        const void* buf,
        std::size_t numBytes,
        WriteStatus* writeStatus,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) = 0;

    /**
     * The close function.  An implementation will take care of any resource management when a writer no longer
     * needs to use an attachment.
     */
    virtual void close() = 0;
};

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTWRITER_H_
