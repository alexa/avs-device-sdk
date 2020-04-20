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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_PRIORITY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_PRIORITY_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/**
 * Enum to capture metric priority levels.
 */
enum class Priority {
    /// Metrics marked as NORMAL are used to communicate normal metrics that do not require urgent attention.
    NORMAL,
    /// Metrics marked as HIGH are used to classify important metrics that might require urgent attention.
    HIGH
};

// Inline function overloading the << operator to feed Priority into ostream
inline std::ostream& operator<<(std::ostream& stream, const Priority& priority) {
    switch (priority) {
        case Priority::NORMAL:
            stream << "NORMAL";
            break;
        case Priority::HIGH:
            stream << "HIGH";
            break;
    }
    return stream;
}

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_PRIORITY_H_