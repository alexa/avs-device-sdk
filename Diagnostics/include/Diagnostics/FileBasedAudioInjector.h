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

#ifndef ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_FILEBASEDAUDIOINJECTOR_H_
#define ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_FILEBASEDAUDIOINJECTOR_H_

#include "Diagnostics/AudioInjectorMicrophone.h"
#include "Diagnostics/DiagnosticsUtils.h"

#include <memory>
#include <string>

#include <AVSCommon/SDKInterfaces/Diagnostics/AudioInjectorInterface.h>
#include <Audio/MicrophoneInterface.h>

namespace alexaClientSDK {
namespace diagnostics {

/**
 * Utility class to inject audio into the SDK's shared data stream.
 */
class FileBasedAudioInjector : public avsCommon::sdkInterfaces::diagnostics::AudioInjectorInterface {
public:
    /**
     * Destructor.
     */
    ~FileBasedAudioInjector();

    /// @name AudioInjectorInterface methods
    /// @{
    std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> getMicrophone(
        const std::shared_ptr<avsCommon::avs::AudioInputStream>& stream,
        const alexaClientSDK::avsCommon::utils::AudioFormat& compatibleAudioFormat) override;
    bool injectAudio(const std::string& filepath) override;
    /// @}

private:
    /// The @c AudioInjectorMicrophone.
    std::shared_ptr<AudioInjectorMicrophone> m_microphone;
};

}  // namespace diagnostics
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_FILEBASEDAUDIOINJECTOR_H_
