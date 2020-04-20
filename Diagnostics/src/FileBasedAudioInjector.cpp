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

#include "Diagnostics/FileBasedAudioInjector.h"

#include <memory>
#include <string>

#include <AVSCommon/AVS/AudioInputStream.h>
#include <Diagnostics/AudioInjectorMicrophone.h>

namespace alexaClientSDK {
namespace diagnostics {

/// String to identify log entries originating from this file.
static const std::string TAG("FileBasedAudioInjector");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> FileBasedAudioInjector::getMicrophone(
    const std::shared_ptr<avsCommon::avs::AudioInputStream>& stream,
    const alexaClientSDK::avsCommon::utils::AudioFormat& compatibleAudioFormat) {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_microphone) {
        m_microphone = AudioInjectorMicrophone::create(stream, compatibleAudioFormat);
        if (!m_microphone) {
            ACSDK_CRITICAL(LX("Failed to create microphone"));
        }
    }
    return m_microphone;
}

bool FileBasedAudioInjector::injectAudio(const std::string& filepath) {
    ACSDK_DEBUG5(LX(__func__));
    std::vector<uint16_t> audioData;

    if (!m_microphone) {
        ACSDK_ERROR(LX("No microphone available for audio injection"));
        return false;
    }

    if (diagnostics::utils::readWavFileToBuffer(filepath, &audioData)) {
        m_microphone->injectAudio(audioData);
        return true;
    }

    return false;
}

FileBasedAudioInjector::~FileBasedAudioInjector() {
    if (m_microphone) {
        m_microphone.reset();
    }
}

}  // namespace diagnostics
}  // namespace alexaClientSDK
