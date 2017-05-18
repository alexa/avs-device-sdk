/*
 * MimeParser.cpp
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
#include "AVSUtils/Logging/Logger.h"
#include "ACL/Transport/MimeParser.h"
#include <sstream>


namespace alexaClientSDK {
namespace acl {
using namespace avsUtils;

/// MIME field name for a part's MIME type
static const std::string MIME_CONTENT_TYPE_FIELD_NAME = "Content-Type";
/// MIME field name for a part's reference id
static const std::string MIME_CONTENT_ID_FIELD_NAME = "Content-ID";
/// MIME type for JSON payloads
static const std::string MIME_JSON_CONTENT_TYPE = "application/json";
/// MIME type for binary streams
static const std::string MIME_OCTET_STREAM_CONTENT_TYPE = "application/octet-stream";

/**
 *  Sanitize the Content-ID field in MIME header.
 *
 *  This function is necessary per RFC-2392: A "cid" URL is converted to the corresponding Content-ID message header
 *  MIME by removing the "cid:" prefix, and enclosing the remaining parts with an angle bracket pair, "<" and ">".
 *  For example, "cid:foo4%25foo1@bar.net" corresponds to Content-ID: <foo4%25foo1@bar.net>
 *
 * @param mimeContentId The raw content ID value in MIME header.
 * @return The sanitized content ID.
 */
std::string sanitizeContentId(const std::string& mimeContentId) {
    std::string sanitizedContentId;
    if (mimeContentId.empty()) {
        avsUtils::Logger::log("The mimeContentId is empty, can't be sanitized");
    } else if ( ('<' == mimeContentId.front()) && ('>' == mimeContentId.back()) ) {
        // Getting attachment ID within angle bracket <>.
        sanitizedContentId = mimeContentId.substr(1, mimeContentId.size() - 2);
    } else {
        sanitizedContentId = mimeContentId;
    }
    return sanitizedContentId;
}

MimeParser::MimeParser(MessageConsumerInterface *messageConsumer,
        std::shared_ptr<avsCommon::AttachmentManagerInterface> attachmentManager)
        : m_receivedFirstChunk{false},
          m_currDataType{ContentType::NONE},
          m_messageConsumer{messageConsumer},
          m_attachmentManager{attachmentManager} {
      m_multipartReader.onPartBegin = MimeParser::partBeginCallback;
      m_multipartReader.onPartData = MimeParser::partDataCallback;
      m_multipartReader.onPartEnd = MimeParser::partEndCallback;
      m_multipartReader.userData = this;
}

void MimeParser::partBeginCallback(const MultipartHeaders &headers, void *userData) {
    MimeParser *parser = static_cast<MimeParser*>(userData);
    std::string contentType = headers[MIME_CONTENT_TYPE_FIELD_NAME];
    if (contentType.find(MIME_JSON_CONTENT_TYPE) != std::string::npos) {
        parser->m_currDataType = MimeParser::ContentType::JSON;
    } else if (contentType.find(MIME_OCTET_STREAM_CONTENT_TYPE) != std::string::npos) {
        if (headers.count(MIME_CONTENT_ID_FIELD_NAME) == 1) {
            parser->m_message = sanitizeContentId(headers[MIME_CONTENT_ID_FIELD_NAME]);
        }
        parser->m_currDataType = MimeParser::ContentType::ATTACHMENT;
        parser->m_attachment = std::make_shared<std::stringstream>();
    }
}

void MimeParser::partDataCallback(const char *buffer, size_t size, void *userData) {
    MimeParser *parser = static_cast<MimeParser*>(userData);
    switch(parser->m_currDataType) {
        case MimeParser::ContentType::JSON:
            parser->m_message.append(std::string(buffer, size));
            break;
        case MimeParser::ContentType::ATTACHMENT:
            parser->m_attachment->write(buffer, size);
            break;
        default:
            Logger::log("Data received for usupported part type");
    }
}

void MimeParser::partEndCallback(void *userData) {
    MimeParser *parser = static_cast<MimeParser*>(userData);
    switch (parser->m_currDataType) {
        case MimeParser::ContentType::JSON:
        {
            auto message = std::make_shared<avsCommon::avs::Message>(parser->m_message,
                    parser->m_attachmentManager);
            if(!parser->m_messageConsumer) {
                Logger::log("Message Consumer has not been set. Message from ACL cannot be processed.");
                break;
            }
            parser->m_messageConsumer->consumeMessage(message);
            break;
        }
        case MimeParser::ContentType::ATTACHMENT:
        {
            parser->m_attachmentManager->createAttachment(parser->m_message, parser->m_attachment);
            break;
        }
        default:
            Logger::log("Ended part for unsupported part type");
    }

    parser->m_message = "";
}

void MimeParser::reset() {
    m_message = "";
    m_currDataType = ContentType::NONE;
    m_receivedFirstChunk = false;
    m_multipartReader.reset();
    m_attachment.reset();
}

void MimeParser::setBoundaryString(const std::string& boundaryString) {
    m_multipartReader.setBoundary(boundaryString);
}

void MimeParser::feed(char *data, size_t length) {
    /// Size of CLRF in chars
    static int LEADING_CRLF_CHAR_SIZE = 2;
    /// ASCII value of CR
    static char CARRIAGE_RETURN_ASCII = 13;
    /// ASCII value of LF
    static char LINE_FEED_ASCII = 10;
    /**
     * Our parser expects no leading CRLF in the data stream. Additionally downchannel streams
     * include this CRLF but event streams do not. So just remove the CRLF in the first chunk of the stream
     * if it exists.
     */
    if (!m_receivedFirstChunk) {
        if (CARRIAGE_RETURN_ASCII == data[0] && LINE_FEED_ASCII == data[1]) {
            data += LEADING_CRLF_CHAR_SIZE;
            length -= LEADING_CRLF_CHAR_SIZE;
            m_receivedFirstChunk = true;
        }
    }
    m_multipartReader.feed(data, length);
}

MessageConsumerInterface* MimeParser::getMessageConsumer() {
    return m_messageConsumer;
}

}// acl
}// alexaClientSDK
