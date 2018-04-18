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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_COMMUNICATIONSAUDIOFACTORYINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_COMMUNICATIONSAUDIOFACTORYINTERFACE_H_

#include <functional>
#include <istream>
#include <memory>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace audio {

/**
 * This is an interface to produce sounds for the Communications interface.
 */
class CommunicationsAudioFactoryInterface {
public:
    virtual ~CommunicationsAudioFactoryInterface() = default;

    virtual std::function<std::unique_ptr<std::istream>()> callConnectedRingtone() const = 0;
    virtual std::function<std::unique_ptr<std::istream>()> callDisconnectedRingtone() const = 0;
    virtual std::function<std::unique_ptr<std::istream>()> outboundRingtone() const = 0;
    virtual std::function<std::unique_ptr<std::istream>()> dropInConnectedRingtone() const = 0;
    virtual std::function<std::unique_ptr<std::istream>()> callIncomingRingtone() const = 0;
};

}  // namespace audio
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_COMMUNICATIONSAUDIOFACTORYINTERFACE_H_
