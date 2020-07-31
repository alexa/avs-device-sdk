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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ALEXARESPONSETYPE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ALEXARESPONSETYPE_H_

#include <ostream>
#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * An enum class indicating possible response from the endpoint on a controller API call.
 * Response are derived from @see https://developer.amazon.com/docs/alexa/device-apis/alexa-errorresponse.html
 */
enum class AlexaResponseType {
    /// Success
    SUCCESS,

    /// Endpoint already in operation.
    ALREADY_IN_OPERATION,

    /// The bridge is unreachable or offline.
    BRIDGE_UNREACHABLE,

    /// The endpoint was busy and could not perform the requested operation.
    ENDPOINT_BUSY,

    /// The endpoint could not perform the requested operation as its battery was low.
    ENDPOINT_LOW_POWER,

    /// The endpoint is unreachable or offline.
    ENDPOINT_UNREACHABLE,

    /// The endpoint was busy and could not perform because its firmware is out of date.
    FIRMWARE_OUT_OF_DATE,

    /// The endpoint was busy and could not perform because it has experienced a hardware malfunction.
    HARDWARE_MALFUNCTION,

    /// The caller does not have the permission to perform specified operation on endpoint.
    INSUFFICIENT_PERMISSIONS,

    /// An error occurred that can't be described by one of the other error types.
    INTERNAL_ERROR,

    /// Invalid value or unsupported value passed.
    INVALID_VALUE,

    /// The endpoint can't handle the requested operation because it is in a calibration phase, such as warming up.
    NOT_CALIBRATED,

    /// The endpoint can't be set to the specified value because of its current mode of operation.
    NOT_SUPPORTED_IN_CURRENT_MODE,

    /// The endpoint is not in operation.
    NOT_IN_OPERATION,

    // The endpoint can't handle the request operation because it doesn't support the requested power level.
    POWER_LEVEL_NOT_SUPPORTED,

    /// The maximum rate at which an endpoint or bridge can handle the requests has been exceeded.
    RATE_LIMIT_EXCEEDED,

    /// The endpoint can't be set to the specified value because it's outside the acceptable temperature range.
    TEMPERATURE_VALUE_OUT_OF_RANGE,

    /// The endpoint can't be set to the specified value because it's outside the acceptable range.
    VALUE_OUT_OF_RANGE
};

/**
 * Insertion operator.
 *
 * @param stream The @c std::ostream we are inserting into.
 * @param responseType The @c AlexaResponseType whose name to insert into the stream.
 * @return The stream with the name inserted on success, "Unknown AlexaResponseType" on failure.
 */
inline std::ostream& operator<<(std::ostream& stream, AlexaResponseType responseType) {
    switch (responseType) {
        case AlexaResponseType::SUCCESS:
            return stream << "SUCCESS";
        case AlexaResponseType::ALREADY_IN_OPERATION:
            return stream << "ALREADY_IN_OPERATION";
        case AlexaResponseType::BRIDGE_UNREACHABLE:
            return stream << "BRIDGE_UNREACHABLE";
        case AlexaResponseType::ENDPOINT_BUSY:
            return stream << "ENDPOINT_BUSY";
        case AlexaResponseType::ENDPOINT_LOW_POWER:
            return stream << "ENDPOINT_LOW_POWER";
        case AlexaResponseType::ENDPOINT_UNREACHABLE:
            return stream << "ENDPOINT_UNREACHABLE";
        case AlexaResponseType::FIRMWARE_OUT_OF_DATE:
            return stream << "FIRMWARE_OUT_OF_DATE";
        case AlexaResponseType::HARDWARE_MALFUNCTION:
            return stream << "HARDWARE_MALFUNCTION";
        case AlexaResponseType::INSUFFICIENT_PERMISSIONS:
            return stream << "INSUFFICIENT_PERMISSIONS";
        case AlexaResponseType::INTERNAL_ERROR:
            return stream << "INTERNAL_ERROR";
        case AlexaResponseType::INVALID_VALUE:
            return stream << "INVALID_VALUE";
        case AlexaResponseType::NOT_CALIBRATED:
            return stream << "NOT_CALIBRATED";
        case AlexaResponseType::NOT_SUPPORTED_IN_CURRENT_MODE:
            return stream << "NOT_SUPPORTED_IN_CURRENT_MODE";
        case AlexaResponseType::NOT_IN_OPERATION:
            return stream << "NOT_IN_OPERATION";
        case AlexaResponseType::POWER_LEVEL_NOT_SUPPORTED:
            return stream << "POWER_LEVEL_NOT_SUPPORTED";
        case AlexaResponseType::RATE_LIMIT_EXCEEDED:
            return stream << "RATE_LIMIT_EXCEEDED";
        case AlexaResponseType::TEMPERATURE_VALUE_OUT_OF_RANGE:
            return stream << "TEMPERATURE_VALUE_OUT_OF_RANGE";
        case AlexaResponseType::VALUE_OUT_OF_RANGE:
            return stream << "VALUE_OUT_OF_RANGE";
    }
    return stream << "Unknown AlexaResponseType";
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ALEXARESPONSETYPE_H_
