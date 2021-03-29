/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/AVS/Attachment/AttachmentManagerInterface.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/SDKInterfaces/EventTracerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/Utils/Power/PowerResource.h>
#include <AVSCommon/Utils/HTTP2/HTTP2MimeRequestSourceInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

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
     * @param metricRecorder The metric recorder.
     * @param eventTracer The object to trace events sent to AVS.
     * @param powerResource The optional @c PowerResource to prevent device from going into LPM.
     * @return A new MessageRequestHandler or nullptr if the operation fails.
     */
    static std::shared_ptr<MessageRequestHandler> create(
        std::shared_ptr<ExchangeHandlerContextInterface> context,
        const std::string& authToken,
        std::shared_ptr<avsCommon::avs::MessageRequest> messageRequest,
        std::shared_ptr<MessageConsumerInterface> messageConsumer,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface> attachmentManager,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        std::shared_ptr<avsCommon::sdkInterfaces::EventTracerInterface> eventTracer = nullptr,
        const std::shared_ptr<avsCommon::utils::power::PowerResource>& powerResource = nullptr);

    /// @name HTTP2MimeRequestSourceInterface methods
    /// @{
    avsCommon::utils::http2::HTTP2GetMimeHeadersResult getMimePartHeaderLines() override;
    std::vector<std::string> getRequestHeaderLines() override;
    avsCommon::utils::http2::HTTP2SendDataResult onSendMimePartData(char* bytes, size_t size) override;
    /// @}

    /// @name MimeResponseStatusHandlerInterface
    /// @{
    void onActivity() override;
    bool onReceiveResponseCode(long responseCode) override;
    void onResponseFinished(avsCommon::utils::http2::HTTP2ResponseFinishedStatus status, const std::string& nonMimeBody)
        override;
    /// @}
private:
    /**
     * Constructor.
     *
     * @param context The ExchangeContext in which this MessageRequest handler will operate.
     * @param authToken The token to use to authorize the request.
     * @param messageRequest The MessageRequest to send.
     * @param metricRecorder The metric recorder.
     * @param powerResource The pointer to  @c PowerResource to prevent device from going into LPM.
     */
    MessageRequestHandler(
        std::shared_ptr<ExchangeHandlerContextInterface> context,
        const std::string& authToken,
        std::shared_ptr<avsCommon::avs::MessageRequest> messageRequest,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        const std::shared_ptr<avsCommon::utils::power::PowerResource>& powerResource);

    /**
     * Notify the associated HTTP2Transport instance that the message request failed or was acknowledged by AVS.
     */
    void reportMessageRequestAcknowledged();

    /**
     * Notify the associated HTTP2Transport instance that the message request exchange has finished.
     */
    void reportMessageRequestFinished();

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

    /// The metric recorder.
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /// Whether acknowledge of the @c MessageRequest was reported.
    bool m_wasMessageRequestAcknowledgeReported;

    /// Whether finish of the @c MessageRequest was reported.
    bool m_wasMessageRequestFinishedReported;

    /// Response code received through @c onReceiveResponseCode (or zero).
    long m_responseCode;

    /// The reference to @c PowerResource to prevent device from going into LPM.
    std::shared_ptr<avsCommon::utils::power::PowerResource> m_powerResource;

    /// Status to be reported back to the @c MessageRequest.
    avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status m_resultStatus;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEREQUESTHANDLER_H_
