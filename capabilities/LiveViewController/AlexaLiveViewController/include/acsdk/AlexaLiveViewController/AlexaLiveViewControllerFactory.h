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

#ifndef ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLER_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLER_ALEXALIVEVIEWCONTROLLERFACTORY_H_
#define ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLER_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLER_ALEXALIVEVIEWCONTROLLERFACTORY_H_

#include <memory>

#include <acsdk/AlexaLiveViewControllerInterfaces/LiveViewControllerInterface.h>
#include <acsdk/AlexaLiveViewControllerInterfaces/LiveViewControllerObserverInterface.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace alexaLiveViewController {

/**
 * This factory can be used to create a new @c AlexaLiveViewControllerFactory
 * object which could be a parameter for capability agent construction.
 */
class AlexaLiveViewControllerFactory {
private:
    /**
     * This object contains the interfaces to interact with the AlexaLiveViewController Capability Agent.
     */
    struct LiveViewControllerCapabilityAgentData {
        /// An Interface used to receive Alexa.Camera.LiveViewController directives.
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler;
        /// An interface that provides the configurations of the capabilities being implemented by this capability
        /// agent.
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface> capabilityConfiguration;
        /// An interface used to notify LiveViewController observers.
        std::shared_ptr<alexaLiveViewControllerInterfaces::LiveViewControllerObserverInterface>
            liveViewControllerObserver;
        /// The object responsible for cleaning up this capability agent's objects during shutdown.
        std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown;
    };

public:
    /**
     * Creates a new LiveViewController capability agent
     *
     * @param endpointId An endpoint to which this capability is associated.
     * @param liveViewController An interface that this object will use to perform the live view controller operations.
     * @param messageSender An interface used to send events to AVS.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     *
     * @return A @c LiveViewControllerCapabilityAgentData object if successful, otherwise an @c Optional object with no
     * value.
     */
    static avsCommon::utils::Optional<LiveViewControllerCapabilityAgentData> create(
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
        const std::shared_ptr<alexaLiveViewControllerInterfaces::LiveViewControllerInterface>& liveViewController,
        const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender);
};

}  // namespace alexaLiveViewController
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLER_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLER_ALEXALIVEVIEWCONTROLLERFACTORY_H_
