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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_RESOURCES_AUDIO_INCLUDE_AUDIO_SYSTEMSOUNDAUDIOFACTORY_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_RESOURCES_AUDIO_INCLUDE_AUDIO_SYSTEMSOUNDAUDIOFACTORY_H_

#include <AVSCommon/SDKInterfaces/Audio/SystemSoundAudioFactoryInterface.h>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace resources {
namespace audio {

/**
 * This class implements the @c SystemSoundAudioFactoryInterface.
 */
class SystemSoundAudioFactory : public avsCommon::sdkInterfaces::audio::SystemSoundAudioFactoryInterface {
public:
    /// @name SystemSoundAudioFactoryInterface Functions
    /// @{
    std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> endSpeechTone()
        const override;
    std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()>
    wakeWordNotificationTone() const override;
    /// @}
};

}  // namespace audio
}  // namespace resources
}  // namespace applicationUtilities
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_RESOURCES_AUDIO_INCLUDE_AUDIO_SYSTEMSOUNDAUDIOFACTORY_H_
