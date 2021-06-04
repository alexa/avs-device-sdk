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

#ifndef ACSDKSPEECHENCODER_SPEECHENCODERCOMPONENT_H_
#define ACSDKSPEECHENCODER_SPEECHENCODERCOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Component.h>
#include <SpeechEncoder/SpeechEncoder.h>

namespace alexaClientSDK {
namespace acsdkSpeechEncoder {

/**
 * Manufactory Component definition for an Opus @c SpeechEncoder.
 */
using SpeechEncoderComponent = acsdkManufactory::Component<std::shared_ptr<speechencoder::SpeechEncoder>>;

/**
 * Get the Manufactory component for creating an Opus @c SpeechEncoder.
 *
 * @return The default @c Manufactory component for creating an Opus @c SpeechEncoder.
 */
SpeechEncoderComponent getComponent();

}  // namespace acsdkSpeechEncoder
}  // namespace alexaClientSDK

#endif  // ACSDKSPEECHENCODER_SPEECHENCODERCOMPONENT_H_
