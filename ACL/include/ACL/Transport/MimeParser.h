/*
 * MimeParser.h
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXACLIENTSDK_ACL_INCLUDE_ACL_TRANSPORT_MIME_PARSER_H_
#define ALEXACLIENTSDK_ACL_INCLUDE_ACL_TRANSPORT_MIME_PARSER_H_

#include <cstddef>
#include <memory>
#include <iostream>
#include <string>
#include <MultipartParser/MultipartReader.h>

#include "ACL/AttachmentManager.h"
#include "ACL/Transport/MessageConsumerInterface.h"

namespace alexaClientSDK {
namespace acl {

class MimeParser {
public:
    /**
     * Constructor.
     *
     * @param messageConsumer The MessageConsumerInterface which should receive messages from AVS.
     * @param attachmentManager The attachment manager that manages the attachment.
     */
    MimeParser(MessageConsumerInterface *messageConsumer,
        std::shared_ptr<AttachmentManagerInterface> attachmentManager);

    /**
     * Resets class for use in another transfer.
     */
    void reset();

    /**
     * Feeds chunk of MIME multipart stream into the underlying MIME multipart parser.
     * @param data pointer to chunk of data
     * @param length length of data to feed
     */
    void feed(char *data, size_t length);

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
    MessageConsumerInterface* getMessageConsumer();

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
    static void partBeginCallback(const MultipartHeaders &headers, void *userData);

    /**
     * Callback that gets called when data from a MIME part is available
     * @param buffer A pointer to the chunk of data provided
     * @param size The size of the data provided
     * @param user A pointer to user set data (should always be an instance of this class)
     */
    static void partDataCallback(const char *buffer, size_t size, void *userData);

    /**
     * Callback that gets called when a multipart MIME part ends
     * @param user A pointer to user set data (should always be an instance of this class)
     */
    static void partEndCallback(void *userData);

    /// Tracks whether we've received our first block of data in the stream
    bool m_receivedFirstChunk;
    /// Tracks the Content-Type of the current MIME part
    ContentType m_currDataType;
    /// If the current MIME part is JSON, stores the payload.
    std::string m_message;
    /// Buffers an incoming attachment
    std::shared_ptr<std::iostream> m_attachment;
    /// Instance of a multipart MIME parser
    MultipartReader m_multipartReader;
    /// The object to report back to when JSON MIME parts are received.
    MessageConsumerInterface *m_messageConsumer;
    /// The attachment manager.
    std::shared_ptr<AttachmentManagerInterface> m_attachmentManager;
};

} // acl
} // alexaClientSDK

#endif // ALEXACLIENTSDK_ACL_INCLUDE_ACL_TRANSPORT_MIME_PARSER_H_
