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

#include <acsdkManufactory/ComponentAccumulator.h>

#include "System/SystemComponent.h"
#include "System/UserInactivityMonitor.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

/**
 * Helper that returns an std::function wrapping the @c UserInactivityMonitor::createUserInactivityMonitorInterface,
 * which captures the sendPeriod.
 *
 * @param sendPeriod The period of send events for the @c UserInactivityMonitorInterface in seconds.
 * @return A std::function that produces an instance of @c UserInactivityMonitorInterface.
 */
static std::function<std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface>(
    const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>&,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&,
    const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>&,
    const std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>&)>
getCreateUserInactivityMonitorInterface(const std::chrono::milliseconds& sendPeriod) {
    return [sendPeriod](
               const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
               const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
                   exceptionEncounteredSender,
               const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier,
               const std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>& directiveSequencer) {
        return UserInactivityMonitor::createUserInactivityMonitorInterface(
            messageSender, exceptionEncounteredSender, shutdownNotifier, directiveSequencer, sendPeriod);
    };
}

SystemComponent getComponent(const std::chrono::milliseconds& sendPeriod) {
    return acsdkManufactory::ComponentAccumulator<>().addRequiredFactory(
        getCreateUserInactivityMonitorInterface(sendPeriod));
}

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
