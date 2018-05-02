/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_KWD_KWDPROVIDER_INCLUDE_KWDPROVIDER_KEYWORDDETECTORPROVIDER_H_
#define ALEXA_CLIENT_SDK_KWD_KWDPROVIDER_INCLUDE_KWDPROVIDER_KEYWORDDETECTORPROVIDER_H_

#include <memory>
#include <string>
#include <unordered_set>

#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AVSCommon/SDKInterfaces/KeyWordDetectorStateObserverInterface.h>
#include <KWD/AbstractKeywordDetector.h>

namespace alexaClientSDK {
namespace kwd {

/**
 * A class that creates a keyword detector.
 */
class KeywordDetectorProvider {
public:
    /**
     * Creates a @c KeywordDetector.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     * @param pathToInputFolder The path to the inputs folder containing data files needed by this application.
     *
     * @return An @c KeywordDetector based on CMake configurations on success or @c nullptr if creation failed.
     */
    static std::unique_ptr<kwd::AbstractKeywordDetector> create(
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        avsCommon::utils::AudioFormat audioFormat,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>
            keyWordDetectorStateObservers,
        const std::string& pathToInputFolder);
};

}  // namespace kwd
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_KWD_KWDPROVIDER_INCLUDE_KWDPROVIDER_KEYWORDDETECTORPROVIDER_H_
