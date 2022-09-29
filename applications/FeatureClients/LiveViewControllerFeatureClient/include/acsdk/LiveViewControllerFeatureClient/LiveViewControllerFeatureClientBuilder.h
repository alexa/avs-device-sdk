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

#ifndef ACSDK_LIVEVIEWCONTROLLERFEATURECLIENT_LIVEVIEWCONTROLLERFEATURECLIENTBUILDER_H_
#define ACSDK_LIVEVIEWCONTROLLERFEATURECLIENT_LIVEVIEWCONTROLLERFEATURECLIENTBUILDER_H_

#include "LiveViewControllerFeatureClient.h"

#include <acsdk/AlexaLiveViewControllerInterfaces/LiveViewControllerInterface.h>
#include <acsdk/SDKClient/FeatureClientBuilderInterface.h>
#include <acsdk/SDKClient/SDKClientRegistry.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>

namespace alexaClientSDK {
namespace featureClient {
/**
 * The @c LiveViewControllerFeatureClientBuilder builder class is responsible for creating an instance of the @c
 * LiveViewControllerFeatureClient and is intended to be used with the @c SDKClientBuilder and @c SDKClientRegistry. It
 * constructs the @c LiveViewController capability.
 *
 * It requires the @c ExceptionEncounteredSenderInterface, @c AVSConnectionManagerInterface, @c
 * EndpointBuilderInterface,
 * @c AlexaInterfaceMessageSender and @c ContextManagerInterface components from the @c
 * SDKClientRegistry which can be provided by @c DefaultClient.
 */
class LiveViewControllerFeatureClientBuilder : public sdkClient::FeatureClientBuilderInterface {
public:
    /**
     * Create an instance of the @c LiveViewControllerFeatureClientBuilder
     *
     * @param endpointId An endpoint to which this capability is associated.
     * @param liveViewController An instance of @c LiveViewControllerInterface implementing the LVC CA Interface to
     * communicate with it.
     *
     * @return Pointer to the feature client builder, or nullptr if creation failed.
     */
    static std::unique_ptr<LiveViewControllerFeatureClientBuilder> create(
        avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
        const std::shared_ptr<alexaClientSDK::alexaLiveViewControllerInterfaces::LiveViewControllerInterface>&
            liveViewController);

    /**
     * Construct an instance of the @c LiveViewControllerFeatureClient.
     * @param sdkClientRegistry The @c SDKClientRegistry object.
     * @return An instance of the @c LiveViewControllerFeatureClient, or nullptr if construction failed.
     */
    std::shared_ptr<LiveViewControllerFeatureClient> construct(
        const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry);

    /// @c FeatureClientBuilderInterface functions
    /// @{
    std::string name() override;
    /// @}

private:
    /**
     * Constructor
     *
     * @param endpointId An endpoint to which this capability is associated.
     * @param liveViewController An instance of @c LiveViewControllerInterface implementing the LVC CA Interface to
     * communicate with it.
     */
    LiveViewControllerFeatureClientBuilder(
        avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
        const std::shared_ptr<alexaClientSDK::alexaLiveViewControllerInterfaces::LiveViewControllerInterface>&
            liveViewController);

    /// The endpoint id.
    avsCommon::sdkInterfaces::endpoints::EndpointIdentifier m_endpointId;

    /// Reference to the @c LiveViewControllerInterface.
    std::shared_ptr<alexaClientSDK::alexaLiveViewControllerInterfaces::LiveViewControllerInterface>
        m_liveViewController;
};
}  // namespace featureClient
}  // namespace alexaClientSDK

#endif  // ACSDK_LIVEVIEWCONTROLLERFEATURECLIENT_LIVEVIEWCONTROLLERFEATURECLIENTBUILDER_H_
