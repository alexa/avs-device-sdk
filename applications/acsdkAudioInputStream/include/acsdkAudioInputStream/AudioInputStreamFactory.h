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

#ifndef ACSDKAUDIOINPUTSTREAM_AUDIOINPUTSTREAMFACTORY_H_
#define ACSDKAUDIOINPUTSTREAM_AUDIOINPUTSTREAMFACTORY_H_

#include <memory>
#include <chrono>

#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

namespace alexaClientSDK {
namespace acsdkAudioInputStream {

/**
 * This class produces an @c AudioInputStream.
 */
class AudioInputStreamFactory {
public:
    /**
     * Method to create a factory method for an @c AudioInputStream.
     *
     * @param audioFormat The shared ptr to the @c AudioFormat of the stream
     * @param wordSize The size of each word within the stream.
     * @param maxReaders The maximum number of readers of the stream.
     * @param amountOfAudioDataInBuffer The amount of audio data to keep in the ring buffer.
     *
     * @return A @c std::function that returns a @cstd::shared_ptr to a new instance of @c AudioInputStream.
     */
    static std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> createAudioInputStream(
        const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
        const size_t wordSize,
        const size_t maxReaders,
        const std::chrono::seconds amountOfAudioDataInBuffer);
};

}  // namespace acsdkAudioInputStream
}  // namespace alexaClientSDK

#endif  // ACSDKAUDIOINPUTSTREAM_AUDIOINPUTSTREAMFACTORY_H_
