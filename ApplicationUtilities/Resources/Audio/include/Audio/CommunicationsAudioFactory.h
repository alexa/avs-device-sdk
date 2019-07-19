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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_RESOURCES_AUDIO_INCLUDE_AUDIO_COMMUNICATIONSAUDIOFACTORY_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_RESOURCES_AUDIO_INCLUDE_AUDIO_COMMUNICATIONSAUDIOFACTORY_H_

#include <AVSCommon/SDKInterfaces/Audio/CommunicationsAudioFactoryInterface.h>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace resources {
namespace audio {

/**
 * A class that delivers a stream to the audio data for Comms sounds.
 */
class CommunicationsAudioFactory : public avsCommon::sdkInterfaces::audio::CommunicationsAudioFactoryInterface {
public:
    std::function<std::unique_ptr<std::istream>()> callConnectedRingtone() const override;
    std::function<std::unique_ptr<std::istream>()> callDisconnectedRingtone() const override;
    std::function<std::unique_ptr<std::istream>()> outboundRingtone() const override;
    std::function<std::unique_ptr<std::istream>()> dropInConnectedRingtone() const override;
    std::function<std::unique_ptr<std::istream>()> callIncomingRingtone() const override;
};

}  // namespace audio
}  // namespace resources
}  // namespace applicationUtilities
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_RESOURCES_AUDIO_INCLUDE_AUDIO_COMMUNICATIONSAUDIOFACTORY_H_
