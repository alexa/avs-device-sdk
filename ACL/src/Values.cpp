/*
* Values.cpp
*
* Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
 * TODO: ACSDK-85 has been created to track moving these enums into files closer to where they have the most meaning.
 */

#include "ACL/Values.h"

namespace alexaClientSDK {
namespace acl {

std::string connectionStatusToString(ConnectionStatus status) {
    switch(status) {
        case ConnectionStatus::DISCONNECTED:
            return "DISCONNECTED";
        case ConnectionStatus::PENDING:
            return "PENDING";
        case ConnectionStatus::CONNECTED:
            return "CONNECTED";
        default:
            return "CONNECTION_STATUS_TO_STRING_ERROR";
    }

    return "connectionStatusToString_UNHANDLED_ERROR";
}

std::string connectionChangedReasonToString(ConnectionChangedReason reason) {
    switch(reason) {
        case ConnectionChangedReason::ACL_CLIENT_REQUEST:
            return "ACL_CLIENT_REQUEST";
        case ConnectionChangedReason::ACL_DISABLED:
            return "ACL_DISABLED";
        case ConnectionChangedReason::DNS_TIMEDOUT:
            return "DNS_TIMEDOUT";
        case ConnectionChangedReason::CONNECTION_TIMEDOUT:
            return "CONNECTION_TIMEDOUT";
        case ConnectionChangedReason::CONNECTION_THROTTLED:
            return "CONNECTION_THROTTLED";
        case ConnectionChangedReason::INVALID_AUTH:
            return "INVALID_AUTH";
        case ConnectionChangedReason::PING_TIMEDOUT:
            return "PING_TIMEDOUT";
        case ConnectionChangedReason::WRITE_TIMEDOUT:
            return "WRITE_TIMEDOUT";
        case ConnectionChangedReason::READ_TIMEDOUT:
            return "READ_TIMEDOUT";
        case ConnectionChangedReason::FAILURE_PROTOCOL_ERROR:
            return "FAILURE_PROTOCOL_ERROR";
        case ConnectionChangedReason::INTERNAL_ERROR:
            return "INTERNAL_ERROR";
        case ConnectionChangedReason::SERVER_INTERNAL_ERROR:
            return "SERVER_INTERNAL_ERROR";
        default:
            return "CONNECTION_CHANGED_REASON_TO_STRING_ERROR";
    }

    return "connectionChangedReasonToString_UNHANDLED_ERROR";
}

std::string sendMessageStatusToString(SendMessageStatus status) {
    switch(status) {
        case SendMessageStatus::PENDING:
            return "PENDING";
        case SendMessageStatus::SUCCESS:
            return "SUCCESS";
        case SendMessageStatus::NOT_CONNECTED:
            return "NOT_CONNECTED";
        case SendMessageStatus::TIMEDOUT:
            return "TIMEDOUT";
        case SendMessageStatus::PROTOCOL_ERROR:
            return "PROTOCOL_ERROR";
        case SendMessageStatus::INTERNAL_ERROR:
            return "INTERNAL_ERROR";
        case SendMessageStatus::SERVER_INTERNAL_ERROR:
            return "SERVER_INTERNAL_ERROR";
        case SendMessageStatus::REFUSED:
            return "REFUSED";
        case SendMessageStatus::CANCELED:
            return "CANCELED";
        case SendMessageStatus::THROTTLED:
            return "THROTTLED";
        default:
            return "SEND_MESSAGE_STATUS_TO_STRING_ERROR";
    }

    return "sendMessageStatusToString_UNHANDLED_ERROR";
}

} // namespace acl
} // namespace alexaClientSDK
