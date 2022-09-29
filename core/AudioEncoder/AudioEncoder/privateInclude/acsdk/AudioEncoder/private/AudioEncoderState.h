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

#ifndef ACSDK_AUDIOENCODER_PRIVATE_AUDIOENCODERSTATE_H_
#define ACSDK_AUDIOENCODER_PRIVATE_AUDIOENCODERSTATE_H_

#include <ostream>

namespace alexaClientSDK {
namespace audioEncoder {

/**
 * @brief Encoder states.
 */
enum class AudioEncoderState {
    /// Encoder is idle and can start encoding.
    IDLE,
    /// Encoder is encoding and can be stopped.
    ENCODING,
    /// Encoder has encountered an error.
    ENCODING_ERROR,
    /// Encoder is encoding and stop is requested.
    /// Encoder should finish current frame processing within the configured timeout.
    STOPPING,
    /// Encoder is encoding and immediate stop is requested.
    ABORTING
};

std::ostream& operator<<(std::ostream& out, AudioEncoderState state);

}  // namespace audioEncoder
}  // namespace alexaClientSDK

#endif  // ACSDK_AUDIOENCODER_PRIVATE_AUDIOENCODERSTATE_H_
