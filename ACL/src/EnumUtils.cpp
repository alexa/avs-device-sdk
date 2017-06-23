/*
 * EnumUtils.cpp
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


#include "ACL/EnumUtils.h"

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::sdkInterfaces;

std::string connectionStatusToString(ConnectionStatusObserverInterface::Status status) {
    switch (status) {
        case ConnectionStatusObserverInterface::Status::DISCONNECTED:
            return "DISCONNECTED";
        case ConnectionStatusObserverInterface::Status::PENDING:
            return "PENDING";
        case ConnectionStatusObserverInterface::Status::CONNECTED:
            return "CONNECTED";
    }

    return "connectionStatusToString_UNHANDLED_ERROR";
}

std::string connectionChangedReasonToString(ConnectionStatusObserverInterface::ChangedReason reason) {
    switch (reason) {
        case ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST:
            return "ACL_CLIENT_REQUEST";
        case ConnectionStatusObserverInterface::ChangedReason::ACL_DISABLED:
            return "ACL_DISABLED";
        case ConnectionStatusObserverInterface::ChangedReason::DNS_TIMEDOUT:
            return "DNS_TIMEDOUT";
        case ConnectionStatusObserverInterface::ChangedReason::CONNECTION_TIMEDOUT:
            return "CONNECTION_TIMEDOUT";
        case ConnectionStatusObserverInterface::ChangedReason::CONNECTION_THROTTLED:
            return "CONNECTION_THROTTLED";
        case ConnectionStatusObserverInterface::ChangedReason::INVALID_AUTH:
            return "INVALID_AUTH";
        case ConnectionStatusObserverInterface::ChangedReason::PING_TIMEDOUT:
            return "PING_TIMEDOUT";
        case ConnectionStatusObserverInterface::ChangedReason::WRITE_TIMEDOUT:
            return "WRITE_TIMEDOUT";
        case ConnectionStatusObserverInterface::ChangedReason::READ_TIMEDOUT:
            return "READ_TIMEDOUT";
        case ConnectionStatusObserverInterface::ChangedReason::FAILURE_PROTOCOL_ERROR:
            return "FAILURE_PROTOCOL_ERROR";
        case ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR:
            return "INTERNAL_ERROR";
        case ConnectionStatusObserverInterface::ChangedReason::SERVER_INTERNAL_ERROR:
            return "SERVER_INTERNAL_ERROR";
        case ConnectionStatusObserverInterface::ChangedReason::SERVER_SIDE_DISCONNECT:
            return "SERVER_SIDE_DISCONNECT";
        case ConnectionStatusObserverInterface::ChangedReason::SERVER_ENDPOINT_CHANGED:
            return "SERVER_ENDPOINT_CHANGED";
    }

    return "connectionChangedReasonToString_UNHANDLED_ERROR";
}

} // namespace acl
} // namespace alexaClientSDK
