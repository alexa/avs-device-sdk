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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SYSTEMCOMPONENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SYSTEMCOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/UserInactivityMonitorInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

/**
 * Manufactory Component definition for the System Capability Agent and related handlers.
 */
using SystemComponent = acsdkManufactory::Component<
    std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>>;

/**
 * Get the @c Manufactory component for System.
 *
 * @param sendPeriod Optional. The period of send events for the @c UserInactivityMonitorInterface in seconds.
 * Default value is 1 hour.
 *
 * @return The @c Manufactory component for System.
 */
SystemComponent getComponent(const std::chrono::milliseconds& sendPeriod = std::chrono::hours(1));

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SYSTEMCOMPONENT_H_
