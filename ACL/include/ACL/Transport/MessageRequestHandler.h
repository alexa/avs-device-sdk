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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEREQUESTHANDLER_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEREQUESTHANDLER_H_

#include <memory>

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/Utils/HTTP2/HTTP2MimeRequestSourceInterface.h>

#include "ACL/Transport/ExchangeHandler.h"
#include "ACL/Transport/MessageConsumerInterface.h"
#include "ACL/Transport/MimeResponseStatusHandlerInterface.h"

namespace alexaClientSDK {
namespace acl {

/**
 * Handle an HTTP2 request and response for a specific @c MessageRequest.
 */
class MessageRequestHandler
        : public ExchangeHandler
        , public avsCommon::utils::http2::HTTP2MimeRequestSourceInterface
        , public MimeResponseStatusHandlerInterface
        , public std::enable_shared_from_this<MessageRequestHandler> {
public:
    /**
     * Destructor.
     */
    ~MessageRequestHandler() override;

    /**
     * Create a MessageRequestHandler and send the message request.
     *
     * @param context The ExchangeContext in which this MessageRequest handler will operate.
     * @param authToken The token to use to authorize the request.
     * @param messageRequest The MessageRequest to send.
     * @param messageConsumer Where to send messages.
     * @param attachmentManager Where to get attachments to write to.
     * @return A new MessageRequestHandler or nullptr if the operation fails.
     */
    static std::shared_ptr<MessageRequestHandler> create(
        std::shared_ptr<ExchangeHandlerContextInterface> context,
        const std::string& authToken,
        std::shared_ptr<avsCommon::avs::MessageRequest> messageRequest,
        std::shared_ptr<MessageConsumerInterface> messageConsumer,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager);

private:
    /**
     * Constructor.
     *
     * @param context The ExchangeContext in which this MessageRequest handler will operate.
     * @param authToken The token to use to authorize the request.
     * @param messageRequest The MessageRequest to send.
     */
    MessageRequestHandler(
        std::shared_ptr<ExchangeHandlerContextInterface> context,
        const std::string& authToken,
        std::shared_ptr<avsCommon::avs::MessageRequest> messageRequest);

    /**
     * Notify the associated HTTP2Transport instance that the message request failed or was acknowledged by AVS.
     */
    void reportMessageRequestAcknowledged();

    /**
     * Notify the associated HTTP2Transport instance that the message request exchange has finished.
     */
    void reportMessageRequestFinished();

    /// @name HTTP2MimeRequestSourceInterface methods
    /// @{
    std::vector<std::string> getRequestHeaderLines() override;
    avsCommon::utils::http2::HTTP2GetMimeHeadersResult getMimePartHeaderLines() override;
    avsCommon::utils::http2::HTTP2SendDataResult onSendMimePartData(char* bytes, size_t size) override;
    /// @}

    /// @name MimeResponseStatusHandlerInterface
    /// @{
    void onActivity() override;
    bool onReceiveResponseCode(long responseCode) override;
    void onResponseFinished(avsCommon::utils::http2::HTTP2ResponseFinishedStatus status, const std::string& nonMimeBody)
        override;
    /// @}

    /// The MessageRequest that this handler is servicing.
    std::shared_ptr<avsCommon::avs::MessageRequest> m_messageRequest;

    /// JSON payload of message to send.
    std::string m_json;

    /// Next char in m_json to send.
    const char* m_jsonNext;

    /// Number of bytes left unsent in m_json.
    size_t m_countOfJsonBytesLeft;

    /// The number of parts that have been sent.
    size_t m_countOfPartsSent;

    /// Reader for current attachment (if any).
    std::shared_ptr<avsCommon::avs::MessageRequest::NamedReader> m_namedReader;

    /// Whether acknowledge of the @c MessageRequest was reported.
    bool m_wasMessageRequestAcknowledgeReported;

    /// Whether finish of the @c MessageRequest was reported.
    bool m_wasMessageRequestFinishedReported;

    /// Response code received through @c onReciveResponseCode (or zero).
    long m_responseCode;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEREQUESTHANDLER_H_
