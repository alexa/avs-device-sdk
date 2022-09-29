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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGERCOMPONENT_INCLUDE_ACSDK_SPEAKERMANAGER_CHANNELVOLUMEFACTORYCOMPONENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGERCOMPONENT_INCLUDE_ACSDK_SPEAKERMANAGER_CHANNELVOLUMEFACTORYCOMPONENT_H_

#include <memory>
#include <vector>

#include <acsdkManufactory/Component.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeFactoryInterface.h>

namespace alexaClientSDK {
namespace speakerManagerComponent {

/// @addtogroup Lib_acsdkSpeakerManagerComponent
/// @{

/**
 * @brief Component for ChannelVolumeFactoryInterface.
 *
 * Definition of a Manufactory component for the ChannelVolumeFactoryInterface.
 */
using ChannelVolumeFactoryComponent =
    acsdkManufactory::Component<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface>>;

/**
 * @brief Create component for ChannelVolumeFactoryInterface.
 *
 * Creates an manufactory component that exports a shared pointer to an implementation of @c
 * ChannelVolumeFactoryInterface.
 *
 * @return A component.
 */
ChannelVolumeFactoryComponent getChannelVolumeFactoryComponent();

/// @}

}  // namespace speakerManagerComponent
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGERCOMPONENT_INCLUDE_ACSDK_SPEAKERMANAGER_CHANNELVOLUMEFACTORYCOMPONENT_H_
