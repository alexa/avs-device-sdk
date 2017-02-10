/*
* Values.h
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_VALUES_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_VALUES_H_

/**
 * TODO: ACSDK-85 has been created to track moving these enums into files closer to where they have the most meaning.
 */

#include <string>

namespace alexaClientSDK {
namespace acl {

/**
 * This enum expresses the states that a logical AVS connection can be in.
 */
enum class ConnectionStatus {

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
enum class ConnectionChangedReason {

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
 * This enum expresses the various end-states that a send request could arrive at.
 */
enum class SendMessageStatus {
    /// The message has not yet been processed for sending.
    PENDING,

    /// The message was successfully sent.
    SUCCESS,

    /// The send failed because AVS was not connected.
    NOT_CONNECTED,

    /// The send failed because of timeout waiting for AVS response.
    TIMEDOUT,

    /// The send failed due to an underlying protocol error.
    PROTOCOL_ERROR,

    /// The send failed due to an internal error within ACL.
    INTERNAL_ERROR,

    /// The send failed due to an internal error on the server.
    SERVER_INTERNAL_ERROR,

    /// The send failed due to server refusing the request.
    REFUSED,

    /// The send failed due to server canceling it before the transmission completed.
    CANCELED,

    /// The send failed due to excessive load on the server.
    THROTTLED,

    /// The access credentials provided to ACL were invalid.
    INVALID_AUTH
};

/**
 * Utility function to convert a modern enum class to a string.
 * @param status The enum value
 * @return The string representation of the incoming value.
 */
std::string connectionStatusToString(ConnectionStatus status);

/**
 * Utility function to convert a modern enum class to a string.
 * @param status The enum value
 * @return The string representation of the incoming value.
 */
std::string connectionChangedReasonToString(ConnectionChangedReason reason);

/**
 * Utility function to convert a modern enum class to a string.
 * @param status The enum value
 * @return The string representation of the incoming value.
 */
std::string sendMessageStatusToString(SendMessageStatus status);

} // namespace acl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_VALUES_H_
