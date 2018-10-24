/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2MIMEREQUESTENCODER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2MIMEREQUESTENCODER_H_

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "AVSCommon/Utils/HTTP2/HTTP2MimeRequestSourceInterface.h"
#include "AVSCommon/Utils/HTTP2/HTTP2RequestSourceInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

/**
 * Class that adapts between HTTP2MimeRequestSourceInterface and a
 * HTTP2RequestSourceInterface providing the encoding of discreet mime parts in
 * to a single request stream.
 */
class HTTP2MimeRequestEncoder : public HTTP2RequestSourceInterface {
public:
    /**
     * Create an HTTP2MimeRequestEncoder.
     *
     * @param boundary The mime boundary to include between mime parts.
     * @param source Pointer to an object providing the mime parts in sequence.
     */
    HTTP2MimeRequestEncoder(const std::string& boundary, std::shared_ptr<HTTP2MimeRequestSourceInterface> source);

    /**
     * Destructor.
     */
    ~HTTP2MimeRequestEncoder() = default;

    /// @name HTTP2RequestSourceInterface methods.
    /// @{
    HTTP2SendDataResult onSendData(char* bytes, size_t size) override;
    std::vector<std::string> getRequestHeaderLines() override;
    /// @}

private:
    /// The states that the encoder transitions through.
    enum class State {
        /// Just created
        NEW,
        /// Requesting the source for the headers for the first mime part.
        GETTING_1ST_PART_HEADERS,
        /// Sending the boundary before the first part.
        SENDING_1ST_BOUNDARY,
        /// Sending the headers for the current part.
        SENDING_PART_HEADERS,
        /// Sending data for the current part.
        SENDING_PART_DATA,
        /// Sending the boundary terminating the current part.
        SENDING_END_BOUNDARY,
        /// Requesting the source for the headers for the next part.
        GETTING_NTH_PART_HEADERS,
        /// Sending the CRLF between the latest boundary and the next part.
        SENDING_CRLF_AFTER_BOUNDARY,
        /// Sending the two dashes after the final boundary.
        SENDING_TERMINATING_DASHES,
        /// Done sending.
        DONE,
        /// Bad state.
        ABORT
    };

    // Friend operator<<() to enable debug output.
    friend std::ostream& operator<<(std::ostream& stream, State state);

    /**
     * Set the state of the encoder.
     *
     * @param newState The new state to transition to.
     */
    void setState(State newState);

    /**
     * Copy a string in to the provided buffer, starting at the offset @m_stringIndex into the buffer,
     * truncating the copy if necessary to not exceed the size of the buffer.  @c m_stringIndex will
     * be updated to the next byte after the copied string, and @c m_bytesCopied will have the count
     * of copied bytes added to it.
     *
     * @param bytes The buffer to copy the string to.
     * @param size The size of the buffer to copy the string to.
     * @param text The string to send.
     * @return Whether the end of the string was sent.
     */
    bool sendString(char* bytes, size_t size, const std::string& text);

    /**
     * Copy a string and a CRLF in to the provided buffer, starting at the offset @m_stringIndex into the buffer,
     * truncating the copy if necessary to not exceed the size of the buffer.  @c m_stringIndex will
     * be updated to the next byte after the copied string and CRLF, and @c m_bytesCopied will have the count
     * of copied bytes added to it.
     *
     * @param bytes The buffer to copy the string to.
     * @param size The size of the buffer to copy the string to.
     * @param text The string to send.
     * @return Whether the end of the string was sent.
     */
    bool sendStringAndCRLF(char* bytes, size_t size, const std::string& text);

    /**
     * Create a HTTP2SendDataResult with HTTP2SendStatus::CONTINUE and a size of @c m_bytesCopied.
     *
     * @return An HTTP2SendDataResult with HTTP2SendStatus::CONTINUE and a size of @c m_bytesCopied.
     */
    HTTP2SendDataResult continueResult();

    /// Current state.
    State m_state;

    /// The boundry string without a CRLF or two-dash prefix.
    std::string m_rawBoundary;

    /// The boundary string with CRLF nd two-dash prefix to simplify emitting it in the encoded stream.
    std::string m_prefixedBoundary;

    /// Shared pointer to the MimeRequestSource implementation.
    std::shared_ptr<HTTP2MimeRequestSourceInterface> m_source;

    /// Number of bytes accumulated in @c bytes during call to @c onSendData().
    size_t m_bytesCopied;

    /// Last result from calling getMimeHeaderLines.
    HTTP2GetMimeHeadersResult m_getMimeHeaderLinesResult;

    /// Iterator to header line currently being sent.
    std::vector<std::string>::const_iterator m_headerLine;

    /// Current index in to @c m_boundary or the current header line.
    size_t m_stringIndex;
};

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2MIMEREQUESTENCODER_H_
