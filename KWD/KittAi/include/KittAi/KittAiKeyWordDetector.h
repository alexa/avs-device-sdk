/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_KWD_KITTAI_INCLUDE_KITTAI_KITTAIKEYWORDDETECTOR_H_
#define ALEXA_CLIENT_SDK_KWD_KITTAI_INCLUDE_KITTAI_KITTAIKEYWORDDETECTOR_H_

#include <atomic>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AVSCommon/SDKInterfaces/KeyWordDetectorStateObserverInterface.h>

#include "KWD/AbstractKeywordDetector.h"
#include "KittAi/SnowboyWrapper.h"

namespace alexaClientSDK {
namespace kwd {

class KittAiKeyWordDetector : public AbstractKeywordDetector {
public:
    /**
     * The configuration used by the KittAiKeyWordDetector to set up keywords to be notified of.
     */
    struct KittAiConfiguration {
        /// The path to the keyword model.
        std::string modelFilePath;

        /// The keyword.
        std::string keyword;

        /**
         * The sensitivity that the user wishes to be associated with the keyword. This should be a value between 0 and
         * 1. The higher this is, the more keyword detections will occur. However, more false alarms might also occur
         * more frequently. @see https://github.com/Kitt-AI/snowboy for more information regarding this.
         */
        double sensitivity;
    };

    /**
     * Creates a @c KittAiKeyWordDetector.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     * @param resourceFilePath The path to the resource file.
     * @param kittAiConfigurations A vector of @c KittAiConfiguration objects that will be used to initialize the
     * engine and keywords.
     * @param audioGain This controls whether to increase (>1) or decrease (<1) input volume.
     * @param applyFrontEnd Whether to apply frontend audio processing.
     * @param msToPushPerIteration The amount of data in milliseconds to push to Kitt.ai at a time. Smaller sizes will
     * lead to less delay but more CPU usage. Additionally, larger amounts of data fed into the engine per iteration
     * might lead longer delays before receiving keyword detection events. This has been defaulted to 20 milliseconds
     * as it is a good trade off between CPU usage and recognition delay.
     * @return A new @c KittAiKeyWordDetector, or @c nullptr if the operation failed.
     * @see https://github.com/Kitt-AI/snowboy for more information regarding @c audioGain and @c applyFrontEnd.
     */
    static std::unique_ptr<KittAiKeyWordDetector> create(
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        avsCommon::utils::AudioFormat audioFormat,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>
            keyWordDetectorStateObservers,
        const std::string& resourceFilePath,
        const std::vector<KittAiConfiguration> kittAiConfigurations,
        float audioGain,
        bool applyFrontEnd,
        std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(20));

    /**
     * Destructor.
     */
    ~KittAiKeyWordDetector() override;

private:
    /**
     * Constructor.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     * @param resourceFilePath The path to the resource file.
     * @param kittAiConfigurations A vector of @c KittAiConfiguration objects that will be used to initialize the
     * engine and keywords.
     * @param audioGain This controls whether to increase (>1) or decrease (<1) input volume.
     * @param applyFrontEnd Whether to apply frontend audio processing.
     * @param msToPushPerIteration The amount of data in milliseconds to push to Kitt.ai at a time. Smaller sizes will
     * lead to less delay but more CPU usage. Additionally, larger amounts of data fed into the engine per iteration
     * might lead longer delays before receiving keyword detection events. This has been defaulted to 20 milliseconds
     * as it is a good trade off between CPU usage and recognition delay.
     * @see https://github.com/Kitt-AI/snowboy for more information regarding @c audioGain and @c applyFrontEnd.
     */
    KittAiKeyWordDetector(
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        avsCommon::utils::AudioFormat audioFormat,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>
            keyWordDetectorStateObservers,
        const std::string& resourceFilePath,
        const std::vector<KittAiConfiguration> kittAiConfigurations,
        float audioGain,
        bool applyFrontEnd,
        std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(20));

    /**
     * Initializes the stream reader and kicks off a thread to read data from the stream. This function should only be
     * called once with each new @c KittAiKeyWordDetector.
     *
     * @param audioFormat The format of the audio data located within the stream.
     * @return @c true if the engine was initialized properly and @c false otherwise.
     */
    bool init(avsCommon::utils::AudioFormat audioFormat);

    /**
     * Checks to see if an @c avsCommon::utils::AudioFormat is compatible with Kitt.ai.
     *
     * @param audioFormat The audio format to check.
     * @return @c true if the audio format is compatible and @c false otherwise.
     */
    bool isAudioFormatCompatibleWithKittAi(avsCommon::utils::AudioFormat audioFormat);

    /// The main function that reads data and feeds it into the engine.
    void detectionLoop();

    /// Indicates whether the internal main loop should keep running.
    std::atomic<bool> m_isShuttingDown;

    /**
     * The Kitt.ai engine emits a number to indicate which keyword was detected. The number corresponds to the model
     * passed in. Since we can pass in multiple models, it's up to us to keep track which return value corrresponds to
     * which keyword. This map maps each keyword to a return value.
     */
    std::unordered_map<unsigned int, std::string> m_detectionResultsToKeyWords;

    /// The stream of audio data.
    const std::shared_ptr<avsCommon::avs::AudioInputStream> m_stream;

    /// The reader that will be used to read audio data from the stream.
    std::shared_ptr<avsCommon::avs::AudioInputStream::Reader> m_streamReader;

    /// Internal thread that reads audio from the buffer and feeds it to the Kitt.ai engine.
    std::thread m_detectionThread;

    /// The Kitt.ai engine instantiation.
    std::unique_ptr<SnowboyWrapper> m_kittAiEngine;

    /**
     * The max number of samples to push into the underlying engine per iteration. This will be determined based on the
     * sampling rate of the audio data passed in.
     */
    const size_t m_maxSamplesPerPush;
};

}  // namespace kwd
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_KWD_KITTAI_INCLUDE_KITTAI_KITTAIKEYWORDDETECTOR_H_
