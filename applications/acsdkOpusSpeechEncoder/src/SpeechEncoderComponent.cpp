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

#include <acsdkManufactory/ComponentAccumulator.h>
#include <acsdk/AudioEncoder/AudioEncoderFactory.h>
#include <acsdk/AudioEncoderComponent/ComponentFactory.h>
#include <acsdk/OpusAudioEncoder/AudioEncoderFactory.h>

namespace alexaClientSDK {
namespace audioEncoderComponent {

using namespace acsdkManufactory;
using namespace audioEncoder;
using namespace audioEncoderInterfaces;
using namespace opusAudioEncoder;

/**
 * Helper to create OPUS-based audio encoder with default parameters.
 *
 * @return Audio encoder instance or nullptr on error.
 * @private
 */
static std::shared_ptr<AudioEncoderInterface> createAudioEncoderWithDefaultParams() {
    auto blockEncoder = createOpusAudioEncoder();
    auto encoder = createAudioEncoder(std::move(blockEncoder));
    return std::move(encoder);
}

AudioEncoderComponent getComponent() {
    return ComponentAccumulator<>().addRetainedFactory(createAudioEncoderWithDefaultParams);
}

}  // namespace audioEncoderComponent
}  // namespace alexaClientSDK
