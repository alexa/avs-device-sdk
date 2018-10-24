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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_PINGHANDLER_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_PINGHANDLER_H_

#include <memory>

#include <AVSCommon/Utils/HTTP2/HTTP2RequestSourceInterface.h>
#include <AVSCommon/Utils/HTTP2/HTTP2ResponseSinkInterface.h>

#include "ACL/Transport/ExchangeHandler.h"

/**
 * Handle HTTP2 request / response for a ping sent to AVS.
 */
namespace alexaClientSDK {
namespace acl {

class PingHandler
        : public acl::ExchangeHandler
        , public avsCommon::utils::http2::HTTP2RequestSourceInterface
        , public avsCommon::utils::http2::HTTP2ResponseSinkInterface
        , public std::enable_shared_from_this<PingHandler> {
public:
    /**
     * Create a PingHandler and send the ping request.
     *
     * @param context The ExchangeContext in which this ping handler will operate.
     * @param authToken The token to use to authorize the request.
     * @return A new PingHandler or nullptr if the operation fails.
     */
    static std::shared_ptr<PingHandler> create(
        std::shared_ptr<ExchangeHandlerContextInterface> context,
        const std::string& authToken);

private:
    /**
     * Constructor.
     *
     * @param context The ExchangeContext in which this ping handler will operate.
     * @param authToken The token to use to authorize the request.
     */
    PingHandler(std::shared_ptr<ExchangeHandlerContextInterface> context, const std::string& authToken);

    /**
     * Report that the ping was acknowledged.
     */
    void reportPingAcknowledged();

    /// @name HTTP2RequestSourceInterface methods
    /// @{
    std::vector<std::string> getRequestHeaderLines() override;
    avsCommon::utils::http2::HTTP2SendDataResult onSendData(char* bytes, size_t size) override;
    /// @}

    /// @name HTTP2ResponseSinkInterface methods
    /// @{
    bool onReceiveResponseCode(long responseCode) override;
    bool onReceiveHeaderLine(const std::string& line) override;
    avsCommon::utils::http2::HTTP2ReceiveDataStatus onReceiveData(const char* bytes, size_t size) override;
    void onResponseFinished(avsCommon::utils::http2::HTTP2ResponseFinishedStatus status) override;
    /// @}

    /// Whether acknowledge of this ping request was reported.
    bool m_wasPingAcknowledgedReported;

    /// Response code received through @c onReciveResponseCode (or zero).
    long m_responseCode;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_PINGHANDLER_H_
