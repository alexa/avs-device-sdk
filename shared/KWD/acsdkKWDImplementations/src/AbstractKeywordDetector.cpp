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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdkKWDImplementations/AbstractKeywordDetector.h"
#include "acsdkKWDImplementations/KeywordDetectorStateNotifier.h"
#include "acsdkKWDImplementations/KeywordNotifier.h"

namespace alexaClientSDK {
namespace acsdkKWDImplementations {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
#define TAG "AbstractKeywordDetector"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

void AbstractKeywordDetector::addKeyWordObserver(std::shared_ptr<KeyWordObserverInterface> keyWordObserver) {
    m_keywordNotifier->addObserver(keyWordObserver);
}

void AbstractKeywordDetector::removeKeyWordObserver(std::shared_ptr<KeyWordObserverInterface> keyWordObserver) {
    m_keywordNotifier->removeObserver(keyWordObserver);
}

void AbstractKeywordDetector::addKeyWordDetectorStateObserver(
    std::shared_ptr<KeyWordDetectorStateObserverInterface> keyWordDetectorStateObserver) {
    m_keywordDetectorStateNotifier->addObserver(keyWordDetectorStateObserver);
}

void AbstractKeywordDetector::removeKeyWordDetectorStateObserver(
    std::shared_ptr<KeyWordDetectorStateObserverInterface> keyWordDetectorStateObserver) {
    m_keywordDetectorStateNotifier->removeObserver(keyWordDetectorStateObserver);
}

AbstractKeywordDetector::AbstractKeywordDetector(
    std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
    std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
    bool supportsDavs) :
        m_detectorState{KeyWordDetectorStateObserverInterface::KeyWordDetectorState::STREAM_CLOSED},
        m_supportsDavs(supportsDavs) {
    m_keywordNotifier = KeywordNotifier::createKeywordNotifierInterface();
    for (auto kwObserver : keyWordObservers) {
        m_keywordNotifier->addObserver(kwObserver);
    }

    m_keywordDetectorStateNotifier = KeywordDetectorStateNotifier::createKeywordDetectorStateNotifierInterface();
    m_keywordDetectorStateNotifier->setAddObserverFunction(
        [this](std::shared_ptr<KeyWordDetectorStateObserverInterface> stateObserver) {
            std::lock_guard<std::mutex> lock(m_detectorStateMutex);
            stateObserver->onStateChanged(m_detectorState);
        });
    for (auto kwdStateObserver : keyWordDetectorStateObservers) {
        m_keywordDetectorStateNotifier->addObserver(kwdStateObserver);
    }
}

AbstractKeywordDetector::AbstractKeywordDetector(
    std::shared_ptr<acsdkKWDInterfaces::KeywordNotifierInterface> keywordNotifier,
    std::shared_ptr<acsdkKWDInterfaces::KeywordDetectorStateNotifierInterface> keyWordDetectorStateNotifier,
    bool supportsDavs) :
        m_keywordNotifier{keywordNotifier},
        m_keywordDetectorStateNotifier{keyWordDetectorStateNotifier},
        m_detectorState{KeyWordDetectorStateObserverInterface::KeyWordDetectorState::STREAM_CLOSED},
        m_supportsDavs(supportsDavs) {
    m_keywordDetectorStateNotifier->setAddObserverFunction(
        [this](std::shared_ptr<KeyWordDetectorStateObserverInterface> stateObserver) {
            std::lock_guard<std::mutex> lock(m_detectorStateMutex);
            stateObserver->onStateChanged(m_detectorState);
        });
}

void AbstractKeywordDetector::notifyKeyWordObservers(
    std::shared_ptr<AudioInputStream> stream,
    std::string keyword,
    AudioInputStream::Index beginIndex,
    AudioInputStream::Index endIndex,
    std::shared_ptr<const std::vector<char>> KWDMetadata) const {
    m_keywordNotifier->notifyObservers(
        [stream, keyword, beginIndex, endIndex, KWDMetadata](
            std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface> observer) {
            observer->onKeyWordDetected(stream, keyword, beginIndex, endIndex, KWDMetadata);
        });
}

void AbstractKeywordDetector::notifyKeyWordDetectorStateObservers(
    KeyWordDetectorStateObserverInterface::KeyWordDetectorState state) {
    std::lock_guard<std::mutex> lock(m_detectorStateMutex);
    if (m_detectorState != state) {
        m_detectorState = state;
        m_keywordDetectorStateNotifier->notifyObservers(
            [state](std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface> observer) {
                observer->onStateChanged(state);
            });
    }
}

ssize_t AbstractKeywordDetector::readFromStream(
    std::shared_ptr<avsCommon::avs::AudioInputStream::Reader> reader,
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
    void* buf,
    size_t nWords,
    std::chrono::milliseconds timeout,
    bool* errorOccurred) {
    if (errorOccurred) {
        *errorOccurred = false;
    }
    ssize_t wordsRead = reader->read(buf, nWords, timeout);
    // Stream has been closed
    if (wordsRead == 0) {
        ACSDK_DEBUG(LX("readFromStream").d("event", "streamClosed"));
        notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::STREAM_CLOSED);
        if (errorOccurred) {
            *errorOccurred = true;
        }
        // This represents some sort of error with the read() call
    } else if (wordsRead < 0) {
        switch (wordsRead) {
            case AudioInputStream::Reader::Error::OVERRUN:
                ACSDK_ERROR(LX("readFromStreamFailed")
                                .d("reason", "streamOverrun")
                                .d("numWordsOverrun",
                                   std::to_string(
                                       reader->tell(AudioInputStream::Reader::Reference::BEFORE_WRITER) -
                                       stream->getDataSize())));
                reader->seek(0, AudioInputStream::Reader::Reference::BEFORE_WRITER);
                break;
            case AudioInputStream::Reader::Error::TIMEDOUT:
                ACSDK_INFO(LX("readFromStreamFailed").d("reason", "readerTimeOut"));
                break;
            default:
                // We should never get this since we are using a Blocking Reader.
                ACSDK_ERROR(LX("readFromStreamFailed")
                                .d("reason", "unexpectedError")
                                // Leave as ssize_t to avoid messiness of casting to enum.
                                .d("error", wordsRead));

                notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ERROR);
                if (errorOccurred) {
                    *errorOccurred = true;
                }
                break;
        }
    }
    return wordsRead;
}

bool AbstractKeywordDetector::isDavsSupported() {
    return m_supportsDavs;
}

bool AbstractKeywordDetector::isByteswappingRequired(avsCommon::utils::AudioFormat audioFormat) {
    bool isPlatformLittleEndian = false;
    int num = 1;
    char* firstBytePtr = reinterpret_cast<char*>(&num);
    if (*firstBytePtr == 1) {
        isPlatformLittleEndian = true;
    }

    bool isFormatLittleEndian = (audioFormat.endianness == avsCommon::utils::AudioFormat::Endianness::LITTLE);
    return isPlatformLittleEndian != isFormatLittleEndian;
}

}  // namespace acsdkKWDImplementations
}  // namespace alexaClientSDK
