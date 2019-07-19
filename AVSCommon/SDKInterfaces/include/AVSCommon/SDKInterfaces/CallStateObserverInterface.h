/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CALLSTATEOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CALLSTATEOBSERVERINTERFACE_H_

#include <iostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * An interface to allow being notified of changes to the state of a call.
 */
class CallStateObserverInterface {
public:
    /// An enumeration representing the state of a call.
    enum class CallState {
        /// The call is connecting.
        CONNECTING,
        /// An incoming call is causing a ringtone to be played.
        INBOUND_RINGING,
        /// The call has successfully connected.
        CALL_CONNECTED,
        /// The call has ended.
        CALL_DISCONNECTED,
        /// No current call state to be relayed to the user.
        NONE
    };

    /**
     * Destructor
     */
    virtual ~CallStateObserverInterface() = default;

    /**
     * Allows the observer to react to a change in call state.
     *
     * @param state The new CallState.
     */
    virtual void onCallStateChange(CallState state) = 0;
};

/**
 * Write a @c CallState value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param state The @c CallState value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const CallStateObserverInterface::CallState& state) {
    switch (state) {
        case CallStateObserverInterface::CallState::CONNECTING:
            stream << "CONNECTING";
            return stream;
        case CallStateObserverInterface::CallState::INBOUND_RINGING:
            stream << "INBOUND_RINGING";
            return stream;
        case CallStateObserverInterface::CallState::CALL_CONNECTED:
            stream << "CALL_CONNECTED";
            return stream;
        case CallStateObserverInterface::CallState::CALL_DISCONNECTED:
            stream << "CALL_DISCONNECTED";
            return stream;
        case CallStateObserverInterface::CallState::NONE:
            stream << "NONE";
            return stream;
    }
    return stream << "UNKNOWN STATE";
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CALLSTATEOBSERVERINTERFACE_H_
