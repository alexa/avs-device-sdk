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

#include <acsdk/Sample/Microphone/PortAudioMicrophoneAdapter.h>
#include <acsdk/Sample/Microphone/PortAudioMicrophoneWrapper.h>

namespace alexaClientSDK {
namespace sample {
namespace microphone {

using applicationUtilities::resources::audio::MicrophoneInterface;
using avsCommon::avs::AudioInputStream;

std::unique_ptr<MicrophoneInterface> createPortAudioMicrophoneInterface(std::shared_ptr<AudioInputStream> stream) {
    return PortAudioMicrophoneWrapper::create(std::move(stream));
}

}  // namespace microphone
}  // namespace sample
}  // namespace alexaClientSDK
