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

/**
 * @file
 */
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUTHOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUTHOBSERVERINTERFACE_H_

#include <ostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This interface is used to observe changes to the state of authorization.
 */
class AuthObserverInterface {
public:
    /// The enum State describes the state of authorization.
    enum class State {
        /// Authorization not yet acquired.
        UNINITIALIZED,
        /// Authorization has been refreshed.
        REFRESHED,
        /// Authorization has expired.
        EXPIRED,
        /// Authorization failed in a manner that cannot be corrected by retry.
        UNRECOVERABLE_ERROR
    };

    /// The enum Error encodes possible errors which may occur when changing state.
    enum class Error {
        /// Success.
        SUCCESS,
        /// An unknown body containing no error field has been encountered.
        UNKNOWN_ERROR,
        /// The client authorization failed.
        AUTHORIZATION_FAILED,
        /// The client is not authorized to use authorization codes.
        UNAUTHORIZED_CLIENT,
        /// The server encountered a runtime error.
        SERVER_ERROR,
        /// The request is missing a required parameter, has an invalid value, or is otherwise improperly formed.
        INVALID_REQUEST,
        /// One of the values in the request was invalid.
        INVALID_VALUE,
        /// The authorization code is invalid, expired, revoked, or was issued to a different client.
        AUTHORIZATION_EXPIRED,
        /// The client specified the wrong token type.
        UNSUPPORTED_GRANT_TYPE,
        /// Invalid code pair provided in Code-based linking token request.
        INVALID_CODE_PAIR,
        /// Waiting for user to authorize the specified code pair.
        AUTHORIZATION_PENDING,
        /// Client should slow down in the rate of requests polling for an access token.
        SLOW_DOWN,
        /// Internal error in client code.
        INTERNAL_ERROR,
        /// Client ID not valid for use with code based linking.
        INVALID_CBL_CLIENT_ID
    };

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~AuthObserverInterface() = default;

    /**
     * Notification that an authorization state has changed.
     *
     * @note Implementations of this method must not call AuthDelegate methods because the AuthDelegate
     * may be in a 'locked' state at the time this call is made.
     *
     * @param newState The new state of the authorization token.
     * @param error The error associated to the state change.
     */
    virtual void onAuthStateChange(State newState, Error error) = 0;
};

/**
 * Write a @c State value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param state The state value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const AuthObserverInterface::State& state) {
    switch (state) {
        case AuthObserverInterface::State::UNINITIALIZED:
            return stream << "UNINTIALIZED";
        case AuthObserverInterface::State::REFRESHED:
            return stream << "REFRESHED";
        case AuthObserverInterface::State::EXPIRED:
            return stream << "EXPIRED";
        case AuthObserverInterface::State::UNRECOVERABLE_ERROR:
            return stream << "UNRECOVERABLE_ERROR";
    }
    return stream << "Unknown AuthObserverInterface::State!: " << state;
}

/**
 * Write an @c Error value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param error The error value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const AuthObserverInterface::Error& error) {
    switch (error) {
        case AuthObserverInterface::Error::SUCCESS:
            return stream << "SUCCESS";
        case AuthObserverInterface::Error::UNKNOWN_ERROR:
            return stream << "UNKNOWN_ERROR";
        case AuthObserverInterface::Error::AUTHORIZATION_FAILED:
            return stream << "AUTHORIZATION_FAILED";
        case AuthObserverInterface::Error::UNAUTHORIZED_CLIENT:
            return stream << "UNAUTHORIZED_CLIENT";
        case AuthObserverInterface::Error::SERVER_ERROR:
            return stream << "SERVER_ERROR";
        case AuthObserverInterface::Error::INVALID_REQUEST:
            return stream << "INVALID_REQUEST";
        case AuthObserverInterface::Error::INVALID_VALUE:
            return stream << "INVALID_VALUE";
        case AuthObserverInterface::Error::AUTHORIZATION_EXPIRED:
            return stream << "AUTHORIZATION_EXPIRED";
        case AuthObserverInterface::Error::UNSUPPORTED_GRANT_TYPE:
            return stream << "UNSUPPORTED_GRANT_TYPE";
        case AuthObserverInterface::Error::INVALID_CODE_PAIR:
            return stream << "INVALID_CODE_PAIR";
        case AuthObserverInterface::Error::AUTHORIZATION_PENDING:
            return stream << "AUTHORIZATION_PENDING";
        case AuthObserverInterface::Error::SLOW_DOWN:
            return stream << "SLOW_DOWN";
        case AuthObserverInterface::Error::INTERNAL_ERROR:
            return stream << "INTERNAL_ERROR";
        case AuthObserverInterface::Error::INVALID_CBL_CLIENT_ID:
            return stream << "INVALID_CBL_CLIENT_ID";
    }
    return stream << "Unknown AuthObserverInterface::Error!: " << error;
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUTHOBSERVERINTERFACE_H_
