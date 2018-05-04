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

#include "AVSCommon/Utils/Bluetooth/FormattedAudioStreamAdapter.h"
#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace bluetooth {

/// String to identify log entries originating from this file.
static const std::string TAG("FormattedAudioStreamAdapter");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

FormattedAudioStreamAdapter::FormattedAudioStreamAdapter(const AudioFormat& audioFormat) {
    // Some compilers fail to initialize AudioFormat structure with a curly brackets notation.
    m_audioFormat = audioFormat;
}

AudioFormat FormattedAudioStreamAdapter::getAudioFormat() const {
    return m_audioFormat;
}

void FormattedAudioStreamAdapter::setListener(std::shared_ptr<FormattedAudioStreamAdapterListener> listener) {
    std::lock_guard<std::mutex> guard(m_readerFunctionMutex);
    m_listener = listener;
}

size_t FormattedAudioStreamAdapter::send(const unsigned char* buffer, size_t size) {
    if (!buffer) {
        ACSDK_ERROR(LX("sendFailed").d("reason", "buffer is null"));
        return 0;
    }

    if (0 == size) {
        ACSDK_ERROR(LX("sendFailed").d("reason", "size is 0"));
        return 0;
    }

    std::shared_ptr<FormattedAudioStreamAdapterListener> listener;

    {
        std::lock_guard<std::mutex> guard(m_readerFunctionMutex);
        listener = m_listener.lock();
    }

    if (listener) {
        listener->onFormattedAudioStreamAdapterData(m_audioFormat, buffer, size);
        return size;
    } else {
        return 0;
    }
}

}  // namespace bluetooth
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
