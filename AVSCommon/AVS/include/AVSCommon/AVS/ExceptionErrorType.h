/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

/**
 * @file
 */
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EXCEPTIONERRORTYPE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EXCEPTIONERRORTYPE_H_

#include <iostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

enum class ExceptionErrorType {
    /// The directive sent to your client was malformed or the payload does not conform to the directive specification.
    UNEXPECTED_INFORMATION_RECEIVED,
    /// The operation specified by the namespace/name in the directive's header are not supported by the client.
    UNSUPPORTED_OPERATION,
    /**
     * An error occurred while the device was handling the directive and the error does not fall into the
     * specified categories.
     */
    INTERNAL_ERROR
};

inline std::ostream& operator<<(std::ostream& stream, ExceptionErrorType type) {
    switch (type) {
        case ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED:
            stream << "UNEXPECTED_INFORMATION_RECEIVED";
            break;
        case ExceptionErrorType::UNSUPPORTED_OPERATION:
            stream << "UNSUPPORTED_OPERATION";
            break;
        case ExceptionErrorType::INTERNAL_ERROR:
            stream << "INTERNAL_ERROR";
            break;
    }
    return stream;
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EXCEPTIONERRORTYPE_H_
