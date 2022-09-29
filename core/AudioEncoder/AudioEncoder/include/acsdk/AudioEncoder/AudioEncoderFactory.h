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

#ifndef ACSDK_AUDIOENCODER_AUDIOENCODERFACTORY_H_
#define ACSDK_AUDIOENCODER_AUDIOENCODERFACTORY_H_

#include <memory>

#include <acsdk/AudioEncoderInterfaces/AudioEncoderInterface.h>
#include <acsdk/AudioEncoderInterfaces/BlockAudioEncoderInterface.h>
#include <acsdk/AudioEncoder/AudioEncoderParams.h>

namespace alexaClientSDK {
namespace audioEncoder {

/**
 * @brief Create audio encoder with default parameters.
 *
 * Method creates a new instance of audio encoder with default parameters.
 *
 * By default, audio encoder uses 10 milliseconds for read timeout, 100 milliseconds for write timeout, and 1000
 * milliseconds for stop timeout. The output stream will buffer up to 20 packets in the output stream and will allow
 * up to 10 readers.
 *
 * @param[in] blockAudioEncoder   The backend encoder implementation. This parameter must not be nullptr.
 *
 * @return New instance of audio encoder or nullptr on error.
 *
 * @see createAudioEncoderWithParams()
 * @see AudioEncoderParams
 */
std::unique_ptr<audioEncoderInterfaces::AudioEncoderInterface> createAudioEncoder(
    const std::shared_ptr<audioEncoderInterfaces::BlockAudioEncoderInterface>& blockAudioEncoder);

/**
 * @brief Create audio encoder with given parameters.
 *
 * @param[in] blockAudioEncoder   The backend encoder implementation. This parameter must not be nullptr.
 * @param[in] params              Encoder parameters.
 *
 * @return New instance of audio encoder or nullptr on error.
 *
 * @see createAudioEncoder()
 */
std::unique_ptr<audioEncoderInterfaces::AudioEncoderInterface> createAudioEncoderWithParams(
    const std::shared_ptr<audioEncoderInterfaces::BlockAudioEncoderInterface>& blockAudioEncoder,
    const AudioEncoderParams& params);

}  // namespace audioEncoder
}  // namespace alexaClientSDK

#endif  // ACSDK_AUDIOENCODER_AUDIOENCODERFACTORY_H_
