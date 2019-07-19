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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "ACL/Transport/MimeResponseSink.h"

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils::http2;

#ifdef DEBUG
/// Carriage return
static const char CR = 0x0D;
#endif

/// The prefix of request IDs passed back in the header of AVS replies.
static const std::string X_AMZN_REQUESTID_PREFIX = "x-amzn-requestid:";

/// MIME field name for a part's MIME type
static const std::string MIME_CONTENT_TYPE_FIELD_NAME = "Content-Type";

/// MIME field name for a part's reference id
static const std::string MIME_CONTENT_ID_FIELD_NAME = "Content-ID";

/// MIME type for JSON payloads
static const std::string MIME_JSON_CONTENT_TYPE = "application/json";

/// MIME type for binary streams
static const std::string MIME_OCTET_STREAM_CONTENT_TYPE = "application/octet-stream";

/// Maximum size of non-mime body to accumulate.
static const size_t NON_MIME_BODY_MAX_SIZE = 4096;

/// String to identify log entries originating from this file.
static const std::string TAG("MimeResponseSink");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

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
static std::string sanitizeContentId(const std::string& mimeContentId) {
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

MimeResponseSink::MimeResponseSink(
    std::shared_ptr<MimeResponseStatusHandlerInterface> handler,
    std::shared_ptr<MessageConsumerInterface> messageConsumer,
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager,
    std::string attachmentContextId) :
        m_handler{handler},
        m_messageConsumer{messageConsumer},
        m_attachmentManager{attachmentManager},
        m_attachmentContextId{std::move(attachmentContextId)} {
    ACSDK_DEBUG5(LX(__func__).d("handler", handler.get()));
}

bool MimeResponseSink::onReceiveResponseCode(long responseCode) {
    ACSDK_DEBUG5(LX(__func__).d("responseCode", responseCode));

    if (m_handler) {
        m_handler->onActivity();
        return m_handler->onReceiveResponseCode(responseCode);
    }
    return false;
}

bool MimeResponseSink::onReceiveHeaderLine(const std::string& line) {
    ACSDK_DEBUG5(LX(__func__).d("line", line));

    if (m_handler) {
        m_handler->onActivity();
    }

#ifdef DEBUG
    if (0 == line.find(X_AMZN_REQUESTID_PREFIX)) {
        auto end = line.find(CR);
        ACSDK_DEBUG0(LX("receivedRequestId").d("value", line.substr(0, end)));
    }
#endif
    return true;
}

bool MimeResponseSink::onBeginMimePart(const std::multimap<std::string, std::string>& headers) {
    ACSDK_DEBUG5(LX(__func__));

    if (m_handler) {
        m_handler->onActivity();
    }

    auto it = headers.find(MIME_CONTENT_TYPE_FIELD_NAME);
    if (headers.end() == it) {
        ACSDK_WARN(LX("noContent-Type"));
        return true;
    }

    auto contentType = it->second;
    if (contentType.find(MIME_JSON_CONTENT_TYPE) != std::string::npos) {
        m_contentType = ContentType::JSON;
        ACSDK_DEBUG9(LX("JsonContentDetected"));
    } else if (
        m_attachmentManager && contentType.find(MIME_OCTET_STREAM_CONTENT_TYPE) != std::string::npos &&
        1 == headers.count(MIME_CONTENT_ID_FIELD_NAME)) {
        auto iy = headers.find(MIME_CONTENT_ID_FIELD_NAME);
        auto contentId = sanitizeContentId(iy->second);
        auto attachmentId = m_attachmentManager->generateAttachmentId(m_attachmentContextId, contentId);
        if (!m_attachmentWriter && attachmentId != m_attachmentIdBeingReceived) {
            m_attachmentWriter = m_attachmentManager->createWriter(attachmentId);
            if (!m_attachmentWriter) {
                ACSDK_ERROR(
                    LX("onBeginMimePartFailed").d("reason", "createWriterFailed").d("attachmentId", attachmentId));
                return false;
            }
            ACSDK_DEBUG9(LX("attachmentContentDetected").d("contentId", contentId));
        }
        m_contentType = ContentType::ATTACHMENT;
    } else {
        ACSDK_WARN(LX("unhandledContent-Type").d("Content-Type", contentType));
        m_contentType = ContentType::NONE;
    }
    return true;
}

HTTP2ReceiveDataStatus MimeResponseSink::onReceiveMimeData(const char* bytes, size_t size) {
    ACSDK_DEBUG5(LX(__func__).d("size", size));

    if (m_handler) {
        m_handler->onActivity();
    }

    switch (m_contentType) {
        case ContentType::JSON:
            m_directiveBeingReceived.append(bytes, size);
            return HTTP2ReceiveDataStatus::SUCCESS;
        case ContentType::ATTACHMENT:
            return writeToAttachment(bytes, size);
        case ContentType::NONE:
            break;
    }
    return HTTP2ReceiveDataStatus::SUCCESS;
}

bool MimeResponseSink::onEndMimePart() {
    ACSDK_DEBUG5(LX(__func__));

    if (m_handler) {
        m_handler->onActivity();
    }

    switch (m_contentType) {
        case ContentType::JSON:
            if (!m_messageConsumer) {
                ACSDK_ERROR(LX("onEndMimePartFailed").d("reason", "nullMessageConsumer"));
                break;
            }
            // Check there's data to send out, because in a re-drive we may skip a directive that's been seen before.
            if (!m_directiveBeingReceived.empty()) {
                m_messageConsumer->consumeMessage(m_attachmentContextId, m_directiveBeingReceived);
                m_directiveBeingReceived.clear();
            }
            break;
        case ContentType::ATTACHMENT:
            m_attachmentIdBeingReceived.clear();
            m_attachmentWriter.reset();
            m_contentType = ContentType::NONE;
            break;
        default:
            ACSDK_ERROR(LX("partEndCallbackFailed").d("reason", "unsupportedContentType"));
            break;
    }
    return true;
}

HTTP2ReceiveDataStatus MimeResponseSink::onReceiveNonMimeData(const char* bytes, size_t size) {
    ACSDK_DEBUG5(LX(__func__).d("size", size));

    if (m_handler) {
        m_handler->onActivity();
    }

    auto total = m_nonMimeBody.size() + size;
    if (total <= NON_MIME_BODY_MAX_SIZE) {
        m_nonMimeBody.append(bytes, size);
    } else {
        // Only append up to the maximum allowed.
        auto spaceLeft = NON_MIME_BODY_MAX_SIZE - m_nonMimeBody.size();
        m_nonMimeBody.append(bytes, spaceLeft);
        ACSDK_ERROR(LX("nonMimeBodyTruncated").d("total", total).d("maxSize", NON_MIME_BODY_MAX_SIZE));
    }

    return HTTP2ReceiveDataStatus(size);
}

void MimeResponseSink::onResponseFinished(HTTP2ResponseFinishedStatus status) {
    ACSDK_DEBUG5(LX(__func__).d("status", status));

    if (m_handler) {
        m_handler->onResponseFinished(status, m_nonMimeBody);
    }
}

HTTP2ReceiveDataStatus MimeResponseSink::writeToAttachment(const char* bytes, size_t size) {
    // Error case.  We can't process the attachment.
    if (!m_attachmentWriter) {
        ACSDK_ERROR(LX("writeToAttachmentFailed").d("reason", "nullAttachmentWriter"));
        return HTTP2ReceiveDataStatus::ABORT;
    }

    auto writeStatus = AttachmentWriter::WriteStatus::OK;
    auto numWritten = m_attachmentWriter->write(const_cast<char*>(bytes), size, &writeStatus);

    switch (writeStatus) {
        case AttachmentWriter::WriteStatus::OK:
            if (numWritten != size) {
                ACSDK_ERROR(LX("writeDataToAttachmentFailed").d("reason", "writeTruncated"));
                return HTTP2ReceiveDataStatus::ABORT;
            }
            return HTTP2ReceiveDataStatus::SUCCESS;

        case AttachmentWriter::WriteStatus::OK_BUFFER_FULL:
            // We're blocked on a slow reader.
            ACSDK_DEBUG9(LX("writeToAttachmentReturningPAUSE"));
            return HTTP2ReceiveDataStatus::PAUSE;

        case AttachmentWriter::WriteStatus::CLOSED:
            // The underlying memory was closed elsewhere.
            ACSDK_WARN(LX("writeDataToAttachmentFailed").d("reason", "attachmentWriterIsClosed"));
            return HTTP2ReceiveDataStatus::ABORT;

        case AttachmentWriter::WriteStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
        case AttachmentWriter::WriteStatus::ERROR_INTERNAL:
            // A low-level error with the Attachment occurred.
            ACSDK_ERROR(LX("writeDataToAttachmentFailed").d("reason", "attachmentWriterInternalError"));
            return HTTP2ReceiveDataStatus::ABORT;

        case AttachmentWriter::WriteStatus::TIMEDOUT:
            // Unexpected status (this attachment writer should be non blocking).
            ACSDK_ERROR(LX("writeDataToAttachmentFailed").d("reason", "unexpectedTimedoutStatus"));
            return HTTP2ReceiveDataStatus::ABORT;
    }
    // Unreachable unless a garbage status was returned.
    ACSDK_ERROR(
        LX("writeDataToAttachmentFailed").d("reason", "unhandledStatus").d("status", static_cast<int>(writeStatus)));
    return HTTP2ReceiveDataStatus::ABORT;
}

}  // namespace acl
}  // namespace alexaClientSDK
