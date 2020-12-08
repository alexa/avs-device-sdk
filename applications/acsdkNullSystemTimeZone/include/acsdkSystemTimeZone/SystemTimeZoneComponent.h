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

#ifndef ACSDKSYSTEMTIMEZONE_SYSTEMTIMEZONECOMPONENT_H_
#define ACSDKSYSTEMTIMEZONE_SYSTEMTIMEZONECOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Component.h>
#include <AVSCommon/SDKInterfaces/SystemTimeZoneInterface.h>

namespace alexaClientSDK {
namespace acsdkSystemTimeZone {

/**
 * Manufactory Component definition for null implementation of SystemTimeZoneInterface
 */
using SystemTimeZoneComponent =
    acsdkManufactory::Component<std::shared_ptr<avsCommon::sdkInterfaces::SystemTimeZoneInterface>>;

/**
 * Get the Manufactory component for creating null instances of SystemTimeZoneInterface.
 *
 * @return The default @c Manufactory component for creating null instances of SystemTimeZoneInterface.
 */
SystemTimeZoneComponent getComponent();

}  // namespace acsdkSystemTimeZone
}  // namespace alexaClientSDK

#endif  // ACSDKSYSTEMTIMEZONE_SYSTEMTIMEZONECOMPONENT_H_
