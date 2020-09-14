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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CHANNELVOLUMEFACTORYINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CHANNELVOLUMEFACTORYINTERFACE_H_

#include <memory>
#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * The @c ChannelVolumeFactoryInterface provide a factory interface that returns a @c
 * ChannelVolumeInterface implementation for an input @c SpeakerInterface.
 */
class ChannelVolumeFactoryInterface {
public:
    /**
     * Creates a @c ChannelVolumeInterface for the input @c SpeakerInterface.
     *
     * @param speaker input @c SpeakerInterface to be encapsulated.
     * @param type optional The @c ChannelVolumeInterface Type to be associated with.
     * @param volumeCurve optional volume curve to be used for channel volume attenuation.
     * @return @c ChannelVolumeInterface
     */
    virtual std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ChannelVolumeInterface>
    createChannelVolumeInterface(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> speaker,
        alexaClientSDK::avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type =
            alexaClientSDK::avsCommon::sdkInterfaces::ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
        std::function<int8_t(int8_t)> volumeCurve = nullptr) = 0;

    /**
     * Destructor
     */
    virtual ~ChannelVolumeFactoryInterface() = default;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CHANNELVOLUMEFACTORYINTERFACE_H_
