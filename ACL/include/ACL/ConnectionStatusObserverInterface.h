/*
* ConnectionStatusObserverInterface.h
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_CONNECTION_STATUS_OBSERVER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_CONNECTION_STATUS_OBSERVER_INTERFACE_H_

namespace alexaClientSDK {
namespace acl {

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
     * @status The current connection status.
     * @reason The reason the status change occurred.
     */
    virtual void onConnectionStatusChanged(const Status status,
                                           const ChangedReason reason) = 0;
};

} // namespace acl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_CONNECTION_STATUS_OBSERVER_INTERFACE_H_
