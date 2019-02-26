/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_FORMATTEDAUDIOSTREAMADAPTERLISTENER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_FORMATTEDAUDIOSTREAMADAPTERLISTENER_H_

#include "AVSCommon/Utils/AudioFormat.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace bluetooth {

/**
 * Interface to be implemented by class listening for data from @c FormattedAudioStreamAdapter
 */
class FormattedAudioStreamAdapterListener {
public:
    /**
     * Method to receive data sent by @c FormattedAudioStreamAdapter
     * @param audioFormat Audio format of the data sent
     * @param buffer Pointer to the buffer containing the data
     * @param size Length of the data in the buffer in bytes.
     */
    virtual void onFormattedAudioStreamAdapterData(
        avsCommon::utils::AudioFormat audioFormat,
        const unsigned char* buffer,
        size_t size) = 0;

    /**
     * Destructor.
     */
    virtual ~FormattedAudioStreamAdapterListener() = default;
};

}  // namespace bluetooth
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_FORMATTEDAUDIOSTREAMADAPTERLISTENER_H_
