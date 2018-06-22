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

#include <AVSCommon/Utils/Logger/Logger.h>
#include "ACL/Transport/MimeParser.h"
#include <sstream>

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::utils;
using namespace avsCommon::avs::attachment;

/// String to identify log entries originating from this file.
static const std::string TAG("MimeParser");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// MIME field name for a part's MIME type
static const std::string MIME_CONTENT_TYPE_FIELD_NAME = "Content-Type";
/// MIME field name for a part's reference id
static const std::string MIME_CONTENT_ID_FIELD_NAME = "Content-ID";
/// MIME type for JSON payloads
static const std::string MIME_JSON_CONTENT_TYPE = "application/json";
/// MIME type for binary streams
static const std::string MIME_OCTET_STREAM_CONTENT_TYPE = "application/octet-stream";
/// Size of CLRF in chars
static const int LEADING_CRLF_CHAR_SIZE = 2;
/// ASCII value of CR
static const char CARRIAGE_RETURN_ASCII = 13;
/// ASCII value of LF
static const char LINE_FEED_ASCII = 10;

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
        ACSDK_ERROR(LX("sanitizeContentIdFailed").d("reason", "emptyMimeContentId"));
    } else if (('<' == mimeContentId.front()) && ('>' == mimeContentId.back())) {
        // Getting attachment ID within angle bracket <>.
        sanitizedContentId = mimeContentId.substr(1, mimeContentId.size() - 2);
    } else {
        sanitizedContentId = mimeContentId;
    }
    return sanitizedContentId;
}

MimeParser::MimeParser(
    std::shared_ptr<MessageConsumerInterface> messageConsumer,
    std::shared_ptr<AttachmentManager> attachmentManager) :
        m_receivedFirstChunk{false},
        m_currDataType{ContentType::NONE},
        m_messageConsumer{messageConsumer},
        m_attachmentManager{attachmentManager},
        m_dataParsedStatus{DataParsedStatus::OK},
        m_currentByteProgress{0},
        m_totalSuccessfullyProcessedBytes{0},
        m_isAttachmentWriterBufferFull{false} {
    m_multipartReader.onPartBegin = MimeParser::partBeginCallback;
    m_multipartReader.onPartData = MimeParser::partDataCallback;
    m_multipartReader.onPartEnd = MimeParser::partEndCallback;
    m_multipartReader.userData = this;
}

void MimeParser::partBeginCallback(const MultipartHeaders& headers, void* userData) {
    MimeParser* parser = static_cast<MimeParser*>(userData);

    if (parser->m_dataParsedStatus != MimeParser::DataParsedStatus::OK) {
        ACSDK_ERROR(
            LX("partBeginCallbackFailed").d("reason", "mimeParsingFailed").d("status", parser->m_dataParsedStatus));
        return;
    }

    std::string contentType = headers[MIME_CONTENT_TYPE_FIELD_NAME];
    if (contentType.find(MIME_JSON_CONTENT_TYPE) != std::string::npos) {
        parser->m_currDataType = MimeParser::ContentType::JSON;
    } else if (contentType.find(MIME_OCTET_STREAM_CONTENT_TYPE) != std::string::npos) {
        if (1 == headers.count(MIME_CONTENT_ID_FIELD_NAME)) {
            auto contentId = sanitizeContentId(headers[MIME_CONTENT_ID_FIELD_NAME]);
            auto attachmentId =
                parser->m_attachmentManager->generateAttachmentId(parser->m_attachmentContextId, contentId);

            if (!parser->m_attachmentWriter && attachmentId != parser->m_attachmentIdBeingReceived) {
                parser->m_attachmentWriter = parser->m_attachmentManager->createWriter(attachmentId);
                if (!parser->m_attachmentWriter) {
                    ACSDK_ERROR(LX("partBeginCallbackFailed")
                                    .d("reason", "createWriterFailed")
                                    .d("attachmentId", attachmentId));
                }
            }
        }
        parser->m_currDataType = MimeParser::ContentType::ATTACHMENT;
    }
}

MimeParser::DataParsedStatus MimeParser::writeDataToAttachment(const char* buffer, size_t size) {
    // Error case.  We can't process the attachment.
    if (!m_attachmentWriter) {
        ACSDK_ERROR(LX("writeDataToAttachmentFailed").d("reason", "nullAttachmentWriter"));
        return MimeParser::DataParsedStatus::ERROR;
    }

    auto writeStatus = AttachmentWriter::WriteStatus::OK;
    auto numWritten = m_attachmentWriter->write(const_cast<char*>(buffer), size, &writeStatus);

    // The underlying memory was closed elsewhere.
    if (AttachmentWriter::WriteStatus::CLOSED == writeStatus) {
        ACSDK_WARN(LX("writeDataToAttachmentFailed").d("reason", "attachmentWriterIsClosed"));
        return MimeParser::DataParsedStatus::ERROR;
    }

    // A low-level error with the Attachment occurred.
    if (AttachmentWriter::WriteStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE == writeStatus ||
        AttachmentWriter::WriteStatus::ERROR_INTERNAL == writeStatus) {
        ACSDK_ERROR(LX("writeDataToAttachmentFailed").d("reason", "attachmentWriterInternalError"));
        return MimeParser::DataParsedStatus::ERROR;
    }

    // We're blocked on a slow reader.
    if (AttachmentWriter::WriteStatus::OK_BUFFER_FULL == writeStatus) {
        setAttachmentWriterBufferFull(true);
        return MimeParser::DataParsedStatus::INCOMPLETE;
    }

    // A final sanity check to ensure we wrote the data we intended to.
    if (AttachmentWriter::WriteStatus::OK == writeStatus && numWritten != size) {
        ACSDK_ERROR(LX("writeDataToAttachmentFailed").d("reason", "writeTruncated"));
        return MimeParser::DataParsedStatus::ERROR;
    }

    setAttachmentWriterBufferFull(false);
    return MimeParser::DataParsedStatus::OK;
}

void MimeParser::partDataCallback(const char* buffer, size_t size, void* userData) {
    MimeParser* parser = static_cast<MimeParser*>(userData);

    if (MimeParser::DataParsedStatus::INCOMPLETE == parser->m_dataParsedStatus) {
        ACSDK_DEBUG9(LX("partDataCallbackIgnored").d("reason", "attachmentWriterFullBuffer"));
        return;
    }

    if (parser->m_dataParsedStatus != MimeParser::DataParsedStatus::OK) {
        ACSDK_ERROR(
            LX("partDataCallbackFailed").d("reason", "mimeParsingError").d("status", parser->m_dataParsedStatus));
        return;
    }

    // If we've already processed any of this part in a previous incomplete iteration, let's not process it twice.
    if (!parser->shouldProcessBytes(size)) {
        ACSDK_DEBUG9(LX("partDataCallbackSkipped").d("reason", "bytesAlreadyProcessed"));
        parser->updateCurrentByteProgress(size);
        parser->m_dataParsedStatus = MimeParser::DataParsedStatus::OK;
        return;
    }

    // Ok, there is data in this part we've not processed yet.

    // Let's do the math so we only process bytes within this part that have not been processed before.
    auto bytesAlreadyProcessed = parser->m_totalSuccessfullyProcessedBytes - parser->m_currentByteProgress;
    auto bytesToProcess = size - bytesAlreadyProcessed;

    // Sanity check that we actually have correctly bounded work to do.
    if (0 == bytesToProcess || bytesToProcess > size) {
        ACSDK_ERROR(LX("partDataCallbackFailed")
                        .d("reason", "invalidBytesToProcess")
                        .d("bytesToProcess", bytesToProcess)
                        .d("totalSize", size));
        parser->m_dataParsedStatus = MimeParser::DataParsedStatus::ERROR;
        return;
    }

    // Find the correct offset in the data to process.
    const char* dataProcessingPoint = &(buffer[bytesAlreadyProcessed]);

    switch (parser->m_currDataType) {
        case MimeParser::ContentType::JSON:
            parser->m_directiveBeingReceived.append(dataProcessingPoint, bytesToProcess);
            parser->updateCurrentByteProgress(bytesToProcess);
            break;
        case MimeParser::ContentType::ATTACHMENT:
            parser->m_dataParsedStatus = parser->writeDataToAttachment(dataProcessingPoint, bytesToProcess);
            if (MimeParser::DataParsedStatus::OK == parser->m_dataParsedStatus) {
                parser->updateCurrentByteProgress(bytesToProcess);
            }
            break;
        default:
            ACSDK_ERROR(LX("partDataCallbackFailed").d("reason", "unsupportedContentType"));
            parser->m_dataParsedStatus = MimeParser::DataParsedStatus::ERROR;
    }
}

void MimeParser::partEndCallback(void* userData) {
    MimeParser* parser = static_cast<MimeParser*>(userData);

    if (parser->m_dataParsedStatus != MimeParser::DataParsedStatus::OK) {
        ACSDK_ERROR(
            LX("partEndCallbackFailed").d("reason", "mimeParsingError").d("status", parser->m_dataParsedStatus));
        return;
    }

    switch (parser->m_currDataType) {
        case MimeParser::ContentType::JSON:
            if (!parser->m_messageConsumer) {
                ACSDK_ERROR(LX("partEndCallbackFailed")
                                .d("reason", "nullMessageConsumer")
                                .d("status", parser->m_dataParsedStatus));
                break;
            }
            // Check there's data to send out, because in a re-drive we may skip a directive that's been seen before.
            if (parser->m_directiveBeingReceived != "") {
                parser->m_messageConsumer->consumeMessage(
                    parser->m_attachmentContextId, parser->m_directiveBeingReceived);
                parser->m_directiveBeingReceived = "";
            }
            break;

        case MimeParser::ContentType::ATTACHMENT:
            parser->closeActiveAttachmentWriter();
            break;

        default:
            ACSDK_ERROR(LX("partEndCallbackFailed").d("reason", "unsupportedContentType"));
    }
}

void MimeParser::reset() {
    m_currDataType = ContentType::NONE;
    m_receivedFirstChunk = false;
    m_multipartReader.reset();
    m_dataParsedStatus = DataParsedStatus::OK;
    closeActiveAttachmentWriter();
    m_isAttachmentWriterBufferFull = false;
}

void MimeParser::setAttachmentContextId(const std::string& attachmentContextId) {
    m_attachmentContextId = attachmentContextId;
}

void MimeParser::setBoundaryString(const std::string& boundaryString) {
    m_multipartReader.setBoundary(boundaryString);
}

/*
 * This function is designed to allow the processing of MIME multipart data in chunks.  If a chunk of data cannot
 * be fully processed, this class allows that chunk to be re-driven until it returns @c DataParsedStatus::OK.
 *
 * Each invocation of of this function may result any number of directives and attachments being parsed out,
 * and then routed out to observers.
 *
 * As a brief example of how a parse might fail, and what the internal logic needs to do, let's imagine a multipart
 * data chunk arranged as follows (let's say it's chunk x > 0 of the data):
 *
 * [ part of directive 2 | attachment 2 | part of directive 3 ]
 *                             ^
 *                             |
 *
 * If the chunk fails while processing attachment 2 (per the arrow above), then the logic here needs to be careful to
 * ensure that a re-drive is possible, without confusing the underlying MultiPartReader object, which is stateful.
 *
 * The solution is to capture the state of the MultiPartReader object at the start of the function, and if the parse
 * is not successful, restore the object to its initial state, allowing a re-drive.  Otherwise it is left in its
 * resulting state for subsequent data chunks.
 */
MimeParser::DataParsedStatus MimeParser::feed(char* data, size_t length) {
    // Capture old state in case the complete parse does not succeed (see function comments).
    auto oldReader = m_multipartReader;
    auto oldReceivedFirstChunk = m_receivedFirstChunk;
    auto oldDataType = m_currDataType;

    /**
     * Our parser expects no leading CRLF in the data stream. Additionally downchannel streams
     * include this CRLF but event streams do not. So just remove the CRLF in the first chunk of the stream
     * if it exists.
     */
    if (!m_receivedFirstChunk) {
        if (length >= LEADING_CRLF_CHAR_SIZE && CARRIAGE_RETURN_ASCII == data[0] && LINE_FEED_ASCII == data[1]) {
            data += LEADING_CRLF_CHAR_SIZE;
            length -= LEADING_CRLF_CHAR_SIZE;
            m_receivedFirstChunk = true;
        }
    }

    // Initialize this before all the feed() callbacks happen (since this persists from previous call).
    m_currentByteProgress = 0;
    m_dataParsedStatus = DataParsedStatus::OK;

    m_multipartReader.feed(data, length);

    if (DataParsedStatus::OK == m_dataParsedStatus) {
        // We parsed all the data ok - reset our counters for the next potential feed of data.
        resetByteProgressCounters();
    } else {
        // There was a problem parsing the data - we need to reset the previous mime parser state for re-drive.
        m_multipartReader = oldReader;
        m_receivedFirstChunk = oldReceivedFirstChunk;
        m_currDataType = oldDataType;
    }

    return m_dataParsedStatus;
}

std::shared_ptr<MessageConsumerInterface> MimeParser::getMessageConsumer() {
    return m_messageConsumer;
}

void MimeParser::closeActiveAttachmentWriter() {
    m_attachmentIdBeingReceived = "";
    m_attachmentWriter.reset();
}

bool MimeParser::shouldProcessBytes(size_t size) const {
    return (m_currentByteProgress + size) > m_totalSuccessfullyProcessedBytes;
}

void MimeParser::updateCurrentByteProgress(size_t size) {
    m_currentByteProgress += size;
    if (m_currentByteProgress > m_totalSuccessfullyProcessedBytes) {
        m_totalSuccessfullyProcessedBytes = m_currentByteProgress;
    }
}

void MimeParser::resetByteProgressCounters() {
    m_totalSuccessfullyProcessedBytes = 0;
    m_currentByteProgress = 0;
}

void MimeParser::setAttachmentWriterBufferFull(bool isFull) {
    if (isFull == m_isAttachmentWriterBufferFull) {
        return;
    }
    ACSDK_DEBUG9(LX("setAttachmentWriterBufferFull").d("full", isFull));
    m_isAttachmentWriterBufferFull = isFull;
}

}  // namespace acl
}  // namespace alexaClientSDK
