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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MIMERESPONSESINK_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MIMERESPONSESINK_H_

#include <memory>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/AVS/Attachment/AttachmentWriter.h>
#include <AVSCommon/Utils/HTTP2/HTTP2MimeResponseSinkInterface.h>

#include "ACL/Transport/MessageConsumerInterface.h"
#include "ACL/Transport/MimeResponseStatusHandlerInterface.h"

namespace alexaClientSDK {
namespace acl {

/**
 * Handle Mime encoded responses from AVS.
 *
 * This includes forwarding json payloads to an @c MessageConsumer, attachments to attachment writers, and
 * capturing exceptions for non 2xx results.
 */
class MimeResponseSink : public avsCommon::utils::http2::HTTP2MimeResponseSinkInterface {
public:
    /**
     * Constructor.
     *
     * @param handler The object to forward status and result notifications to.
     * @param messageConsumer Object to send decoded messages to.
     * @param attachmentManager Object with which to get attachments to write to.
     * @param attachmentContextId Id added to content IDs to assure global uniqueness.
     */
    MimeResponseSink(
        std::shared_ptr<MimeResponseStatusHandlerInterface> handler,
        std::shared_ptr<MessageConsumerInterface> messageConsumer,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager,
        std::string attachmentContextId);

    /**
     * Destructor.
     */
    virtual ~MimeResponseSink() = default;

    /// @name HTTP2MimeResponseSinkInterface methods
    /// @{
    bool onReceiveResponseCode(long responseCode) override;
    bool onReceiveHeaderLine(const std::string& line) override;
    bool onBeginMimePart(const std::multimap<std::string, std::string>& headers) override;
    avsCommon::utils::http2::HTTP2ReceiveDataStatus onReceiveMimeData(const char* bytes, size_t size) override;
    bool onEndMimePart() override;
    avsCommon::utils::http2::HTTP2ReceiveDataStatus onReceiveNonMimeData(const char* bytes, size_t size) override;
    void onResponseFinished(avsCommon::utils::http2::HTTP2ResponseFinishedStatus status) override;
    /// @}

private:
    /// Types of mime parts
    enum ContentType {
        /// The default value, indicating no data.
        NONE,
        /// The content represents a JSON formatted string.
        JSON,
        /// The content represents binary data.
        ATTACHMENT
    };

    /**
     * Write received data to the currently accumulating attachment.
     *
     * @param bytes Pointer to the bytes to write.
     * @param size Number of bytes to write.
     * @return Status of the operation.  @see HTTP2ReceiveDataStatus.
     */
    avsCommon::utils::http2::HTTP2ReceiveDataStatus writeToAttachment(const char* bytes, size_t size);

    /// The handler to forward status to.
    std::shared_ptr<MimeResponseStatusHandlerInterface> m_handler;

    /// The object to send decoded messages to.
    std::shared_ptr<MessageConsumerInterface> m_messageConsumer;

    /// The attachment manager.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> m_attachmentManager;

    /// Type of content in the current part.
    ContentType m_contentType;

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

    /// Non-mime response body acculumulated for response codes other than HTTPResponseCode::SUCCESS_OK.
    std::string m_nonMimeBody;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MIMERESPONSESINK_H_
