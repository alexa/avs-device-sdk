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

#include <string>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "ACL/Transport/HTTP2Transport.h"
#include "ACL/Transport/ExchangeHandler.h"

namespace alexaClientSDK {
namespace acl {

/// HTTP Header key for the bearer token.
static const std::string AUTHORIZATION_HEADER = "Authorization: Bearer ";

/// String to identify log entries originating from this file.
static const std::string TAG("ExchangeHandler");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

ExchangeHandler::ExchangeHandler(
    std::shared_ptr<ExchangeHandlerContextInterface> context,
    const std::string& authToken) :
        m_context{context},
        m_authToken{authToken},
        m_authHeader{AUTHORIZATION_HEADER + authToken} {
    ACSDK_DEBUG5(LX(__func__).d("context", context.get()).sensitive("authToken", authToken));
    if (m_authToken.empty()) {
        ACSDK_ERROR(LX(__func__).m("emptyAuthToken"));
    }
}

}  // namespace acl
}  // namespace alexaClientSDK
