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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_EXCHANGEHANDLER_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_EXCHANGEHANDLER_H_

#include <memory>

#include "ACL/Transport/ExchangeHandlerContextInterface.h"

namespace alexaClientSDK {
namespace acl {

class HTTP2Transport;

/**
 * Common base class for HTTP2 request / response exchanges with AVS.
 */
class ExchangeHandler {
public:
    /**
     * Constructor.
     *
     * @param context The context in which this HTTP2 request / reply exchange will be performed.
     * @param authToken The authorization token to send in the request.
     */
    ExchangeHandler(std::shared_ptr<ExchangeHandlerContextInterface> context, const std::string& authToken);

    /**
     * Destructor
     */
    virtual ~ExchangeHandler() = default;

protected:
    /// The @c HTTP2Transport instance for which this exchange is to be performed.
    std::shared_ptr<ExchangeHandlerContextInterface> m_context;

    /// The auth token used to make the request.
    const std::string m_authToken;

    /// The AVS authorization header to send in the request.
    const std::string m_authHeader;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_EXCHANGEHANDLER_H_
