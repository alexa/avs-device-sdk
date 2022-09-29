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
#ifndef ACSDK_ALEXAREMOTEVIDEOPLAYER_ALEXAREMOTEVIDEOPLAYERFACTORY_H_
#define ACSDK_ALEXAREMOTEVIDEOPLAYER_ALEXAREMOTEVIDEOPLAYERFACTORY_H_

#include <memory>

#include <acsdk/AlexaRemoteVideoPlayerInterfaces/RemoteVideoPlayerInterface.h>

#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace alexaRemoteVideoPlayer {

/**
 * This factory can be used to create a AlexaRemoteVideoPlayerFactory object which could be a parameter for Capability
 * Agent construction.
 */
class AlexaRemoteVideoPlayerFactory {
    /**
     * This object contains the interfaces to interact with the AlexaRemoteVideoPlayer Capability Agent.
     */
    struct RemoteVideoPlayerCapabilityAgentData {
        /// An interface used to handle Alexa.RemoteVideoPlayer directives.
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler;
        /// An interface that provides the configurations of the capabilities being implemented by this capability
        /// agent.
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CapabilityConfigurationInterface>
            capabilityConfigurationInterface;
        /// The object responsible for cleaning up this capability agent's objects during shutdown.
        std::shared_ptr<alexaClientSDK::avsCommon::utils::RequiresShutdown> requiresShutdown;
    };

public:
    /**
     * Creates a new RemoteVideoPlayer capability agent configuration.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param remoteVideoPlayer An interface that this object will use to perform the remote video player operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @return A @c RemoteVideoPlayerCapabilityAgentData object if successful,  otherwise an @c Optional object with no
     * valid value.
     */
    static avsCommon::utils::Optional<RemoteVideoPlayerCapabilityAgentData> create(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::shared_ptr<alexaRemoteVideoPlayerInterfaces::RemoteVideoPlayerInterface>& remoteVideoPlayer,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender);
};

}  // namespace alexaRemoteVideoPlayer
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAREMOTEVIDEOPLAYER_ALEXAREMOTEVIDEOPLAYERFACTORY_H_