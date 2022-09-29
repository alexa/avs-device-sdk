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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEECHINTERACTIONHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEECHINTERACTIONHANDLERINTERFACE_H_

#include <stdlib.h>
#include <chrono>
#include <future>
#include <memory>
#include <string>

#include <AIP/AudioInputProcessor.h>
#include <AIP/AudioProvider.h>
#include <AVSCommon/AVS/AudioInputStream.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * A @c SpeechInteractionHandler may be any client component who responds to WakeWork and TapToTalk Events
 * This specifies the interface to a @c SpeechInteractionHandler.
 */

class SpeechInteractionHandlerInterface {
public:
    virtual ~SpeechInteractionHandlerInterface() = default;

    /**
     * Begins a wake word initiated Alexa interaction.
     *
     * @param wakeWordAudioProvider The audio provider containing the audio data stream along with its metadata.
     * @param beginIndex The begin index of the keyword found within the stream.
     * @param endIndex The end index of the keyword found within the stream.
     * @param keyword The keyword that was detected.
     * @param KWDMetadata Wake word engine metadata.
     * @param startOfSpeechTimestamp Moment in time when user started talking to Alexa.
     * @return A future indicating whether the interaction was successfully started.
     */
    virtual std::future<bool> notifyOfWakeWord(
        capabilityAgents::aip::AudioProvider wakeWordAudioProvider,
        avsCommon::avs::AudioInputStream::Index beginIndex,
        avsCommon::avs::AudioInputStream::Index endIndex,
        std::string keyword,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp,
        std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr) = 0;

    /**
     * Begins a tap to talk initiated Alexa interaction. Note that this can also be used for wake word engines that
     * don't support providing both a begin and end index.
     *
     * @param tapToTalkAudioProvider The audio provider containing the audio data stream along with its metadata.
     * @param beginIndex An optional parameter indicating where in the stream to start reading from.
     * @param startOfSpeechTimestamp Moment in time when user started talking to Alexa. This parameter is optional
     * and it is used to measure user perceived latency. INVALID_INDEX may be used to explicitly communicate that
     * there is no wake work.
     * @return A future indicating whether the interaction was successfully started.
     */
    virtual std::future<bool> notifyOfTapToTalk(
        capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
        avsCommon::avs::AudioInputStream::Index beginIndex,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp = std::chrono::steady_clock::now()) = 0;

    /**
     * Begins a hold to talk initiated Alexa interaction.
     *
     * @param holdToTalkAudioProvider The audio provider containing the audio data stream along with its metadata.
     * @param startOfSpeechTimestamp Moment in time when user started talking to Alexa. This parameter is optional
     * and it is used to measure user perceived latency.
     * @param beginIndex An optional parameter indicating where in the stream to start reading from.
     * @return A future indicating whether the interaction was successfully started.
     */
    virtual std::future<bool> notifyOfHoldToTalkStart(
        capabilityAgents::aip::AudioProvider holdToTalkAudioProvider,
        std::chrono::steady_clock::time_point startOfSpeechTimestamp = std::chrono::steady_clock::now(),
        avsCommon::avs::AudioInputStream::Index beginIndex =
            capabilityAgents::aip::AudioInputProcessor::INVALID_INDEX) = 0;

    /**
     * Ends a hold to talk interaction by forcing the client to stop streaming audio data to the cloud and ending any
     * currently ongoing recognize interactions.
     *
     * @return A future indicating whether audio streaming was successfully stopped. This can be false if this was
     * called in the wrong state.
     */
    virtual std::future<bool> notifyOfHoldToTalkEnd() = 0;

    /**
     * Ends a tap to talk interaction by forcing the client to stop streaming audio data to the cloud and ending any
     * currently ongoing recognize interactions.
     *
     * @return A future indicating whether audio streaming was successfully stopped. This can be false if this was
     * called in the wrong state.
     */
    virtual std::future<bool> notifyOfTapToTalkEnd() = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEECHINTERACTIONHANDLERINTERFACE_H_
