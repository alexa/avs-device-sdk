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

#ifndef ACSDKDONOTDISTURB_DONOTDISTURBCOMPONENT_H_
#define ACSDKDONOTDISTURB_DONOTDISTURBCOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Annotated.h>
#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

#include "acsdkDoNotDisturb/DoNotDisturbCapabilityAgent.h"

namespace alexaClientSDK {
namespace acsdkDoNotDisturb {

/**
 * Definition of a Manufactory Component for DoNotDisturbCapabilityAgent.
 */
using DoNotDisturbComponent = acsdkManufactory::Component<
    std::shared_ptr<capabilityAgents::doNotDisturb::DoNotDisturbCapabilityAgent>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>>,
    acsdkManufactory::Import<std::shared_ptr<settings::storage::DeviceSettingStorageInterface>>>;

/**
 * Creates an manufactory component that exports @c DoNotDisturbCapabilityAgent.
 */
DoNotDisturbComponent getComponent();

}  // namespace acsdkDoNotDisturb
}  // namespace alexaClientSDK

#endif  // ACSDKDONOTDISTURB_DONOTDISTURBCOMPONENT_H_
