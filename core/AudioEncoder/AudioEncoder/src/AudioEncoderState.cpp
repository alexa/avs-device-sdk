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

#include <acsdk/AudioEncoder/private/AudioEncoderState.h>

namespace alexaClientSDK {
namespace audioEncoder {

std::ostream& operator<<(std::ostream& out, AudioEncoderState state) {
    const char* stateStr;
    switch (state) {
        case AudioEncoderState::IDLE:
            stateStr = "IDLE";
            break;
        case AudioEncoderState::ENCODING:
            stateStr = "ENCODING";
            break;
        case AudioEncoderState::ENCODING_ERROR:
            stateStr = "ENCODING_ERROR";
            break;
        case AudioEncoderState::STOPPING:
            stateStr = "STOPPING";
            break;
        case AudioEncoderState::ABORTING:
            stateStr = "ABORTING";
            break;
        default:
            return out << "Unknown(" << static_cast<int>(state) << ")";
    }
    return out << stateStr;
}

}  // namespace audioEncoder
}  // namespace alexaClientSDK
