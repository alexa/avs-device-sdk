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

#ifndef ACSDKEQUALIZERIMPLEMENTATIONS_EQUALIZERCOMPONENT_H_
#define ACSDKEQUALIZERIMPLEMENTATIONS_EQUALIZERCOMPONENT_H_

#include <memory>
#include <vector>

#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerModeControllerInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerStorageInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

#include "EqualizerController.h"

namespace alexaClientSDK {
namespace acsdkEqualizer {

/**
 * Creates an manufactory component that exports shared pointers to equalizer-related interfaces.
 *
 * @return A component.
 */
acsdkManufactory::Component<
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerStorageInterface>,
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerConfigurationInterface>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>>>
getComponent();

}  // namespace acsdkEqualizer
}  // namespace alexaClientSDK

#endif  // ACSDKEQUALIZERIMPLEMENTATIONS_EQUALIZERCOMPONENT_H_
