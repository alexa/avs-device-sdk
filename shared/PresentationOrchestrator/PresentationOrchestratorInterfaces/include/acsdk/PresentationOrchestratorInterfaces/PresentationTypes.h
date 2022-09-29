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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONTYPES_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONTYPES_H_

#include <chrono>
#include <ostream>
#include <string>

namespace alexaClientSDK {
namespace presentationOrchestratorInterfaces {
using PresentationRequestToken = uint64_t;

enum class PresentationLifespan {
    /// A short lived presentation which cannot be backgrounded, upon timeout will be dismissed and the next SHORT,
    /// LONG or PERMANENT presentation will be resumed
    TRANSIENT,

    /// A short lived presentation, not generally backgrounded but can be if a transient presentation is displayed.
    /// Upon timeout the next LONG or PERMANENT presentation will be resumed
    SHORT,

    /// A long lived presentation, may not have a timeout attached to it - will be backgrounded if another presentation
    /// is displayed
    LONG,

    /// Special use case for applications that are always running and are not expected to terminate, for example home
    /// screens. Permanent presentations can be backgrounded but cannot be dismissed as a result of back navigation.
    PERMANENT
};

/**
 * enumeration of all PresentationStates
 */
enum class PresentationState {
    /**
     * Presentation is in the foreground of its window and has focus (it is the primary target of user interaction)
     */
    FOREGROUND,

    /**
     * Presentation is in the foreground of the window, but is not the focused application
     */
    FOREGROUND_UNFOCUSED,

    /**
     * Presentation is no longer in the foreground and does not have focus
     */
    BACKGROUND,

    /**
     * Presentation has been dismissed
     */
    NONE,
};

/**
 * This function converts the provided @c PresentationState to a string.
 *
 * @param state The @c PresentationState to convert to a string.
 * @return The string conversion of @c state.
 */
inline std::string presentationStateToString(PresentationState state) {
    switch (state) {
        case PresentationState::FOREGROUND:
            return "FOREGROUND";
        case PresentationState::FOREGROUND_UNFOCUSED:
            return "FOREGROUND_UNFOCUSED";
        case PresentationState::BACKGROUND:
            return "BACKGROUND";
        case PresentationState::NONE:
            return "NONE";
    }
    return "Unknown State";
}

/**
 * Write a @c PresentationState value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param state The @c PresentationState value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const PresentationState& state) {
    return stream << presentationStateToString(state);
}

/**
 * This function converts the provided @c PresentationLifespan to a string.
 *
 * @param state The @c PresentationLifespan to convert to a string.
 * @return The string conversion of @c lifespan.
 */
inline std::string presentationLifespanToString(PresentationLifespan lifespan) {
    switch (lifespan) {
        case PresentationLifespan::TRANSIENT:
            return "TRANSIENT";
        case PresentationLifespan::SHORT:
            return "SHORT";
        case PresentationLifespan::LONG:
            return "LONG";
        case PresentationLifespan::PERMANENT:
            return "PERMANENT";
    }
    return "Unknown Lifespan";
}

/**
 * Write a @c PresentationLifespan value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param lifespan The @c PresentationLifespan value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const PresentationLifespan& lifespan) {
    return stream << presentationLifespanToString(lifespan);
}

struct PresentationOptions {
    /// The timeout for the document, use @c PresentationInterface::getTimeoutDisabled() to disable the timeout
    /// use @c PresentationInterface::getTimeoutDefault() to default timeout based on the presentation lifespan
    std::chrono::milliseconds timeout;

    /// Specifies the lifespan type for this presentation
    PresentationLifespan presentationLifespan;

    /// The AVS namespace associated with this presentation
    std::string interfaceName;

    /// Metadata associated with the interface
    /// For example, for the Alexa.Presentation.APL interface this should contain the presentation token
    std::string metadata;
};

}  // namespace presentationOrchestratorInterfaces
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONTYPES_H_
