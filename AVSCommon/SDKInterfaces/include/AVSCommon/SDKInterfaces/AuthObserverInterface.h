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
        /// The authorization code is invalid, expired, revoked, or was issued to a different client.
        AUTHORIZATION_EXPIRED,
        /// The client specified the wrong token type.
        UNSUPPORTED_GRANT_TYPE
    };

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~AuthObserverInterface() = default;

    /**
     * Notification that an authorization state has changed.
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
            stream << "UNINTIALIZED";
            break;
        case AuthObserverInterface::State::REFRESHED:
            stream << "REFRESHED";
            break;
        case AuthObserverInterface::State::EXPIRED:
            stream << "EXPIRED";
            break;
        case AuthObserverInterface::State::UNRECOVERABLE_ERROR:
            stream << "UNRECOVERABLE_ERROR";
            break;
    }
    return stream;
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
            stream << "SUCCESS";
            break;
        case AuthObserverInterface::Error::UNKNOWN_ERROR:
            stream << "UNKNOWN_ERROR";
            break;
        case AuthObserverInterface::Error::AUTHORIZATION_FAILED:
            stream << "AUTHORIZATION_FAILED";
            break;
        case AuthObserverInterface::Error::UNAUTHORIZED_CLIENT:
            stream << "UNAUTHORIZED_CLIENT";
            break;
        case AuthObserverInterface::Error::SERVER_ERROR:
            stream << "SERVER_ERROR";
            break;
        case AuthObserverInterface::Error::INVALID_REQUEST:
            stream << "INVALID_REQUEST";
            break;
        case AuthObserverInterface::Error::AUTHORIZATION_EXPIRED:
            stream << "AUTHORIZATION_EXPIRED";
            break;
        case AuthObserverInterface::Error::UNSUPPORTED_GRANT_TYPE:
            stream << "UNSUPPORTED_GRANT_TYPE";
            break;
    }
    return stream;
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUTHOBSERVERINTERFACE_H_
