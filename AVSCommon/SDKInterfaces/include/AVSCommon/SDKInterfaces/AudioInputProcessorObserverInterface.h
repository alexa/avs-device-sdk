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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIOINPUTPROCESSOROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIOINPUTPROCESSOROBSERVERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/// A state observer for an @c AudioInputProcessor.
class AudioInputProcessorObserverInterface {
public:
    /// The different states the @c AudioInputProcessor can be in
    enum class State {
        /// In this state, the @c AudioInputProcessor is not waiting for or transmitting speech.
        IDLE,

        /// In this state, the @c AudioInputProcessor is waiting for a call to recognize().
        EXPECTING_SPEECH,

        /// In this state, the @c AudioInputProcessor is actively streaming speech.
        RECOGNIZING,

        /**
         * In this state, the @c AudioInputProcessor has finished streaming and is waiting for completion of an Event.
         * Note that @c recognize() calls are not permitted while in the @c BUSY state.
         */
        BUSY
    };

    /// Destructor.
    virtual ~AudioInputProcessorObserverInterface() = default;

    /**
     * This function is called when the state of the observed @c AudioInputProcessor changes.  This function will block
     * processing of audio inputs, so implementations should return quickly.
     *
     * @param state The new state of the @c AudioInputProcessor.
     */
    virtual void onStateChanged(State state) = 0;

    /**
     * This function converts the provided @c State to a string.
     *
     * @param state The @c State to convert to a string.
     * @return The string conversion of @c state.
     */
    static std::string stateToString(State state) {
        switch (state) {
            case State::IDLE:
                return "IDLE";
            case State::EXPECTING_SPEECH:
                return "EXPECTING_SPEECH";
            case State::RECOGNIZING:
                return "RECOGNIZING";
            case State::BUSY:
                return "BUSY";
        }
        return "Unknown State";
    }
};

/**
 * Write an @c AudioInputProcessorObserverInterface::State value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param state The @c AudioInputProcessorObserverInterface::State value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const AudioInputProcessorObserverInterface::State& state) {
    return stream << AudioInputProcessorObserverInterface::stateToString(state);
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIOINPUTPROCESSOROBSERVERINTERFACE_H_
