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

#ifndef ACSDK_OPUSAUDIOENCODER_AUDIOENCODERFACTORY_H_
#define ACSDK_OPUSAUDIOENCODER_AUDIOENCODERFACTORY_H_

#include <memory>

#include <acsdk/AudioEncoderInterfaces/BlockAudioEncoderInterface.h>

namespace alexaClientSDK {
namespace opusAudioEncoder {

/**
 * @brief Creates block audio encoder.
 *
 * Method creates block audio encoder that uses OPUS library as a backend. This encoder supports only 16KHz single
 * channel or interleaved dual channel LPCM input, and produces OPUS encoded output.
 *
 * @return New encoder reference or nullptr on error.
 */
std::unique_ptr<audioEncoderInterfaces::BlockAudioEncoderInterface> createOpusAudioEncoder();

}  // namespace opusAudioEncoder
}  // namespace alexaClientSDK

#endif  // ACSDK_OPUSAUDIOENCODER_AUDIOENCODERFACTORY_H_