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

#include "Audio/AudioFactory.h"

#include "Audio/AlertsAudioFactory.h"
#include "Audio/CommunicationsAudioFactory.h"
#include "Audio/NotificationsAudioFactory.h"
#include "Audio/SystemSoundAudioFactory.h"

namespace alexaClientSDK {
namespace applicationUtilities {
namespace resources {
namespace audio {

std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface> AudioFactory::createAudioFactoryInterface() {
    return std::make_shared<alexaClientSDK::applicationUtilities::resources::audio::AudioFactory>();
}

std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface> AudioFactory::alerts() const {
    return std::make_shared<AlertsAudioFactory>();
}

std::shared_ptr<avsCommon::sdkInterfaces::audio::NotificationsAudioFactoryInterface> AudioFactory::notifications()
    const {
    return std::make_shared<NotificationsAudioFactory>();
}

std::shared_ptr<avsCommon::sdkInterfaces::audio::CommunicationsAudioFactoryInterface> AudioFactory::communications()
    const {
    return std::make_shared<CommunicationsAudioFactory>();
}

std::shared_ptr<avsCommon::sdkInterfaces::audio::SystemSoundAudioFactoryInterface> AudioFactory::systemSounds() const {
    return std::make_shared<SystemSoundAudioFactory>();
}

}  // namespace audio
}  // namespace resources
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
