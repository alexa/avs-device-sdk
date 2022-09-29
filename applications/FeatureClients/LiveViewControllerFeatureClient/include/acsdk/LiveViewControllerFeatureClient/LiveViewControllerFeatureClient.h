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

#ifndef ACSDK_LIVEVIEWCONTROLLERFEATURECLIENT_LIVEVIEWCONTROLLERFEATURECLIENT_H_
#define ACSDK_LIVEVIEWCONTROLLERFEATURECLIENT_LIVEVIEWCONTROLLERFEATURECLIENT_H_

#include <memory>

#include <acsdk/AlexaLiveViewControllerInterfaces/LiveViewControllerInterface.h>
#include <acsdk/SDKClient/FeatureClientInterface.h>
#include <acsdk/SDKClient/SDKClientRegistry.h>
#include <acsdkShutdownManagerInterfaces/ShutdownManagerInterface.h>
#include <Alexa/AlexaInterfaceMessageSender.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>

namespace alexaClientSDK {
namespace featureClient {
/**
 * The @c LiveViewControllerFeatureClient is a Feature Client that adds support for LiveViewController directives
 * through the use of @c Alexa.Camera.LiveViewController components. It is recommended that the @c
 * LiveViewControllerFeatureClientBuilder is used in combination with the
 * @c SDKClientBuilder to construct this Feature Client.
 */
class LiveViewControllerFeatureClient : public sdkClient::FeatureClientInterface {
public:
    /**
     * Create the LiveViewControllerFeatureClient, this client must be provided to the @c ClientBuilder or added to an
     * existing @c SDKClientRegistry.
     * @param endpointId An endpoint to which this capability is associated.
     * @param liveViewController Pointer to the @c LiveViewControllerInterface.
     * @param connectionManager Pointer to the @c AVSConnectionManagerInterface.
     * @param contextManager Pointer to the @c ContextManagerInterface.
     * @param responseSender Pointer to the @c AlexaInterfaceMessageSender.
     * @param exceptionSender Pointer to the @c ExceptionEncounteredSenderInterface.
     * @param endpointBuilder Pointer to the @c EndpointBuilderInterface
     * @return pointer to the LiveViewControllerFeatureClient, or nullptr if creation failed.
     */
    static std::unique_ptr<LiveViewControllerFeatureClient> create(
        avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
        const std::shared_ptr<alexaClientSDK::alexaLiveViewControllerInterfaces::LiveViewControllerInterface>&
            liveViewController,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AVSConnectionManagerInterface>&
            connectionManager,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<alexaClientSDK::capabilityAgents::alexa::AlexaInterfaceMessageSender>& responseSender,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
            exceptionSender,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>&
            endpointBuilder);

    /// @c FeatureClientInterface functions
    /// @{
    bool configure(const std::shared_ptr<alexaClientSDK::sdkClient::SDKClientRegistry>& sdkClientRegistry) override;
    void doShutdown() override;
    /// @}

    /// Destructor
    ~LiveViewControllerFeatureClient();

private:
    /**
     * Constructor
     * @param shutdownManager Pointer to the @c ShutdownManagerInterface.
     */
    LiveViewControllerFeatureClient(
        std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> shutdownManager);

    /// The shutdown manager
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> m_shutdownManager;
};
}  // namespace featureClient
}  // namespace alexaClientSDK
#endif  // ACSDK_LIVEVIEWCONTROLLERFEATURECLIENT_LIVEVIEWCONTROLLERFEATURECLIENT_H_
