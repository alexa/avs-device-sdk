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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_FORMATTEDAUDIOSTREAMADAPTER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_FORMATTEDAUDIOSTREAMADAPTER_H_

#include <memory>
#include <mutex>

#include "AVSCommon/Utils/AudioFormat.h"
#include "AVSCommon/Utils/Bluetooth/FormattedAudioStreamAdapterListener.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace bluetooth {

/**
 * A class providing the way to receive a sequence of audio data blocks and a format associated with it.
 *
 * This class should be used when you want to publish a real time audio stream of the specified format to zero or one
 * recipient without buffering the data. The receiving party may start listening at any moment. With no listener set
 * all the published data is lost.
 */
class FormattedAudioStreamAdapter {
public:
    /**
     * Constructor initializing the class with an @c AudioFormat.
     *
     * @param audioFormat @c AudioFormat describing the data being sent.
     */
    explicit FormattedAudioStreamAdapter(const AudioFormat& audioFormat);

    /**
     * Get @c AudioFormat associated with the class.
     *
     * @return @c AudioFormat associated with the class.
     */
    AudioFormat getAudioFormat() const;

    /**
     * Set the listener to receive data.
     *
     * @param listener the listener to receive data.
     */
    void setListener(std::shared_ptr<FormattedAudioStreamAdapterListener> listener);

    /**
     * Publish data to the listener.
     *
     * @param buffer Buffer containing the data
     * @param size Size of the data block in bytes. The value must be greater than zero.
     * @return number of bytes processed.
     */
    size_t send(const unsigned char* buffer, size_t size);

private:
    /// The @c AudioFormat associated with the class.
    AudioFormat m_audioFormat;

    /// the listener to receive data.
    std::weak_ptr<FormattedAudioStreamAdapterListener> m_listener;

    /// Mutex to guard listener changes.
    std::mutex m_readerFunctionMutex;
};

}  // namespace bluetooth
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_FORMATTEDAUDIOSTREAMADAPTER_H_
