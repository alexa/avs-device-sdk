/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITIESOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITIESOBSERVERINTERFACE_H_

#include <ostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This interface is used to observe changes to the state of the CapabilitiesDelegate.
 */
class CapabilitiesObserverInterface {
public:
    /// The enum State describes the state of the CapabilitiesDelegate.
    enum class State {
        /// CapabilitiesDelegate not yet published.
        UNINITIALIZED,
        /// The Capabilities API message went through without issues.
        SUCCESS,
        /// The message did not go through because of issues that need fixing.
        FATAL_ERROR,
        /// The message did not go through, but you can retry to see if you succeed.
        RETRIABLE_ERROR
    };

    /// The enum Error encodes possible errors which may occur when changing state.
    enum class Error {
        /// The state (and hence the error) has not been initialized.
        UNINITIALIZED,
        /// Success.
        SUCCESS,
        /// An unknown error occurred.
        UNKNOWN_ERROR,
        /// The authorization failed.
        FORBIDDEN,
        /// The server encountered a runtime error.
        SERVER_INTERNAL_ERROR,
        /// The request is missing a required parameter, has an invalid value, or is otherwise improperly formed.
        BAD_REQUEST
    };

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~CapabilitiesObserverInterface() = default;

    /**
     * Notification that an CapabilitiesDelegate state has changed.
     *
     * @note Implementations of this method must not call CapabilitiesDelegate methods because the CapabilitiesDelegate
     * may be in a 'locked' state at the time this call is made. If you do, then you may end up with a deadlock.
     *
     * @param newState The new state of the CapabilitiesDelegate.
     * @param newError The error associated to the state change.
     */
    virtual void onCapabilitiesStateChange(State newState, Error newError) = 0;
};

/**
 * Write a @c State value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param state The state value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const CapabilitiesObserverInterface::State& state) {
    switch (state) {
        case CapabilitiesObserverInterface::State::UNINITIALIZED:
            return stream << "UNINTIALIZED";
        case CapabilitiesObserverInterface::State::SUCCESS:
            return stream << "SUCCESS";
        case CapabilitiesObserverInterface::State::FATAL_ERROR:
            return stream << "FATAL_ERROR";
        case CapabilitiesObserverInterface::State::RETRIABLE_ERROR:
            return stream << "RETRIABLE_ERROR";
    }
    return stream << "Unknown CapabilitiesObserverInterface::State!: " << state;
}

/**
 * Write an @c Error value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param error The error value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const CapabilitiesObserverInterface::Error& error) {
    switch (error) {
        case CapabilitiesObserverInterface::Error::UNINITIALIZED:
            return stream << "UNINTIALIZED";
        case CapabilitiesObserverInterface::Error::SUCCESS:
            return stream << "SUCCESS";
        case CapabilitiesObserverInterface::Error::UNKNOWN_ERROR:
            return stream << "UNKNOWN_ERROR";
        case CapabilitiesObserverInterface::Error::FORBIDDEN:
            return stream << "FORBIDDEN";
        case CapabilitiesObserverInterface::Error::SERVER_INTERNAL_ERROR:
            return stream << "SERVER_INTERNAL_ERROR";
        case CapabilitiesObserverInterface::Error::BAD_REQUEST:
            return stream << "CLIENT_ERROR_BAD_REQUEST";
    }
    return stream << "Unknown CapabilitiesObserverInterface::Error!: " << error;
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITIESOBSERVERINTERFACE_H_
