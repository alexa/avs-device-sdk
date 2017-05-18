/*
 * AbstractKeywordDetector.cpp
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

#include "AVSUtils/Logging/Logger.h"
 
#include "KWD/AbstractKeywordDetector.h"

namespace alexaClientSDK {
namespace kwd {

using namespace avsCommon;
using namespace avsCommon::sdkInterfaces;

void AbstractKeywordDetector::addKeyWordObserver(std::shared_ptr<KeyWordObserverInterface> keyWordObserver) {
    std::lock_guard<std::mutex> lock(m_keyWordObserversMutex);
    m_keyWordObservers.insert(keyWordObserver);
}

void AbstractKeywordDetector::removeKeyWordObserver(std::shared_ptr<KeyWordObserverInterface> keyWordObserver) {
    std::lock_guard<std::mutex> lock(m_keyWordObserversMutex);
    m_keyWordObservers.erase(keyWordObserver);
}

void AbstractKeywordDetector::addKeyWordDetectorStateObserver(
        std::shared_ptr<KeyWordDetectorStateObserverInterface> keyWordDetectorStateObserver) {
    std::lock_guard<std::mutex> lock(m_keyWordDetectorStateObserversMutex);
    m_keyWordDetectorStateObservers.insert(keyWordDetectorStateObserver);
}

void AbstractKeywordDetector::removeKeyWordDetectorStateObserver(
        std::shared_ptr<KeyWordDetectorStateObserverInterface> keyWordDetectorStateObserver) {
    std::lock_guard<std::mutex> lock(m_keyWordDetectorStateObserversMutex);
    m_keyWordDetectorStateObservers.erase(keyWordDetectorStateObserver);
}

AbstractKeywordDetector::AbstractKeywordDetector(
        std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers, 
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers) :
    m_keyWordObservers{keyWordObservers}, 
    m_keyWordDetectorStateObservers{keyWordDetectorStateObservers},
    m_detectorState{KeyWordDetectorStateObserverInterface::KeyWordDetectorState::STREAM_CLOSED} {
}

void AbstractKeywordDetector::notifyKeyWordObservers(
        std::shared_ptr<AudioInputStream> stream,
        std::string keyword,
        AudioInputStream::Index beginIndex,
        AudioInputStream::Index endIndex) const {
    std::lock_guard<std::mutex> lock(m_keyWordObserversMutex);
    for (auto keyWordObserver : m_keyWordObservers) {
        keyWordObserver->onKeyWordDetected(stream, keyword, beginIndex, endIndex);
    }
}

void AbstractKeywordDetector::notifyKeyWordDetectorStateObservers(
        KeyWordDetectorStateObserverInterface::KeyWordDetectorState state) {
    if (m_detectorState != state) {
        m_detectorState = state;
        std::lock_guard<std::mutex> lock(m_keyWordDetectorStateObserversMutex);
        for (auto keyWordDetectorStateObserver : m_keyWordDetectorStateObservers) {
            keyWordDetectorStateObserver->onStateChanged(
                m_detectorState);
        }
    }
}

ssize_t AbstractKeywordDetector::readFromStream(
        std::shared_ptr<avsCommon::sdkInterfaces::AudioInputStream::Reader> reader,
        std::shared_ptr<avsCommon::sdkInterfaces::AudioInputStream> stream,
        void * buf, 
        size_t nWords,
        std::chrono::milliseconds timeout, 
        bool* errorOccurred) {
    if (errorOccurred) {
        *errorOccurred = false;
    }
    ssize_t wordsRead = reader->read(buf, nWords, timeout);
    // Stream has been closed
    if (wordsRead == 0) {
        avsUtils::Logger::log("Reader stream has been closed");
        notifyKeyWordDetectorStateObservers(
            KeyWordDetectorStateObserverInterface::KeyWordDetectorState::STREAM_CLOSED);
        if (errorOccurred) {
            *errorOccurred = true;
        }
    // This represents some sort of error with the read() call
    } else if (wordsRead < 0) {
        switch (wordsRead) {
            case AudioInputStream::Reader::Error::OVERRUN:
                avsUtils::Logger::log(
                    "Reader of stream has been overwritten by " + 
                    std::to_string(reader->tell(AudioInputStream::Reader::Reference::BEFORE_WRITER) - 
                    stream->getDataSize()) + "words"
                );
                reader->seek(0, AudioInputStream::Reader::Reference::BEFORE_WRITER);
                break;
            case AudioInputStream::Reader::Error::TIMEDOUT:
                avsUtils::Logger::log("Reader of stream has timed out");
                break;
            default:
                // We should never get this since we are using a Blocking Reader.
                avsUtils::Logger::log("Unexpected return error from Reader");
                notifyKeyWordDetectorStateObservers(
                    KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ERROR);
                if (errorOccurred) {
                    *errorOccurred = true;
                }
                break;
        }
    }
    return wordsRead;
}

bool AbstractKeywordDetector::isByteswappingRequired(avsCommon::AudioFormat audioFormat) {
    bool isPlatformLittleEndian = false;
    int num = 1;
    char* firstBytePtr = reinterpret_cast<char*>(&num);
    if (*firstBytePtr == 1) {
        isPlatformLittleEndian = true;
    }

    bool isFormatLittleEndian = (audioFormat.endianness == avsCommon::AudioFormat::Endianness::LITTLE);
    return isPlatformLittleEndian != isFormatLittleEndian;
}

} // namespace kwd
} // namespace alexaClientSDK