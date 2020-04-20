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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATATYPE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATATYPE_H_

#include <iostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/**
 * Enum which captures types of data supported as metrics
 */
enum class DataType {
    /// Used to denote a duration metric
    DURATION,
    /// Used to denote a counter metric
    COUNTER,
    /// Used to denote a string metric
    STRING
};

// Inline function overloading the << operator to feed DataType into ostream
inline std::ostream& operator<<(std::ostream& stream, const DataType& dataType) {
    switch (dataType) {
        case DataType::DURATION:
            stream << "DURATION";
            break;
        case DataType::COUNTER:
            stream << "COUNTER";
            break;
        case DataType::STRING:
            stream << "STRING";
            break;
    }
    return stream;
}

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATATYPE_H_