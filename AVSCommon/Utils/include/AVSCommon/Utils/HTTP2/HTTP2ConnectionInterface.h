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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2CONNECTIONINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2CONNECTIONINTERFACE_H_

#include <memory>

#include "AVSCommon/Utils/HTTP2/HTTP2RequestConfig.h"
#include "AVSCommon/Utils/HTTP2/HTTP2RequestInterface.h"
#include "AVSCommon/Utils/HTTP2/HTTP2RequestType.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

/**
 * Interface for managing an HTTP2 connection.
 */
class HTTP2ConnectionInterface {
public:
    /**
     * Destructor.
     */
    virtual ~HTTP2ConnectionInterface() = default;

    /**
     * Create an HTTP2 request.  Send it immediately.
     *
     * @param config The configuration object which defines the request.
     * @return A new HTTP2GetRequest instance.
     */
    virtual std::shared_ptr<HTTP2RequestInterface> createAndSendRequest(const HTTP2RequestConfig& config) = 0;

    /**
     * Terminate the HTTP2 connection, forcing immediate termination of any active requests.
     */
    virtual void disconnect() = 0;
};

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2CONNECTIONINTERFACE_H_
