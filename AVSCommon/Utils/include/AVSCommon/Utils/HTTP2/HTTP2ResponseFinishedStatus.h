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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2RESPONSEFINISHEDSTATUS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2RESPONSEFINISHEDSTATUS_H_

#include <ostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

/**
 * Status provided when an HTTP2 response is finished.
 */
enum class HTTP2ResponseFinishedStatus {
    /// Receipt of the response was completed.
    COMPLETE,
    /// Receipt of the response was not completed due to a timeout.
    TIMEOUT,
    /// Receipt of the response was not completed because the operation was cancelled.
    CANCELLED,
    /// Receipt of the response was not completed due to an internal error.
    INTERNAL_ERROR
};

/**
 * Write a @c HTTP2ResponseFinishedStatus value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param status The status to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, HTTP2ResponseFinishedStatus status) {
    switch (status) {
        case HTTP2ResponseFinishedStatus::COMPLETE:
            return stream << "COMPLETE";
        case HTTP2ResponseFinishedStatus::TIMEOUT:
            return stream << "TIMEOUT";
        case HTTP2ResponseFinishedStatus::CANCELLED:
            return stream << "CANCELLED";
        case HTTP2ResponseFinishedStatus::INTERNAL_ERROR:
            return stream << "INTERNAL_ERROR";
    }
    return stream << "Invalid(" << static_cast<int>(status) << ")";
}

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2RESPONSEFINISHEDSTATUS_H_
