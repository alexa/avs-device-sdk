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

#ifndef ALEXA_CLIENT_SDK_KWD_INCLUDE_KWD_ABSTRACTKEYWORDDETECTOR_H_
#define ALEXA_CLIENT_SDK_KWD_INCLUDE_KWD_ABSTRACTKEYWORDDETECTOR_H_

#include <mutex>
#include <unordered_set>

#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AVSCommon/SDKInterfaces/KeyWordDetectorStateObserverInterface.h>

namespace alexaClientSDK {
namespace kwd {

class AbstractKeywordDetector {
public:
    /**
     * Adds the specified observer to the list of observers to notify of key word detection events.
     *
     * @param keyWordObserver The observer to add.
     */
    void addKeyWordObserver(std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface> keyWordObserver);

    /**
     * Removes the specified observer to the list of observers to notify of key word detection events.
     *
     * @param keyWordObserver The observer to remove.
     */
    void removeKeyWordObserver(std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface> keyWordObserver);

    /**
     * Adds the specified observer to the list of observers to notify of key word detector state changes.
     *
     * @param keyWordDetectorStateObserver The observer to add.
     */
    void addKeyWordDetectorStateObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface> keyWordDetectorStateObserver);

    /**
     * Removes the specified observer to the list of observers to notify of key word detector state changes.
     *
     * @param keyWordDetectorStateObserver The observer to remove.
     */
    void removeKeyWordDetectorStateObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface> keyWordDetectorStateObserver);

    /**
     * Destructor.
     */
    virtual ~AbstractKeywordDetector() = default;

protected:
    /**
     * Constructor.
     *
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     */
    AbstractKeywordDetector(
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers =
            std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>>(),
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>
            keyWordDetectorStateObservers =
                std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>());

    /**
     * Notifies all keyword observers of the keyword detection.
     *
     * @param stream The stream in which the keyword was detected.
     * @param keyword The keyword detected.
     * @param beginIndex The absolute begin index of the first part of the keyword found within the @c stream.
     * @param endIndex The absolute end index of the last part of the keyword within the stream of the @c stream.
     * @param KWDMetadata Wake word engine metadata.
     */
    void notifyKeyWordObservers(
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        std::string keyword,
        avsCommon::avs::AudioInputStream::Index beginIndex,
        avsCommon::avs::AudioInputStream::Index endIndex,
        std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr) const;

    /**
     * Notifies all keyword detector state observers of state changes in the derived detector.
     *
     * @param state The state of the detector.
     */
    void notifyKeyWordDetectorStateObservers(
        avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState state);

    /**
     * Reads from the specified stream into the specified buffer and does the appropriate error checking and observer
     * notifications.
     *
     * @param reader The stream reader. This should be a blocking reader.
     * @param buf The buffer to read into.
     * @param nWords The number of words to read.
     * @param timeout The amount of time to wait for data to become available.
     * @param[out] errorOccurred Lets caller know if there were any errors that occurred with the read call.
     * @return The number of words successfully read.
     */
    ssize_t readFromStream(
        std::shared_ptr<avsCommon::avs::AudioInputStream::Reader> reader,
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        void* buf,
        size_t nWords,
        std::chrono::milliseconds timeout,
        bool* errorOccurred);

    /**
     * Checks to see if the @c audioFormat matches the platform endianness.
     *
     * @param audioFormat The audio format to check
     * @return @c true if the endiannesses don't match and @c false otherwise.
     */
    static bool isByteswappingRequired(avsCommon::utils::AudioFormat audioFormat);

private:
    /**
     * The observers to notify on key word detections. This should be locked with m_keyWordObserversMutex prior to
     * usage.
     */
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> m_keyWordObservers;

    /**
     * The observers to notify of state changes in the engine. This should be locked with
     * m_keyWordDetectorStateObserversMutex prior to usage.
     */
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>
        m_keyWordDetectorStateObservers;

    /// Lock to protect m_keyWordObservers when users wish to add or remove observers
    mutable std::mutex m_keyWordObserversMutex;

    /// Lock to protect m_keyWordDetectorStateObservers when users wish to add or remove observers
    mutable std::mutex m_keyWordDetectorStateObserversMutex;

    /**
     * The current state of the detector. This is stored so that we don't notify observers of the same change in state
     * multiple times.
     */
    avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState m_detectorState;
};

}  // namespace kwd
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_KWD_INCLUDE_KWD_ABSTRACTKEYWORDDETECTOR_H_
