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

#ifndef ACSDKAPPLICATIONAUDIOPIPELINEFACTORY_APPLICATIONAUDIOPIPELINEFACTORYCOMPONENT_H_
#define ACSDKAPPLICATIONAUDIOPIPELINEFACTORY_APPLICATIONAUDIOPIPELINEFACTORYCOMPONENT_H_

#include <memory>

#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerRuntimeSetupInterface.h>
#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <Captions/CaptionManagerInterface.h>

namespace alexaClientSDK {
namespace acsdkApplicationAudioPipelineFactory {

/**
 * Definition of a Manufactory Component for the Gstreamer implementation of @c
 * ApplicationAudioPipelineFactoryInterface.
 */
using GstreamerApplicationAudioPipelineFactoryComponent = acsdkManufactory::Component<
    std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>,
    acsdkManufactory::Import<std::shared_ptr<acsdkEqualizerInterfaces::EqualizerRuntimeSetupInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>>,
    acsdkManufactory::Import<std::shared_ptr<captions::CaptionManagerInterface>>>;

/**
 * Creates an manufactory component that exports @c ApplicationAudioPipelineFactoryInterface.
 *
 * @return A component.
 */
GstreamerApplicationAudioPipelineFactoryComponent getComponent();

}  // namespace acsdkApplicationAudioPipelineFactory
}  // namespace alexaClientSDK

#endif  // ACSDKAPPLICATIONAUDIOPIPELINEFACTORY_APPLICATIONAUDIOPIPELINEFACTORYCOMPONENT_H_
