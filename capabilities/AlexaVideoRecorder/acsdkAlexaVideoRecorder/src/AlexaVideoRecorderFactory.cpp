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

#include "acsdkAlexaVideoRecorder/AlexaVideoRecorderCapabilityAgent.h"
#include "acsdkAlexaVideoRecorder/AlexaVideoRecorderFactory.h"

namespace alexaClientSDK {
namespace acsdkAlexaVideoRecorder {
using namespace avsCommon::utils;

Optional<AlexaVideoRecorderFactory::VideoRecorderCapabilityAgentData> AlexaVideoRecorderFactory::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<acsdkAlexaVideoRecorderInterfaces::VideoRecorderInterface>& videoRecorder,
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender) {
    AlexaVideoRecorderFactory::VideoRecorderCapabilityAgentData videoRecorderCapabilityAgentData;
    auto videoRecorderCA = acsdkAlexaVideoRecorder::AlexaVideoRecorderCapabilityAgent::create(
        endpointId, videoRecorder, contextManager, responseSender, exceptionSender);
    if (videoRecorderCA) {
        videoRecorderCapabilityAgentData.capabilityConfigurationInterface = videoRecorderCA;
        videoRecorderCapabilityAgentData.directiveHandler = videoRecorderCA;
        videoRecorderCapabilityAgentData.requiresShutdown = videoRecorderCA;
        return avsCommon::utils::Optional<AlexaVideoRecorderFactory::VideoRecorderCapabilityAgentData>(
            videoRecorderCapabilityAgentData);
    }

    return avsCommon::utils::Optional<AlexaVideoRecorderFactory::VideoRecorderCapabilityAgentData>();
}

}  // namespace acsdkAlexaVideoRecorder
}  // namespace alexaClientSDK
