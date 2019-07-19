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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_AUDIOFACTORYINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_AUDIOFACTORYINTERFACE_H_

#include <memory>

#include "AVSCommon/SDKInterfaces/Audio/AlertsAudioFactoryInterface.h"
#include "AVSCommon/SDKInterfaces/Audio/CommunicationsAudioFactoryInterface.h"
#include "AVSCommon/SDKInterfaces/Audio/NotificationsAudioFactoryInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace audio {

/**
 * This is the interface that distributes interfaces for various audio stream providers.
 */
class AudioFactoryInterface {
public:
    virtual ~AudioFactoryInterface() = default;

    /**
     * This shares a factory that produces audio streams for the alerts components.
     */
    virtual std::shared_ptr<AlertsAudioFactoryInterface> alerts() const = 0;

    /**
     * This shares a factory that produces audio streams for the notifications components.
     */
    virtual std::shared_ptr<NotificationsAudioFactoryInterface> notifications() const = 0;

    /**
     * This shares a factory that produces audio streams for the communications components.
     */
    virtual std::shared_ptr<CommunicationsAudioFactoryInterface> communications() const = 0;
};

}  // namespace audio
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_AUDIOFACTORYINTERFACE_H_
