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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdkAudioInputStream/AudioInputStreamFactory.h"

namespace alexaClientSDK {
namespace acsdkAudioInputStream {

/// String to identify log entries originating from this file.
#define TAG "AudioInputStreamFactory"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> AudioInputStreamFactory::createAudioInputStream(
    const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
    const size_t wordSize,
    const size_t maxReaders,
    const std::chrono::seconds amountOfAudioDataInBuffer) {
    if (!audioFormat) {
        ACSDK_ERROR(LX("createAudioInputStreamFailed").d("isAudioFormatNull", !audioFormat));
        return nullptr;
    }

    /// The size of the ring buffer.
    size_t bufferSizeInSamples = audioFormat->sampleRateHz * amountOfAudioDataInBuffer.count();

    size_t bufferSize = alexaClientSDK::avsCommon::avs::AudioInputStream::calculateBufferSize(
        bufferSizeInSamples, wordSize, maxReaders);

    auto buffer = std::make_shared<alexaClientSDK::avsCommon::avs::AudioInputStream::Buffer>(bufferSize);
    std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedDataStream =
        alexaClientSDK::avsCommon::avs::AudioInputStream::create(buffer, wordSize, maxReaders);

    if (!sharedDataStream) {
        ACSDK_ERROR(LX("createAudioInputStreamFailed").m("null AudioInputStream"));
        return nullptr;
    }

    return sharedDataStream;
}

}  // namespace acsdkAudioInputStream
}  // namespace alexaClientSDK
