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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2REQUESTTYPE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2REQUESTTYPE_H_

#include <ostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

/**
 * Type of request mde over an HTTP2Connection.
 */
enum class HTTP2RequestType {
    /// A HTTP2 GET request.
    GET,
    /// A HTTP2 POST request.
    POST
};

inline std::ostream& operator<<(std::ostream& stream, HTTP2RequestType type) {
    switch (type) {
        case HTTP2RequestType::GET:
            return stream << "GET";
        case HTTP2RequestType::POST:
            return stream << "POST";
    }
    return stream << "UNKNOWN(" << static_cast<int>(type) << ")";
}

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2REQUESTTYPE_H_
