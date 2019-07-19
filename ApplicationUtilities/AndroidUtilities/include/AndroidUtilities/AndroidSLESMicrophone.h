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
#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_INCLUDE_ANDROIDUTILITIES_ANDROIDSLESMICROPHONE_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_INCLUDE_ANDROIDUTILITIES_ANDROIDSLESMICROPHONE_H_

#include <memory>
#include <SLES/OpenSLES.h>

#include <AndroidUtilities/AndroidSLESBufferQueue.h>
#include <AndroidUtilities/AndroidSLESEngine.h>
#include <AndroidUtilities/AndroidSLESObject.h>
#include <Audio/MicrophoneInterface.h>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {

class AndroidSLESBufferQueue;

/**
 * Class that represents the OpenSL ES microphone.
 *
 * This class is responsible for setting the OpenSL ES microphone recording state as well as to trigger
 * the @c AndroidSLESBufferQueue methods that manage the OpenSL ES microphone buffers.
 *
 * The method @c startStreamingMicrophoneData should be used to start recording, and it's counterpart is
 * @c stopStreamingMicrophoneData, which stops the recording and clear any unprocessed data.
 */
class AndroidSLESMicrophone : public applicationUtilities::resources::audio::MicrophoneInterface {
public:
    /**
     * AndroidSLESEngine constructor.
     *
     * @param engine Engine used to create the microphone, which should not be destroyed before the mic.
     * @param recorderObject Pointer to an AndroidSLESObject abstraction.
     * @param recorderInterface The interface representation of a recorder.
     * @param queue The buffer queue used for storing the recorded data.
     */
    AndroidSLESMicrophone(
        std::shared_ptr<AndroidSLESEngine> engine,
        std::shared_ptr<AndroidSLESObject> recorderObject,
        SLRecordItf recorderInterface,
        std::unique_ptr<AndroidSLESBufferQueue> queue);

    /**
     * Configure audio recorder to recognize mode.
     *
     * @return @c true if it suceeds, and @c false if it fails.
     */
    bool configureRecognizeMode();

    /// @name MicrophoneInterface methods
    /// @{
    bool stopStreamingMicrophoneData() override;
    bool startStreamingMicrophoneData() override;
    /// @}

    /**
     * Create audio sink configuration following AVS Speech Recognition parameters.
     *
     *  - 16bit Linear PCM
     *  - 16kHz sample rate
     *  - Single channel
     *  - Little endian byte order
     *
     * @return Struct with the audio sink configuration.
     */
    static SLDataSink createSinkConfiguration();

    /**
     * Create audio source configuration.
     *
     * @return Struct with the audio sink configuration.
     */
    static SLDataSource createSourceConfiguration();

    /**
     * Destructor.
     */
    ~AndroidSLESMicrophone();

private:
    /// Implement the stop streaming logic. This should only be called when holding the @c m_mutex.
    bool stopLocked();

    /// Save pointer to the engine, since it can only be destroyed once we are done with the recorder.
    /// @note The engine should be declared first so it will be destroyed after other openSl elements.
    std::shared_ptr<AndroidSLESEngine> m_engineObject;

    /// Object which implements the OpenSL ES microphone logic.
    std::shared_ptr<AndroidSLESObject> m_recorderObject;

    /// OpenSL ES recorder interface used to access the microphone methods.
    SLRecordItf m_recorderInterface;

    /// Buffer queue object used to manage the recorded data.
    std::unique_ptr<AndroidSLESBufferQueue> m_queue;

    /// Mutex used to synchronize all recorder operations.
    std::mutex m_mutex;
};

}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_INCLUDE_ANDROIDUTILITIES_ANDROIDSLESMICROPHONE_H_
