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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_SYSTEMSOUNDAUDIOFACTORYINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_SYSTEMSOUNDAUDIOFACTORYINTERFACE_H_

#include <functional>
#include <istream>
#include <memory>
#include <utility>

#include <AVSCommon/Utils/MediaType.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace audio {

/**
 * This is an interface to produce streams of the system sound audio resources.
 */
class SystemSoundAudioFactoryInterface {
public:
    /**
     * Destructor
     */
    virtual ~SystemSoundAudioFactoryInterface() = default;

    /**
     * Returns an audio stream to play for the end speech tone.
     *
     * @return audio stream of the end speech tone.
     */
    virtual std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> endSpeechTone()
        const = 0;

    /**
     * Returns an audio stream to play for the wake word notification tone.
     *
     * @return audio stream of the wake word notification tone.
     */
    virtual std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()>
    wakeWordNotificationTone() const = 0;
};

}  // namespace audio
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_SYSTEMSOUNDAUDIOFACTORYINTERFACE_H_
