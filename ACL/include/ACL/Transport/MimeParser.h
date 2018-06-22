/*
 * Copyright 2016-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

/**
 * @file
 */
#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MIMEPARSER_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MIMEPARSER_H_

#include <cstddef>
#include <memory>
#include <iostream>
#include <set>
#include <string>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/AVS/Attachment/AttachmentWriter.h>

#include <MultipartParser/MultipartReader.h>

#include "ACL/Transport/MessageConsumerInterface.h"

namespace alexaClientSDK {
namespace acl {

class MimeParser {
public:
    /**
     * Values that express the result of a @c feed() call.
     */
    enum class DataParsedStatus {
        /// The most recent chunk of data was parsed ok.
        OK,
        /// The most recent chunk of data was not fully processed.
        INCOMPLETE,
        /// There was a problem handling the most recent chunk of data.
        ERROR
    };

    /**
     * Constructor.
     *
     * @param messageConsumer The MessageConsumerInterface which should receive messages from AVS.
     * @param attachmentManager The attachment manager that manages the attachment.
     */
    MimeParser(
        std::shared_ptr<MessageConsumerInterface> messageConsumer,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager);

    /**
     * Resets class for use in another transfer.
     */
    void reset();

    /**
     * Feeds chunk of MIME multipart stream into the underlying MIME multipart parser.
     * @param data pointer to chunk of data.
     * @param length length of data to feed.
     * @return A value expressing the final status of the read operation.
     */
    DataParsedStatus feed(char* data, size_t length);

    /**
     * Set the context ID to use when creating attachments.
     *
     * @param attachmentContextId The context ID to use when creating attachments.
     */
    void setAttachmentContextId(const std::string& attachmentContextId);

    /**
     * Sets the MIME multipart boundary string that the underlying mime multipart parser
     * uses.
     * @param boundaryString The MIME multipart boundary string
     */
    void setBoundaryString(const std::string& boundaryString);

    /**
     * Utility function to get the MessageConsumer object that the MimeParser is using.
     * The returned parameter's lifetime is guaranteed to be valid for the lifetime of the MimeParser object.
     * @return The MessageConsumer object being used by the MimeParser.
     */
    std::shared_ptr<MessageConsumerInterface> getMessageConsumer();

    /**
     * A utility function to close the currently active attachment writer, if there is one.
     */
    void closeActiveAttachmentWriter();

private:
    enum ContentType {
        /// The default value, indicating no data.
        NONE,

        /// The content represents a JSON formatted string.
        JSON,

        /// The content represents binary data.
        ATTACHMENT
    };

    /**
     * Callback that gets called when a multipart MIME part begins
     * @param headers The MIME headers for the upcoming MIME part
     * @param user A pointer to user set data (should always be an instance of this class)
     */
    static void partBeginCallback(const MultipartHeaders& headers, void* userData);

    /**
     * Callback that gets called when data from a MIME part is available
     * @param buffer A pointer to the chunk of data provided
     * @param size The size of the data provided
     * @param user A pointer to user set data (should always be an instance of this class)
     */
    static void partDataCallback(const char* buffer, size_t size, void* userData);

    /**
     * Callback that gets called when a multipart MIME part ends
     * @param user A pointer to user set data (should always be an instance of this class)
     */
    static void partEndCallback(void* userData);

    /**
     * Utility function to encapsulate the logic required to write data to an attachment.
     *
     * @param buffer The data to be written to the attachment.
     * @param size The size of the data to be written to the attachment.
     * @return A value expressing the final status of the write operation.
     */
    MimeParser::DataParsedStatus writeDataToAttachment(const char* buffer, size_t size);

    /**
     * Utility function to determine if the given number of bytes has been processed already by this mime parser
     * in a previous iteration.  This follows the idea that when processing n bytes, a mime parse may succeed
     * breaking out the first mime part, but fail on the second.  The caller may want to re-drive the same data in
     * order to parse all the data, and so the first bytes should not be re-processed.  This function clarifies if,
     * given the current state of the parser, these bytes have been already processed.
     *
     * @param size The number of bytes that can be processed.
     * @return Whether any of these bytes have yet to be processed.
     */
    bool shouldProcessBytes(size_t size) const;

    /**
     * Function to update the parser's state to capture that size bytes have been successfully processed.
     *
     * @param size The number of bytes that have been processed.
     */
    void updateCurrentByteProgress(size_t size);

    /**
     * Function to reset the tracking byte counters of the mime parser, which should be called after successfully
     * parsing a chunk of data.
     */
    void resetByteProgressCounters();

    /**
     * Remember if the attachment writer's buffer is full.
     *
     * @param isFull Whether the attachment writer's buffer is full.
     **/
    void setAttachmentWriterBufferFull(bool isFull);

    /// Tracks whether we've received our first block of data in the stream.
    bool m_receivedFirstChunk;
    /// Tracks the Content-Type of the current MIME part.
    ContentType m_currDataType;
    /// Instance of a multipart MIME reader.
    MultipartReader m_multipartReader;
    /// The object to report back to when JSON MIME parts are received.
    std::shared_ptr<MessageConsumerInterface> m_messageConsumer;
    /// The attachment manager.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> m_attachmentManager;
    /// The contextId, needed for creating attachments.
    std::string m_attachmentContextId;
    /**
     * The directive message being received from AVS by this stream.  It may be built up over several calls if either
     * the write quantums are small, or if the message is long.
     */
    std::string m_directiveBeingReceived;
    /**
     * The attachment id of the attachment currently being processed.  This variable is needed to prevent duplicate
     * creation of @c Attachment objects when data is re-driven.
     */
    std::string m_attachmentIdBeingReceived;
    /// The current AttachmentWriter.
    std::unique_ptr<avsCommon::avs::attachment::AttachmentWriter> m_attachmentWriter;
    /**
     * The status of the last feed() call.  This is required as a class data member because the callback functions
     * this class provides to MultiPartReader do not allow for a return value of our choosing.
     */
    DataParsedStatus m_dataParsedStatus;
    /**
     * In the context of pause & re-drive of the same set of data, this value reflects the current progress of the
     * mime parser over that data.
     */
    size_t m_currentByteProgress;
    /**
     * In the context of pause & re-drive of the same set of data, this value reflects how many of those bytes
     * have been successfully processed by the parser on any iteration.  On a re-drive of the same data, these bytes
     * should not be re-processed.
     */
    size_t m_totalSuccessfullyProcessedBytes;
    /// Records whether the attachment writer's buffer appears to be full.
    bool m_isAttachmentWriterBufferFull;
};

/**
 * Write a @c DataParsedStatus value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param status The status value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, MimeParser::DataParsedStatus status) {
    switch (status) {
        case MimeParser::DataParsedStatus::OK:
            stream << "OK";
            break;
        case MimeParser::DataParsedStatus::INCOMPLETE:
            stream << "INCOMPLETE";
            break;
        case MimeParser::DataParsedStatus::ERROR:
            stream << "ERROR";
            break;
    }
    return stream;
}

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MIMEPARSER_H_
