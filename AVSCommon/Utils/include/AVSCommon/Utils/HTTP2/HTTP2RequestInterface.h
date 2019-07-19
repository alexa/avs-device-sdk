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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2REQUESTINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2REQUESTINTERFACE_H_

#include <string>

#include "AVSCommon/Utils/HTTP2/HTTP2RequestSourceInterface.h"
#include "AVSCommon/Utils/HTTP2/HTTP2ResponseSinkInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

/**
 * Interface for making an HTTP2 request.
 */
class HTTP2RequestInterface {
public:
    /**
     * Destructor.
     */
    virtual ~HTTP2RequestInterface() = default;

    /**
     * Cancel this @c HTTP2Request.
     *
     * @return Whether cancelling this request was triggered.
     */
    virtual bool cancel() = 0;

    /**
     * Get an integer uniquely identifying this request.
     *
     * @return An integer uniquely identifying this request.
     */
    virtual std::string getId() const = 0;
};

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2REQUESTINTERFACE_H_
