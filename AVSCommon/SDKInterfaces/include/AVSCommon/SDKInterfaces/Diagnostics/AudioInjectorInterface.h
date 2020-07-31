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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIAGNOSTICS_AUDIOINJECTORINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIAGNOSTICS_AUDIOINJECTORINTERFACE_H_

#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/Utils/AudioFormat.h>
#include <Audio/MicrophoneInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace diagnostics {

class AudioInjectorInterface {
public:
    /**
     * Returns a @c MicrophoneInterface instance.
     *
     * @param stream The shared data stream to write to.
     * @param compatibleAudioFormat The format of audio data.
     * @return a new instance of @MicrophoneInterface.
     */
    virtual std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> getMicrophone(
        const std::shared_ptr<avsCommon::avs::AudioInputStream>& stream,
        const alexaClientSDK::avsCommon::utils::AudioFormat& compatibleAudioFormat) = 0;

    /**
     * Injects audio into the audio buffer.
     *
     * @param filepath Location of audio file.
     * @return Whether audio was successfully injected.
     */
    virtual bool injectAudio(const std::string& filepath) = 0;

    /**
     * Destructor.
     */
    virtual ~AudioInjectorInterface() = default;
};

}  // namespace diagnostics
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIAGNOSTICS_AUDIOINJECTORINTERFACE_H_
