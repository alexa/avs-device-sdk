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

#ifndef ACSDKKWDPROVIDER_KWDPROVIDER_KEYWORDDETECTORPROVIDER_H_
#define ACSDKKWDPROVIDER_KWDPROVIDER_KEYWORDDETECTORPROVIDER_H_

#include <memory>
#include <string>
#include <unordered_set>

#include <acsdkKWDImplementations/AbstractKeywordDetector.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/KeyWordDetectorStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AVSCommon/Utils/AudioFormat.h>

namespace alexaClientSDK {
namespace kwd {

/**
 * A class that creates a keyword detector.
 */
class KeywordDetectorProvider {
public:
    /**
     * Creates a @c KeywordDetector. The @c KeywordDetector that is created is determined by the create method
     * registered by the @c KWDRegistration Class. Only one @c KeywordDetector can be registered at a time to the
     * @c KeywordDetectorProvider.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     *
     * @return An @c KeywordDetector based on CMake configurations on success or @c nullptr if creation failed.
     */
    static std::unique_ptr<acsdkKWDImplementations::AbstractKeywordDetector> create(
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        avsCommon::utils::AudioFormat audioFormat,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>
            keyWordDetectorStateObservers);

    // Signature of functions to create an AbstractKeywordDetector.
    using KWDCreateMethod = std::unique_ptr<acsdkKWDImplementations::AbstractKeywordDetector> (*)(
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        avsCommon::utils::AudioFormat audioFormat,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>
            keyWordDetectorStateObservers);

    /**
     * Class that enables registration of a keyword detector's create functions.
     */
    class KWDRegistration {
    public:
        /**
         * Register an @c AbstractKeywordDetector to be returned by @c KeywordDetectorProvider. If a @c KeywordDetector
         * is already registered then this will log an error and do nothing.
         *
         * @param createFunction The function to use to create instances of the specified @c AbstractKeywordDetector.
         */
        KWDRegistration(KWDCreateMethod createFunction);
    };

private:
    /**
     * The keyword detector create method registered. @c m_KWDCreateMethod is initialized by the @c KWDRegistration
     * class constructor.
     */
    static KWDCreateMethod m_KWDCreateMethod;
};

}  // namespace kwd
}  // namespace alexaClientSDK

#endif  // ACSDKKWDPROVIDER_KWDPROVIDER_KEYWORDDETECTORPROVIDER_H_
