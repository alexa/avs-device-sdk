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

#ifndef ALEXA_CLIENT_SDK_ACSDKDEVICESETUP_INCLUDE_ACSDKDEVICESETUP_DEVICESETUPFACTORY_H_
#define ALEXA_CLIENT_SDK_ACSDKDEVICESETUP_INCLUDE_ACSDKDEVICESETUP_DEVICESETUPFACTORY_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <acsdkDeviceSetupInterfaces/DeviceSetupInterface.h>

namespace alexaClientSDK {
namespace acsdkDeviceSetup {

/**
 * Creates a new DeviceSetup instance.
 *
 * @param messageSender Used for sending events.
 * @return A new @c DeviceSetup instance.
 */
std::shared_ptr<acsdkDeviceSetupInterfaces::DeviceSetupInterface> createDeviceSetup(
    const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender);

}  // namespace acsdkDeviceSetup
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACSDKDEVICESETUP_INCLUDE_ACSDKDEVICESETUP_DEVICESETUPFACTORY_H_
