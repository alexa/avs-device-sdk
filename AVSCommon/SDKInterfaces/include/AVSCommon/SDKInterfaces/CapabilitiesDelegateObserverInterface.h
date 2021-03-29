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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITIESDELEGATEOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITIESDELEGATEOBSERVERINTERFACE_H_

#include <ostream>
#include <vector>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This interface is used to observe changes to the state of the CapabilitiesDelegate.
 */
class CapabilitiesDelegateObserverInterface {
public:
    /// The enum State describes the state of the CapabilitiesDelegate.
    enum class State {
        /// CapabilitiesDelegate is uninitialized.
        UNINITIALIZED,
        /// The Capabilities API message went through without issues.
        SUCCESS,
        /// The message did not go through because of issues that need fixing.
        FATAL_ERROR,
        /// The message did not go through, but you can retry to see if you succeed.
        RETRIABLE_ERROR,
    };

    /// The enum Error encodes possible errors which may occur when changing state.
    enum class Error {
        /// The state (and hence the error) has not been initialized.
        UNINITIALIZED,
        /// Success.
        SUCCESS,
        /// An unknown error occurred.
        UNKNOWN_ERROR,
        /// The request was canceled.
        CANCELED,
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
    virtual ~CapabilitiesDelegateObserverInterface() = default;

    /**
     * Notification that an CapabilitiesDelegate state has changed.
     *
     * @note Implementations of this method must not call CapabilitiesDelegate methods because the CapabilitiesDelegate
     * may be in a 'locked' state at the time this call is made. If you do, then you may end up with a deadlock.
     *
     * @param newState The new state of the CapabilitiesDelegate.
     * @param newError The error associated to the state change.
     * @param addedOrUpdatedEndpointIds The endpoint identifiers of endpoints sent in the addOrUpdateReport.
     * @param deletedEndpointIds The endpoint identifiers of endpoints sent in the deleteReport.
     */
    virtual void onCapabilitiesStateChange(
        State newState,
        Error newError,
        const std::vector<std::string>& addedOrUpdatedEndpointIds,
        const std::vector<std::string>& deletedEndpointIds) = 0;
};

/**
 * Write a @c State value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param state The state value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const CapabilitiesDelegateObserverInterface::State& state) {
    switch (state) {
        case CapabilitiesDelegateObserverInterface::State::UNINITIALIZED:
            return stream << "UNINITIALIZED";
        case CapabilitiesDelegateObserverInterface::State::SUCCESS:
            return stream << "SUCCESS";
        case CapabilitiesDelegateObserverInterface::State::FATAL_ERROR:
            return stream << "FATAL_ERROR";
        case CapabilitiesDelegateObserverInterface::State::RETRIABLE_ERROR:
            return stream << "RETRIABLE_ERROR";
    }
    return stream << "Unknown CapabilitiesDelegateObserverInterface::State!: " << state;
}

/**
 * Write an @c Error value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param error The error value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const CapabilitiesDelegateObserverInterface::Error& error) {
    switch (error) {
        case CapabilitiesDelegateObserverInterface::Error::UNINITIALIZED:
            return stream << "UNINITIALIZED";
        case CapabilitiesDelegateObserverInterface::Error::SUCCESS:
            return stream << "SUCCESS";
        case CapabilitiesDelegateObserverInterface::Error::UNKNOWN_ERROR:
            return stream << "UNKNOWN_ERROR";
        case CapabilitiesDelegateObserverInterface::Error::FORBIDDEN:
            return stream << "FORBIDDEN";
        case CapabilitiesDelegateObserverInterface::Error::SERVER_INTERNAL_ERROR:
            return stream << "SERVER_INTERNAL_ERROR";
        case CapabilitiesDelegateObserverInterface::Error::BAD_REQUEST:
            return stream << "CLIENT_ERROR_BAD_REQUEST";
        case CapabilitiesDelegateObserverInterface::Error::CANCELED:
            return stream << "CANCELED";
    }
    return stream << "Unknown CapabilitiesDelegateObserverInterface::Error!: " << error;
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITIESDELEGATEOBSERVERINTERFACE_H_
