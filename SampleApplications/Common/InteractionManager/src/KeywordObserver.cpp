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

#include "acsdk/Sample/InteractionManager/KeywordObserver.h"

#include "AVSCommon/Utils/Logger/LogEntry.h"
#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/// String to identify log entries originating from this file.
#define TAG "KeywordObserver"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<KeywordObserver> KeywordObserver::create(
    std::shared_ptr<defaultClient::DefaultClient> client,
    capabilityAgents::aip::AudioProvider audioProvider,
    std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector> keywordDetector) {
    auto keywordObserver = std::make_shared<KeywordObserver>(client, audioProvider);
    if (keywordDetector) {
        keywordDetector->addKeyWordObserver(keywordObserver);
    }

    return keywordObserver;
}

KeywordObserver::KeywordObserver(
    std::shared_ptr<defaultClient::DefaultClient> client,
    capabilityAgents::aip::AudioProvider audioProvider) :
        m_client{client},
        m_audioProvider{audioProvider} {
}

/**
 * Compute the start of speech timestamp used in UPL calculations. This is calculated by taking the time detected on
 * wakeword to the time and subtracting the duration of the wakeword uttered to determine time at the start of the
 * wakeword. @c startOfSpeechTimestamp must be set to the time on wakeword detected for this calculation to work. If
 * wakeword duration can not be calculated start of speech timestamp is left as the time on wakeword detected.
 *
 * @param wakewordBeingIndex The begin index of the first part of the keyword found within the @c stream.
 * @param wakewordEndIndex The end index of the last part of the keyword within the @c stream.
 * @param sampleRateHz The SampleRate in Hz of the @c stream.
 * @param stream The stream in which the keyword was detected.
 * @param startOfSpeechTimestamp Moment in time to calculate when user started talking to Alexa.
 */
static void computeStartOfSpeechTimestamp(
    const avsCommon::avs::AudioInputStream::Index wakewordBeginIndex,
    const avsCommon::avs::AudioInputStream::Index wakewordEndIndex,
    const int sampleRateHz,
    const std::shared_ptr<avsCommon::avs::AudioInputStream>& stream,
    std::chrono::steady_clock::time_point& startOfSpeechTimestamp) {
    // Create a reader to get the current index position
    static const auto startWithNewData = true;
    if (!stream) {
        ACSDK_WARN(LX("onKeywordDetected").m("Audio stream was null, using default offset."));
        return;
    }

    auto reader = stream->createReader(avsCommon::avs::AudioInputStream::Reader::Policy::NONBLOCKING, startWithNewData);
    if (!reader) {
        ACSDK_WARN(LX("onKeywordDetected").m("Reader was null, using default offset."));
        return;
    }

    // Get the current index position
    const auto currentIndex = reader->tell();

    if (currentIndex <= wakewordBeginIndex) {
        ACSDK_WARN(LX("onKeywordDetected").m("Index wrapping occurred, using default offset."));
        // Note: this should never happen, since it should not be possible for millions of years w/ 64-bit indexes.
        return;
    }

    // Translate the currentIndex position to a time duration elapsed since the end of wakeword
    const auto sampleRatePerMillisec = sampleRateHz / 1000;

    const auto timeSinceStartOfWW =
        std::chrono::milliseconds((currentIndex - wakewordBeginIndex) / sampleRatePerMillisec);
    ACSDK_DEBUG9(LX(__func__).d("timeSinceStartOfWW", timeSinceStartOfWW.count()));

    // Adjust the start of speech timestamp to be the start of the WW.
    startOfSpeechTimestamp -= timeSinceStartOfWW;
}

void KeywordObserver::onKeyWordDetected(
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
    std::string keyword,
    avsCommon::avs::AudioInputStream::Index beginIndex,
    avsCommon::avs::AudioInputStream::Index endIndex,
    std::shared_ptr<const std::vector<char>> KWDMetadata) {
    if (endIndex != avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX &&
        beginIndex == avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX) {
        if (m_client) {
            m_client->notifyOfTapToTalk(m_audioProvider, endIndex);
        }
    } else if (
        endIndex != avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX &&
        beginIndex != avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX) {
        if (m_client) {
            auto startOfSpeechTimestamp = std::chrono::steady_clock::now();
            computeStartOfSpeechTimestamp(
                beginIndex, endIndex, m_audioProvider.format.sampleRateHz, stream, startOfSpeechTimestamp);
            m_client->notifyOfWakeWord(
                m_audioProvider, beginIndex, endIndex, keyword, startOfSpeechTimestamp, KWDMetadata);
        }
    }
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
