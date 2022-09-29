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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_DEFAULTCHANNELVOLUMEFACTORY_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_DEFAULTCHANNELVOLUMEFACTORY_H_

#include <AVSCommon/SDKInterfaces/ChannelVolumeFactoryInterface.h>
#include <acsdk/SpeakerManager/private/ChannelVolumeManager.h>

namespace alexaClientSDK {
namespace speakerManager {

/**
 * @brief Default channel volume factory implementation.
 *
 * The @c DefaultChannelVolumeFactory provides a default implementation of @c ChannelVolumeFactoryInterface
 * using the @c ChannelVolumeManager.
 *
 * @ingroup Lib_acsdkSpeakerManager
 */
class DefaultChannelVolumeFactory : public alexaClientSDK::avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface {
public:
    /// ChannelVolumeFactoryInterface Functions.
    /// @{
    virtual std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ChannelVolumeInterface>
    createChannelVolumeInterface(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> speaker,
        alexaClientSDK::avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        std::function<int8_t(int8_t)> volumeCurve) override;
    /// @}
};

}  // namespace speakerManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_DEFAULTCHANNELVOLUMEFACTORY_H_
