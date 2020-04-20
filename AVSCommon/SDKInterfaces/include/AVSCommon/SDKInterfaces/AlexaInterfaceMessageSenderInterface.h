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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ALEXAINTERFACEMESSAGESENDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ALEXAINTERFACEMESSAGESENDERINTERFACE_H_

#include <memory>

#include <AVSCommon/AVS/AlexaResponseType.h>
#include <AVSCommon/AVS/AVSMessageEndpoint.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * Messaging interface to allow CapabilityAgents to send common AlexaInterface events.
 */
class AlexaInterfaceMessageSenderInterface {
public:
    /// The type of error when calling sendErrorResponseEvent().
    enum class ErrorResponseType {
        /// The operation can't be performed because the endpoint is already in operation.
        ALREADY_IN_OPERATION,

        /// The bridge is unreachable or offline.
        BRIDGE_UNREACHABLE,

        /// The endpoint can't handle the directive because it is performing another action.
        ENDPOINT_BUSY,

        /// The endpoint can't handle the directive because the battery power is too low.
        ENDPOINT_LOW_POWER,

        /// The endpoint is unreachable or offline.
        ENDPOINT_UNREACHABLE,

        /// The authorization credential provided by Alexa has expired.
        EXPIRED_AUTHORIZATION_CREDENTIAL,

        /// The endpoint can't handle the directive because it's firmware is out of date.
        FIRMWARE_OUT_OF_DATE,

        /// The endpoint can't handle the directive because it has experienced a hardware malfunction.
        HARDWARE_MALFUNCTION,

        /// AVS does not have permissions to perform the specified action on the endpoint.
        INSUFFICIENT_PERMISSIONS,

        /// An error occurred that can't be described by one of the other error types.
        INTERNAL_ERROR,

        /// The authorization credential provided by Alexa is invalid.
        INVALID_AUTHORIZATION_CREDENTIAL,

        /// The directive is not supported or is malformed.
        INVALID_DIRECTIVE,

        /// The directive contains a value that is not valid for the target endpoint.
        INVALID_VALUE,

        /// The endpoint does not exist, or no longer exists.
        NO_SUCH_ENDPOINT,

        /// The endpoint can't handle the directive because it is in a calibration phase, such as warming up.
        NOT_CALIBRATED,

        /// The endpoint can't be set to the specified value because of its current mode of operation.
        NOT_SUPPORTED_IN_CURRENT_MODE,

        /// The endpoint is not in operation.
        NOT_IN_OPERATION,

        /// The endpoint can't handle the directive because it doesn't support the requested power level.
        POWER_LEVEL_NOT_SUPPORTED,

        /// The maximum rate at which an endpoint or bridge can process directives has been exceeded.
        RATE_LIMIT_EXCEEDED,

        /// The endpoint can't be set to the specified value because it's outside the acceptable temperature range.
        TEMPERATURE_VALUE_OUT_OF_RANGE,

        /// The endpoint can't be set to the specified value because it's outside the acceptable range.
        VALUE_OUT_OF_RANGE
    };

    /**
     * Destructor.
     */
    virtual ~AlexaInterfaceMessageSenderInterface() = default;

    /**
     * Send an Alexa.Response event.  Since these events require context, the event will be enqueued and this method
     * will return immediately (non-blocking).  The message will be sent once context has been received from
     * ContextManager.
     *
     * @param instance The instance ID of the responding capability.
     * @param correlationToken The correlation token from the directive to which we are responding.
     * @param endpoint The @c AVSMessageEndpoint to identify the endpoint related to this event.
     * @param jsonPayload a JSON string representing the payload for the response event (optional).
     * @return true if the event was successfuly enqueued, false on failure.
     */
    virtual bool sendResponseEvent(
        const std::string& instance,
        const std::string& correlationToken,
        const avsCommon::avs::AVSMessageEndpoint& endpoint,
        const std::string& jsonPayload = "{}") = 0;

    /**
     * Send an Alexa.ErrorResponse event.  The message is enqueued for sending and this method returns immediately
     * (non-blocking).
     *
     * @param instance The instance ID of the responding capability.
     * @param correlationToken The correlation token from the directive to which we are responding.
     * @param endpoint The @c AVSMessageEndpoint to identify the endpoint related to this event.
     * @param errorType the error type.
     * @param errorMessage a string containing the error message (optional).
     * @return true if the message was sent successfully, false otherwise.
     */
    virtual bool sendErrorResponseEvent(
        const std::string& instance,
        const std::string& correlationToken,
        const avsCommon::avs::AVSMessageEndpoint& endpoint,
        const ErrorResponseType errorType,
        const std::string& errorMessage = "") = 0;

    /**
     * Send an Alexa.DeferredResponse event.  The message is enqueued for sending and this method returns immediately
     * (non-blocking).
     *
     * @param instance The instance ID of the responding capability.
     * @param correlationToken The correlation token from the directive to which we are responding.
     * @param estimatedDeferralInSeconds number of seconds until the response is expected.
     * @return true if the message was sent successfully, false otherwise.
     */
    virtual bool sendDeferredResponseEvent(
        const std::string& instance,
        const std::string& correlationToken,
        const int estimatedDeferralInSeconds = 0) = 0;

    /**
     * Convert an AlexaResponseType to its corresponding ErrorResponseType.  Note that any AlexaResponseType that does
     * not map to ErrorResponseType will return INTERNAL_ERROR.
     *
     * @param responseType the response type to convert.
     * @return the corresponding ErrorResponseType.
     */
    static ErrorResponseType alexaResponseTypeToErrorType(const avsCommon::avs::AlexaResponseType responseType);
};

inline AlexaInterfaceMessageSenderInterface::ErrorResponseType AlexaInterfaceMessageSenderInterface::
    alexaResponseTypeToErrorType(const avsCommon::avs::AlexaResponseType responseType) {
    switch (responseType) {
        case avs::AlexaResponseType::SUCCESS:  // not supported
            return ErrorResponseType::INTERNAL_ERROR;
        case avs::AlexaResponseType::ALREADY_IN_OPERATION:
            return ErrorResponseType::ALREADY_IN_OPERATION;
        case avs::AlexaResponseType::BRIDGE_UNREACHABLE:
            return ErrorResponseType::BRIDGE_UNREACHABLE;
        case avs::AlexaResponseType::ENDPOINT_BUSY:
            return ErrorResponseType::ENDPOINT_BUSY;
        case avs::AlexaResponseType::ENDPOINT_LOW_POWER:
            return ErrorResponseType::ENDPOINT_LOW_POWER;
        case avs::AlexaResponseType::ENDPOINT_UNREACHABLE:
            return ErrorResponseType::ENDPOINT_UNREACHABLE;
        case avs::AlexaResponseType::FIRMWARE_OUT_OF_DATE:
            return ErrorResponseType::FIRMWARE_OUT_OF_DATE;
        case avs::AlexaResponseType::HARDWARE_MALFUNCTION:
            return ErrorResponseType::HARDWARE_MALFUNCTION;
        case avs::AlexaResponseType::INSUFFICIENT_PERMISSIONS:
            return ErrorResponseType::INSUFFICIENT_PERMISSIONS;
        case avs::AlexaResponseType::INTERNAL_ERROR:
            return ErrorResponseType::INTERNAL_ERROR;
        case avs::AlexaResponseType::INVALID_VALUE:
            return ErrorResponseType::INVALID_VALUE;
        case avs::AlexaResponseType::NOT_CALIBRATED:
            return ErrorResponseType::NOT_CALIBRATED;
        case avs::AlexaResponseType::NOT_SUPPORTED_IN_CURRENT_MODE:
            return ErrorResponseType::NOT_SUPPORTED_IN_CURRENT_MODE;
        case avs::AlexaResponseType::NOT_IN_OPERATION:
            return ErrorResponseType::NOT_IN_OPERATION;
        case avs::AlexaResponseType::POWER_LEVEL_NOT_SUPPORTED:
            return ErrorResponseType::POWER_LEVEL_NOT_SUPPORTED;
        case avs::AlexaResponseType::RATE_LIMIT_EXCEEDED:
            return ErrorResponseType::RATE_LIMIT_EXCEEDED;
        case avs::AlexaResponseType::TEMPERATURE_VALUE_OUT_OF_RANGE:
            return ErrorResponseType::TEMPERATURE_VALUE_OUT_OF_RANGE;
        case avs::AlexaResponseType::VALUE_OUT_OF_RANGE:
            return ErrorResponseType::VALUE_OUT_OF_RANGE;
    }
    return ErrorResponseType::INTERNAL_ERROR;
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ALEXAINTERFACEMESSAGESENDERINTERFACE_H_
