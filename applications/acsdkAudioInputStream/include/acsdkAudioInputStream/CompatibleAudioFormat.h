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

#ifndef ACSDKAUDIOINPUTSTREAM_COMPATIBLEAUDIOFORMAT_H_
#define ACSDKAUDIOINPUTSTREAM_COMPATIBLEAUDIOFORMAT_H_

#include <memory>

#include <AVSCommon/Utils/AudioFormat.h>

namespace alexaClientSDK {
namespace acsdkAudioInputStream {

/**
 * This class provides the compatible @c AudioFormat.
 */
class CompatibleAudioFormat {
public:
    /**
     * Factory method that returns a @c std::shared_ptr to the compatible @c AudioFormat.
     */
    static std::shared_ptr<avsCommon::utils::AudioFormat> getCompatibleAudioFormat();
};

}  // namespace acsdkAudioInputStream
}  // namespace alexaClientSDK

#endif  // ACSDKAUDIOINPUTSTREAM_COMPATIBLEAUDIOFORMAT_H_
