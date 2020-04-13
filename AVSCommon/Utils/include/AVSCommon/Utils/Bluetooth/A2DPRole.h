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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_A2DPROLE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_A2DPROLE_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace bluetooth {

/// An Enum representing the current A2DP role.
enum class A2DPRole {
    /// Does not support A2DP.
    NONE,
    /// AVS device acting as an A2DPSink.
    SINK,
    /// AVS device acting as an A2DPSource.
    SOURCE
};

/**
 * Converts the @c A2DPRole enum to a string.
 *
 * @param The @c A2DPRole to convert.
 * @return A string representation of the @c A2DPRole.
 */
inline std::string a2DPRoleToString(A2DPRole value) {
    switch (value) {
        case A2DPRole::NONE:
            return "NONE";
        case A2DPRole::SINK:
            return "SINK";
        case A2DPRole::SOURCE:
            return "SOURCE";
        default:
            return "UNKNOWN";
    }
}

/**
 * Overload for the @c A2DPRole enum. This will write the @c A2DPRole as a string to the provided stream.
 *
 * @param An ostream to send the @c A2DPRole as a string.
 * @param The @c A2DPRole to convert.
 * @return The stream.
 */
inline std::ostream& operator<<(std::ostream& stream, const A2DPRole a2DPRole) {
    return stream << a2DPRoleToString(a2DPRole);
}

}  // namespace bluetooth
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_A2DPROLE_H_
