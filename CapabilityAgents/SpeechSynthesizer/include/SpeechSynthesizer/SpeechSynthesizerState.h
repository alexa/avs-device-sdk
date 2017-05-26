/*
 * SpeechSynthesizerState.h
 *
 * Copyright (c) 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_SPEECH_SYNTHESIZER_INCLUDE_SPEECH_SYNTHESIZER_SPEECH_SYNTHESIZER_STATE_H_
#define ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_SPEECH_SYNTHESIZER_INCLUDE_SPEECH_SYNTHESIZER_SPEECH_SYNTHESIZER_STATE_H_

namespace alexaClientSDK {
namespace capabilityAgent {
namespace speechSynthesizer {

/**
 * This is an enum class used to indicate the state of the @c SpeechSynthesizer.
 */
enum class SpeechSynthesizerState {
    /// In this state, the @c SpeechSynthesizer is playing back the speech.
    PLAYING,

    /// In this state, the @c SpeechSynthesizer is idle and not playing speech.
    FINISHED
};

} // namespace speechSynthesizer
} // namespace capabilityAgent
} // namespace alexaClientSDK

#endif //ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_SPEECH_SYNTHESIZER_INCLUDE_SPEECH_SYNTHESIZER_SPEECH_SYNTHESIZER_STATE_H_
