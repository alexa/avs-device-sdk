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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEECHSYNTHESIZEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEECHSYNTHESIZEROBSERVERINTERFACE_H_

#include <iostream>
#include <vector>

#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/AudioAnalyzer/AudioAnalyzerState.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * Interface for observing a SpeechSynthesizer.
 */
class SpeechSynthesizerObserverInterface {
public:
    /**
     * This is an enum class used to indicate the state of the @c SpeechSynthesizer.
     */
    enum class SpeechSynthesizerState {
        /// In this state, the @c SpeechSynthesizer is playing back the speech.
        PLAYING,

        /// In this state, the @c SpeechSynthesizer is idle and not playing speech.
        FINISHED,

        /// In this state, the @c SpeechSynthesizer is idle due to a barge in.
        INTERRUPTED,

        /// In this state, the @c SpeechSynthesizer is gaining the channel focus while still not playing anything
        GAINING_FOCUS,

        /// In this state, the @c SpeechSynthesizer is losing the channel focus but not yet considered @c FINISHED
        LOSING_FOCUS
    };

    /**
     * Destructor.
     */
    virtual ~SpeechSynthesizerObserverInterface() = default;

    /**
     * Notification that the @c SpeechSynthesizer state has changed. Callback functions must return as soon as possible.
     * @param state The new state of the @c speechSynthesizer.
     * @param mediaSourceId The current media source id for SpeechSynthesizer
     * @param mediaPlayerState Optional state of the media player as of this state change. The Optional is blank
     *                         if the state is unavailable.
     * @param audioAnalyzerState states of the audio analyzers related to the speech output.
     */
    virtual void onStateChanged(
        SpeechSynthesizerState state,
        const avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId mediaSourceId,
        const avsCommon::utils::Optional<avsCommon::utils::mediaPlayer::MediaPlayerState>& mediaPlayerState,
        const std::vector<avsCommon::utils::audioAnalyzer::AudioAnalyzerState>& audioAnalyzerState) = 0;
};

/**
 * Write a @c State value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param state The state value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(
    std::ostream& stream,
    const SpeechSynthesizerObserverInterface::SpeechSynthesizerState state) {
    switch (state) {
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::PLAYING:
            stream << "PLAYING";
            break;
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::FINISHED:
            stream << "FINISHED";
            break;
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::INTERRUPTED:
            stream << "INTERRUPTED";
            break;
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::GAINING_FOCUS:
            stream << "GAINING_FOCUS";
            break;
        case SpeechSynthesizerObserverInterface::SpeechSynthesizerState::LOSING_FOCUS:
            stream << "LOSING_FOCUS";
            break;
    }
    return stream;
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEECHSYNTHESIZEROBSERVERINTERFACE_H_
