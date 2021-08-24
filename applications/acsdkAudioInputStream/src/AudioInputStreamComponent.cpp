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

#include "acsdkAudioInputStream/CompatibleAudioFormat.h"
#include "acsdkAudioInputStream/AudioInputStreamComponent.h"
#include "acsdkAudioInputStream/AudioInputStreamFactory.h"

namespace alexaClientSDK {
namespace acsdkAudioInputStream {

static std::function<std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream>(
    const std::shared_ptr<avsCommon::utils::AudioFormat>&)>
getCreateAudioInputStream(
    const size_t wordSize,
    const size_t maxReaders,
    const std::chrono::seconds amountOfAudioDataInBuffer) {
    return [wordSize, maxReaders, amountOfAudioDataInBuffer](
               const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat) {
        return AudioInputStreamFactory::createAudioInputStream(
            audioFormat, wordSize, maxReaders, amountOfAudioDataInBuffer);
    };
};

AudioInputStreamComponent getComponent(
    const size_t wordSize,
    const size_t maxReaders,
    const std::chrono::seconds amountOfAudioDataInBuffer) {
    return acsdkManufactory::ComponentAccumulator<>()
        .addRequiredFactory(getCreateAudioInputStream(wordSize, maxReaders, amountOfAudioDataInBuffer))
        .addRequiredFactory(CompatibleAudioFormat::getCompatibleAudioFormat);
}

}  // namespace acsdkAudioInputStream
}  // namespace alexaClientSDK
