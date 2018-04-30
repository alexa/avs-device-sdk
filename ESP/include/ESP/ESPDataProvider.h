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
#ifndef ALEXA_CLIENT_SDK_ESP_INCLUDE_ESP_ESPDATAPROVIDER_H_
#define ALEXA_CLIENT_SDK_ESP_INCLUDE_ESP_ESPDATAPROVIDER_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "DA_Metrics/FrameEnergyComputing.h"
#include "VAD_Features/VAD_class.h"

#include <AIP/ESPData.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <ESP/ESPDataProviderInterface.h>

namespace alexaClientSDK {
namespace esp {

/**
 * The ESPDataProvider is used to connect the sample app with the ESP library. When enabled, the ESPDataProvider object
 * feeds the ESP Library constantly using its own thread.
 */
class ESPDataProvider : public ESPDataProviderInterface {
public:
    /**
     * Create a unique pointer for an ESPDataProvider.
     *
     * @param audioProvider Should have the audio input stream used by the wakeword engine and the input parameters.
     * @return A valid ESPDataProvider pointer if creation succeeds and a empty pointer if it fails.
     */
    static std::unique_ptr<ESPDataProvider> create(const capabilityAgents::aip::AudioProvider& audioProvider);

    /**
     * ESPDataProvider Destructor.
     */
    ~ESPDataProvider();

    /// @name Overridden ESPDataProviderInterface methods.
    /// @{
    capabilityAgents::aip::ESPData getESPData() override;
    bool isEnabled() const override;
    void disable() override;
    void enable() override;
    /// @}

    /**
     * Delete ESPDataProvider default constructor.
     */
    ESPDataProvider() = delete;

    /**
     * Delete ESPDataProvider copy constructor.
     */
    ESPDataProvider(const ESPDataProvider&) = delete;

    /**
     * Delete ESPDataProvider copy operator.
     */
    ESPDataProvider operator=(const ESPDataProvider&) = delete;

private:
    /**
     * ESP processing loop. This method will feed the ESP library with the audio input until the ESPDataProvider is
     * disabled.
     */
    void espLoop();

    /**
     * ESPDataProvider constructor.
     *
     * @param reader Audio input stream that should be used to feed the ESP library.
     * @param frameSize The audio frame size per ms in bits.
     */
    ESPDataProvider(std::unique_ptr<avsCommon::avs::AudioInputStream::Reader> reader, unsigned int frameSize);

    // Unique pointer to a valid stream reader.
    std::unique_ptr<avsCommon::avs::AudioInputStream::Reader> m_reader;

    /// Object responsible for VAD algorithm.
    VADClass m_vad;

    /// Object used to calculate the frame energy. The access to this variable is guarded by @c m_mutex.
    FrameEnergyClass m_frameEnergyCompute;

    /// Thread that keeps feeding audio to ESP library.
    std::thread m_thread;

    /// Serializes access to m_FrameEnergyCompute.
    std::mutex m_mutex;

    /// Indicates if ESP data is provided or not. The access to this variable is guarded by @c m_mutex.
    bool m_isEnabled;

    /// Indicates whether the internal main loop should keep running.
    std::atomic<bool> m_isShuttingDown;

    /// Keeps the frame size.
    unsigned int m_frameSize;
};

}  // namespace esp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ESP_INCLUDE_ESP_ESPDATAPROVIDER_H_
