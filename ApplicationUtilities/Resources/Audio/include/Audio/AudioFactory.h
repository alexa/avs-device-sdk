/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_RESOURCES_AUDIO_INCLUDE_AUDIO_AUDIOFACTORY_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_RESOURCES_AUDIO_INCLUDE_AUDIO_AUDIOFACTORY_H_

#include <AVSCommon/SDKInterfaces/Audio/AudioFactoryInterface.h>

#include <AVSCommon/SDKInterfaces/Audio/AlertsAudioFactoryInterface.h>
#include "AVSCommon/SDKInterfaces/Audio/CommunicationsAudioFactoryInterface.h"
#include <AVSCommon/SDKInterfaces/Audio/NotificationsAudioFactoryInterface.h>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace resources {
namespace audio {

/**
 * A class that constructs an object to create unique streams to the audio data for the Alerts.
 */
class AudioFactory : public avsCommon::sdkInterfaces::audio::AudioFactoryInterface {
public:
    std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface> alerts() const override;
    std::shared_ptr<avsCommon::sdkInterfaces::audio::NotificationsAudioFactoryInterface> notifications() const override;
    std::shared_ptr<avsCommon::sdkInterfaces::audio::CommunicationsAudioFactoryInterface> communications()
        const override;
};

}  // namespace audio
}  // namespace resources
}  // namespace applicationUtilities
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_RESOURCES_AUDIO_INCLUDE_AUDIO_AUDIOFACTORY_H_
