/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDEREROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDEREROBSERVERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace renderer {

/**
 * An interface class which specifies an observer to an @c Alert renderer.
 */
class RendererObserverInterface {
public:
    /**
     * The states which a Renderer may be in.
     */
    enum class State {
        /// An uninitialized value.
        UNSET,
        /// The renderer has started rendering.
        STARTED,
        /// The renderer has stopped rendering due to being stopped via a direct api call.
        STOPPED,
        /// The renderer has completed rendering, if the renderer was initiated with a finite loop count.
        COMPLETED,
        /// The renderer has encountered an error.
        ERROR
    };

    /**
     * Destructor.
     */
    virtual ~RendererObserverInterface() = default;

    /**
     * A callback function to communicate a change in render state.
     *
     * @param state The current state of the renderer.
     * @param reason The reason for the change of state, if required.  This is typically set on an error.
     */
    virtual void onRendererStateChange(State state, const std::string& reason = "") = 0;

    /**
     * Convert a @c State to a @c std::string.
     *
     * @param state The @c State to convert.
     * @return The string representation of @c state.
     */
    static std::string stateToString(State state);
};

inline std::string RendererObserverInterface::stateToString(State state) {
    switch (state) {
        case State::UNSET:
            return "UNSET";
        case State::STARTED:
            return "STARTED";
        case State::STOPPED:
            return "STOPPED";
        case State::COMPLETED:
            return "COMPLETED";
        case State::ERROR:
            return "ERROR";
    }
    return "unknown State";
}

/**
 * Write a @c State value to an @c ostream.
 *
 * @param stream The stream to write the value to.
 * @param state The @c State value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const RendererObserverInterface::State& state) {
    return stream << RendererObserverInterface::stateToString(state);
}

}  // namespace renderer
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDEREROBSERVERINTERFACE_H_
