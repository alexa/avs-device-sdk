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

#include "acsdk/AlexaRemoteVideoPlayer/AlexaRemoteVideoPlayerFactory.h"
#include "acsdk/AlexaRemoteVideoPlayer/private/AlexaRemoteVideoPlayerCapabilityAgent.h"

namespace alexaClientSDK {
namespace alexaRemoteVideoPlayer {

alexaClientSDK::avsCommon::utils::Optional<AlexaRemoteVideoPlayerFactory::RemoteVideoPlayerCapabilityAgentData>
AlexaRemoteVideoPlayerFactory::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<alexaRemoteVideoPlayerInterfaces::RemoteVideoPlayerInterface>& remoteVideoPlayer,
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender) {
    AlexaRemoteVideoPlayerFactory::RemoteVideoPlayerCapabilityAgentData remoteVideoPlayerCapabilityAgentData;
    auto remoteVideoPlayerCA = alexaRemoteVideoPlayer::AlexaRemoteVideoPlayerCapabilityAgent::create(
        endpointId, remoteVideoPlayer, contextManager, responseSender, exceptionSender);
    if (remoteVideoPlayerCA) {
        remoteVideoPlayerCapabilityAgentData.capabilityConfigurationInterface = remoteVideoPlayerCA;
        remoteVideoPlayerCapabilityAgentData.directiveHandler = remoteVideoPlayerCA;
        remoteVideoPlayerCapabilityAgentData.requiresShutdown = remoteVideoPlayerCA;
        return alexaClientSDK::avsCommon::utils::Optional<
            AlexaRemoteVideoPlayerFactory::RemoteVideoPlayerCapabilityAgentData>(remoteVideoPlayerCapabilityAgentData);
    }

    return alexaClientSDK::avsCommon::utils::Optional<
        AlexaRemoteVideoPlayerFactory::RemoteVideoPlayerCapabilityAgentData>();
}

}  // namespace alexaRemoteVideoPlayer
}  // namespace alexaClientSDK