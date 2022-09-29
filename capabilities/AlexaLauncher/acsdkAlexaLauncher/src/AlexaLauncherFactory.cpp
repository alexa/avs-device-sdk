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

#include "acsdkAlexaLauncher/AlexaLauncherCapabilityAgent.h"
#include "acsdkAlexaLauncher/AlexaLauncherFactory.h"

namespace alexaClientSDK {
namespace acsdkAlexaLauncher {

avsCommon::utils::Optional<AlexaLauncherFactory::AlexaLauncherCapabilityAgentData> AlexaLauncherFactory::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    std::shared_ptr<acsdkAlexaLauncherInterfaces::AlexaLauncherInterface> launcher,
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable) {
    AlexaLauncherFactory::AlexaLauncherCapabilityAgentData AlexaLauncherCapabilityAgentData;
    auto launcherCA = acsdkAlexaLauncher::AlexaLauncherCapabilityAgent::create(
        endpointId, launcher, contextManager, responseSender, exceptionSender, isProactivelyReported, isRetrievable);
    if (launcherCA) {
        AlexaLauncherCapabilityAgentData.capabilityConfigurationInterface = launcherCA;
        AlexaLauncherCapabilityAgentData.directiveHandler = launcherCA;
        AlexaLauncherCapabilityAgentData.requiresShutdown = launcherCA;
        return avsCommon::utils::Optional<AlexaLauncherFactory::AlexaLauncherCapabilityAgentData>(
            AlexaLauncherCapabilityAgentData);
    }

    return avsCommon::utils::Optional<AlexaLauncherFactory::AlexaLauncherCapabilityAgentData>();
}

}  // namespace acsdkAlexaLauncher
}  // namespace alexaClientSDK
