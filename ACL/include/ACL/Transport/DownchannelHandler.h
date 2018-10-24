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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_DOWNCHANNELHANDLER_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_DOWNCHANNELHANDLER_H_

#include <memory>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/Utils/HTTP2/HTTP2RequestSourceInterface.h>

#include "ACL/Transport/ExchangeHandler.h"
#include "ACL/Transport/MessageConsumerInterface.h"
#include "ACL/Transport/MimeResponseStatusHandlerInterface.h"

namespace alexaClientSDK {
namespace acl {

class DownchannelHandler;

/**
 * Handle the HTTP2 request and response that establishes and services the downchannel of an connection to AVS.
 */
class DownchannelHandler
        : public ExchangeHandler
        , public avsCommon::utils::http2::HTTP2RequestSourceInterface
        , public MimeResponseStatusHandlerInterface
        , public std::enable_shared_from_this<DownchannelHandler> {
public:
    /**
     * Create a DownchannelHandler, and send the downchannel request.
     *
     * @param context The ExchangeContext in which this MessageRequest handler will operate.
     * @param authToken The token to use to authorize the request.
     * @param messageConsumer Object to send decoded messages to.
     * @param attachmentManager Object with which to get attachments to write to.
     * @return The new DownchannelHandler or nullptr if the operation failed.
     */
    static std::shared_ptr<DownchannelHandler> create(
        std::shared_ptr<ExchangeHandlerContextInterface> context,
        const std::string& authToken,
        std::shared_ptr<MessageConsumerInterface> messageConsumer,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager);

private:
    /**
     * Constructor.
     *
     * @param context The ExchangeContext in which this MessageRequest handler will operate.
     * @param authToken The token to use to authorize the request.
     */
    DownchannelHandler(std::shared_ptr<ExchangeHandlerContextInterface> context, const std::string& authToken);

    /// @name HTTP2RequestSourceInterface methods
    /// @{
    std::vector<std::string> getRequestHeaderLines() override;
    avsCommon::utils::http2::HTTP2SendDataResult onSendData(char* bytes, size_t size) override;
    /// @}

    /// @name MimeResponseStatusHandlerInterface
    /// @{
    void onActivity() override;
    bool onReceiveResponseCode(long responseCode) override;
    void onResponseFinished(avsCommon::utils::http2::HTTP2ResponseFinishedStatus status, const std::string& nonMimeBody)
        override;
    /// @}
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_DOWNCHANNELHANDLER_H_
