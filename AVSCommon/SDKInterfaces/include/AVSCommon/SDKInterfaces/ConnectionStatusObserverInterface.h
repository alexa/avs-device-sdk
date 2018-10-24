/*
 * Copyright 2016-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONNECTIONSTATUSOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONNECTIONSTATUSOBSERVERINTERFACE_H_

#include <iostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This class allows a client to be notified of changes to connection status to AVS.
 */
class ConnectionStatusObserverInterface {
public:
    /**
     * This enum expresses the states that a logical AVS connection can be in.
     */
    enum class Status {
        /// ACL is not connected to AVS.
        DISCONNECTED,

        /// ACL is attempting to establish a connection to AVS.
        PENDING,

        /// ACL is connected to AVS.
        CONNECTED
    };

    /**
     * This enum expresses the reasons a connection status may change.
     */
    enum class ChangedReason {
        /// The non-reason, to be used when no reason is specified (i.e. the 'unset' value).
        NONE,

        /// The status changed to due to a successful operation.
        SUCCESS,

        /// The status changed due to an error from which there is no recovery.
        UNRECOVERABLE_ERROR,

        /// The connection status changed due to the client interacting with the Connection public api.
        ACL_CLIENT_REQUEST,

        /// The connection attempt failed due to the Connection object being disabled.
        ACL_DISABLED,

        /// The connection attempt failed due to DNS resolution timeout.
        DNS_TIMEDOUT,

        /// The connection attempt failed due to timeout.
        CONNECTION_TIMEDOUT,

        /// The connection attempt failed due to excessive load on the server.
        CONNECTION_THROTTLED,

        /// The access credentials provided to ACL were invalid.
        INVALID_AUTH,

        /// There was a timeout sending a ping request.
        PING_TIMEDOUT,

        /// There was a timeout writing to AVS.
        WRITE_TIMEDOUT,

        /// There was a timeout reading from AVS.
        READ_TIMEDOUT,

        /// There was an underlying protocol error.
        FAILURE_PROTOCOL_ERROR,

        /// There was an internal error within ACL.
        INTERNAL_ERROR,

        /// There was an internal error on the server.
        SERVER_INTERNAL_ERROR,

        /// The server asked the client to reconnect.
        SERVER_SIDE_DISCONNECT,

        /// The server endpoint has changed.
        SERVER_ENDPOINT_CHANGED
    };

    /**
     * Destructor.
     */
    virtual ~ConnectionStatusObserverInterface() = default;

    /**
     * Called when the AVS connection state changes.
     *
     * @param status The current connection status.
     * @param reason The reason the status change occurred.
     */
    virtual void onConnectionStatusChanged(const Status status, const ChangedReason reason) = 0;
};

/**
 * Write a @c ConnectionStatusObserverInterface::Status value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param status The ConnectionStatusObserverInterface::Status value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, ConnectionStatusObserverInterface::Status status) {
    switch (status) {
        case ConnectionStatusObserverInterface::Status::DISCONNECTED:
            stream << "DISCONNECTED";
            break;
        case ConnectionStatusObserverInterface::Status::PENDING:
            stream << "PENDING";
            break;
        case ConnectionStatusObserverInterface::Status::CONNECTED:
            stream << "CONNECTED";
            break;
    }
    return stream;
}

/**
 * Write a @c ConnectionStatusObserverInterface::ChangeReason value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param reason The ConnectionStatusObserverInterface::ChangeReason value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, ConnectionStatusObserverInterface::ChangedReason reason) {
    switch (reason) {
        case ConnectionStatusObserverInterface::ChangedReason::NONE:
            stream << "NONE";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::SUCCESS:
            stream << "SUCCESS";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::UNRECOVERABLE_ERROR:
            stream << "UNRECOVERABLE_ERROR";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST:
            stream << "ACL_CLIENT_REQUEST";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::ACL_DISABLED:
            stream << "ACL_DISABLED";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::DNS_TIMEDOUT:
            stream << "DNS_TIMEDOUT";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::CONNECTION_TIMEDOUT:
            stream << "CONNECTION_TIMEDOUT";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::CONNECTION_THROTTLED:
            stream << "CONNECTION_THROTTLED";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::INVALID_AUTH:
            stream << "INVALID_AUTH";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::PING_TIMEDOUT:
            stream << "PING_TIMEDOUT";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::WRITE_TIMEDOUT:
            stream << "WRITE_TIMEDOUT";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::READ_TIMEDOUT:
            stream << "READ_TIMEDOUT";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::FAILURE_PROTOCOL_ERROR:
            stream << "FAILURE_PROTOCOL_ERROR";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::INTERNAL_ERROR:
            stream << "INTERNAL_ERROR";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::SERVER_INTERNAL_ERROR:
            stream << "SERVER_INTERNAL_ERROR";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::SERVER_SIDE_DISCONNECT:
            stream << "SERVER_SIDE_DISCONNECT";
            break;
        case ConnectionStatusObserverInterface::ChangedReason::SERVER_ENDPOINT_CHANGED:
            stream << "SERVER_ENDPOINT_CHANGED";
            break;
    }
    return stream;
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONNECTIONSTATUSOBSERVERINTERFACE_H_
