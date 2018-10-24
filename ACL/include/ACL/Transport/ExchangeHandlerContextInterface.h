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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_EXCHANGEHANDLERCONTEXTINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_EXCHANGEHANDLERCONTEXTINTERFACE_H_

#include <memory>
#include <string>

#include <AVSCommon/Utils/HTTP2/HTTP2RequestConfig.h>
#include <AVSCommon/Utils/HTTP2/HTTP2RequestInterface.h>

namespace alexaClientSDK {
namespace acl {

/**
 * Interface providing context that an ExchangeHandler operates within.
 */
class ExchangeHandlerContextInterface {
public:
    /**
     * Destructor
     */
    virtual ~ExchangeHandlerContextInterface() = default;

    /**
     * Notification that the downchannel has been established.
     */
    virtual void onDownchannelConnected() = 0;

    /**
     * Notification that the downchannel has failed to be established or has disconnected.
     */
    virtual void onDownchannelFinished() = 0;

    /**
     * Notification that an @c MessgeRequest has been sent.
     */
    virtual void onMessageRequestSent() = 0;

    /**
     * Notification that sending a message request timed out.
     */
    virtual void onMessageRequestTimeout() = 0;

    /**
     * Notification that sending a @c MessageRequest has failed or been acknowledged by AVS
     * (this is used to indicate it is okay to send the next message).
     */
    virtual void onMessageRequestAcknowledged() = 0;

    /**
     * Notification tht a message request has finished it's exchange with AVS.
     */
    virtual void onMessageRequestFinished() = 0;

    /**
     * Notification that sending a ping to AVS has failed or been acknowledged by AVS.
     *
     * @param success Whether the ping was acknowledged successfully.
     */
    virtual void onPingRequestAcknowledged(bool success) = 0;

    /**
     * Notification that a ping request timed out.
     */
    virtual void onPingTimeout() = 0;

    /**
     * Notification of network activity between this client and AVS.
     * (this is used to detect sustained inactivity requiring the send of a ping).
     */
    virtual void onActivity() = 0;

    /**
     * Notification that a request received a FORBIDDEN (403) response.
     *
     * @param authToken The auth token used for the forbidden request or an empty string if the token is not specified.
     */
    virtual void onForbidden(const std::string& authToken = "") = 0;

    /**
     * Create an HTTP2Request for this HTTP2Transport.
     *
     * @param cfg The configuration object which defines the request.
     * @return The new instance of HTTP2RequestInterface, or nullptr if the operation failed.
     */
    virtual std::shared_ptr<avsCommon::utils::http2::HTTP2RequestInterface> createAndSendRequest(
        const avsCommon::utils::http2::HTTP2RequestConfig& cfg) = 0;

    /**
     * Get AVS endpoint to send request to.
     */
    virtual std::string getEndpoint() = 0;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_EXCHANGEHANDLERCONTEXTINTERFACE_H_
