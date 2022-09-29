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

#ifndef ACSDK_SAMPLE_MICROPHONE_PORTAUDIOMICROPHONEADAPTER_H_
#define ACSDK_SAMPLE_MICROPHONE_PORTAUDIOMICROPHONEADAPTER_H_

#include <memory>
#include <AVSCommon/AVS/AudioInputStream.h>

#include <Audio/MicrophoneInterface.h>

namespace alexaClientSDK {
namespace sample {
namespace microphone {

std::unique_ptr<applicationUtilities::resources::audio::MicrophoneInterface> createPortAudioMicrophoneInterface(
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream);

}  // namespace microphone
}  // namespace sample
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_MICROPHONE_PORTAUDIOMICROPHONEADAPTER_H_
