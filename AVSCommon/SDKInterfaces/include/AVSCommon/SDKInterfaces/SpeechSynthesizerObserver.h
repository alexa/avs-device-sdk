/*
 * SpeechSynthesizerObserver.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVS_COMMON_SDK_INTERFACES_INCLUDE_AVS_COMMON_SDK_INTERFACES_SPEECH_SYNTHESIZER_OBSERVER_H_
#define ALEXA_CLIENT_SDK_AVS_COMMON_SDK_INTERFACES_INCLUDE_AVS_COMMON_SDK_INTERFACES_SPEECH_SYNTHESIZER_OBSERVER_H_

#include <iostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * Interface for observing a SpeechSynthesizer.
 */
class SpeechSynthesizerObserver {
public:
    /**
     * This is an enum class used to indicate the state of the @c SpeechSynthesizer.
     */
    enum class SpeechSynthesizerState {
        /// In this state, the @c SpeechSynthesizer is playing back the speech.
        PLAYING,

        /// In this state, the @c SpeechSynthesizer is idle and not playing speech.
        FINISHED
    };

    /**
     * Destructor.
     */
    virtual ~SpeechSynthesizerObserver() = default;

    /**
     * Notification that the @c SpeechSynthesizer state has changed.
     * @param state The new state of the @c speechSynthesizer.
     */
    virtual void onStateChanged(SpeechSynthesizerState state) = 0;
};

/**
 * Write a @c State value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param state The state value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const SpeechSynthesizerObserver::SpeechSynthesizerState state) {
    switch (state) {
        case SpeechSynthesizerObserver::SpeechSynthesizerState::PLAYING:
            stream << "PLAYING";
            break;
        case SpeechSynthesizerObserver::SpeechSynthesizerState::FINISHED:
            stream << "FINISHED";
            break;
    }
    return stream;
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVS_COMMON_SDK_INTERFACES_INCLUDE_AVS_COMMON_SDK_INTERFACES_SPEECH_SYNTHESIZER_OBSERVER_H_
