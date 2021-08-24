
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

#ifndef ACSDKAUDIOINPUTSTREAM_AUDIOINPUTSTREAMCOMPONENT_H_
#define ACSDKAUDIOINPUTSTREAM_AUDIOINPUTSTREAMCOMPONENT_H_

#include <memory>
#include <chrono>

#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/Utils/AudioFormat.h>

namespace alexaClientSDK {
namespace acsdkAudioInputStream {

/// Default params for @c AudioInputStreamComponent
static const size_t WORD_SIZE = 2;
static const size_t MAX_READERS = 10;
static const std::chrono::seconds AMOUNT_OF_AUDIO_DATA_IN_BUFFER = std::chrono::seconds(15);

/**
 * Definition of a Manufactory Component for the default @c AudioInputStream.
 */
using AudioInputStreamComponent = acsdkManufactory::
    Component<std::shared_ptr<avsCommon::avs::AudioInputStream>, std::shared_ptr<avsCommon::utils::AudioFormat>>;

/**
 * Creates an manufactory component that exports @c AudioInputStream.
 *
 * @param wordSize The size of each word within the stream.
 * @param maxReaders The maximum number of readers of the stream.
 * @param amountOfAudioDataInBuffer The amount of audio data to keep in the ring buffer.
 *
 * @return A component.
 */
AudioInputStreamComponent getComponent(
    const size_t wordSize = WORD_SIZE,
    const size_t maxReaders = MAX_READERS,
    const std::chrono::seconds amountOfAudioDataInBuffer = AMOUNT_OF_AUDIO_DATA_IN_BUFFER);

}  // namespace acsdkAudioInputStream
}  // namespace alexaClientSDK

#endif  // ACSDKAUDIOINPUTSTREAM_AUDIOINPUTSTREAMCOMPONENT_H_
