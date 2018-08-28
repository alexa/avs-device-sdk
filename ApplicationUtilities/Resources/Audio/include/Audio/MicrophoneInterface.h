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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_RESOURCES_AUDIO_INCLUDE_AUDIO_MICROPHONEINTERFACE_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_RESOURCES_AUDIO_INCLUDE_AUDIO_MICROPHONEINTERFACE_H_

#include <mutex>
#include <thread>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace resources {
namespace audio {

/// This acts as an interface to record audio input from a microphone.
class MicrophoneInterface {
public:
    /**
     * Stops streaming from the microphone.
     *
     * @return Whether the stop was successful.
     */
    virtual bool stopStreamingMicrophoneData() = 0;

    /**
     * Starts streaming from the microphone.
     *
     * @return Whether the start was successful.
     */
    virtual bool startStreamingMicrophoneData() = 0;

    /**
     * Destructor.
     */
    virtual ~MicrophoneInterface() = default;
};

}  // namespace audio
}  // namespace resources
}  // namespace applicationUtilities
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_RESOURCES_AUDIO_INCLUDE_AUDIO_MICROPHONEINTERFACE_H_
