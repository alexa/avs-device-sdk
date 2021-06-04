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

#ifndef ACSDKDEVICESETUP_DEVICESETUPCOMPONENT_H_
#define ACSDKDEVICESETUP_DEVICESETUPCOMPONENT_H_

#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <acsdkManufactory/Annotated.h>
#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>

#include <memory>

#include "acsdkDeviceSetupInterfaces/DeviceSetupInterface.h"

namespace alexaClientSDK {
namespace acsdkDeviceSetup {

/**
 * Manufactory Component definition for the @c DeviceSetupInterface.
 */
using DeviceSetupComponent = acsdkManufactory::Component<
    std::shared_ptr<acsdkDeviceSetupInterfaces::DeviceSetupInterface>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>>;

/**
 * Get the @c Manufactory component for creating an instance of @c DeviceSetupInterface.
 *
 * @return The @c Manufactory component for creating an instance of @c DeviceSetupInterface.
 */
DeviceSetupComponent getComponent();

}  // namespace acsdkDeviceSetup
}  // namespace alexaClientSDK

#endif  // ACSDKDEVICESETUP_DEVICESETUPCOMPONENT_H_
