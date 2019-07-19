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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_REQUESTER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_REQUESTER_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/// An enum class indicating whether an operation originated from a Device or Cloud (AVS).
enum class Requester {
    /// The request came from AVS as a result of a directive.
    CLOUD,
    /// The request came from the device. Can be from either the AVS device or a connected device.
    DEVICE
};

/**
 * Converts an @c Requester enum to a string.
 *
 * @param requester The @c Requester enum.
 * @return The string representation of the enum.
 */
inline std::string requesterToString(Requester requester) {
    switch (requester) {
        case Requester::CLOUD:
            return "CLOUD";
        case Requester::DEVICE:
            return "DEVICE";
    }
    return "UNKNOWN";
}

/**
 * Write a @c Requester value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param requester The @c Requester value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, Requester requester) {
    return stream << requesterToString(requester);
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_REQUESTER_H_
