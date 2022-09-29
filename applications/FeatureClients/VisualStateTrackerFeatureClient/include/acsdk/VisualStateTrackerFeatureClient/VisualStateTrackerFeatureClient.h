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

#ifndef ACSDK_VISUALSTATETRACKERFEATURECLIENT_VISUALSTATETRACKERFEATURECLIENT_H_
#define ACSDK_VISUALSTATETRACKERFEATURECLIENT_VISUALSTATETRACKERFEATURECLIENT_H_

#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateTrackerInterface.h>
#include <acsdk/SDKClient/FeatureClientInterface.h>
#include <acsdk/SDKClient/SDKClientRegistry.h>
#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsSerializerInterface.h>
#include <acsdkShutdownManagerInterfaces/ShutdownManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>

namespace alexaClientSDK {
namespace featureClient {
/**
 * The @c VisualStateTrackerFeatureClient is a Feature Client that provides visual state tracking functionality through
 * the use of the @c VisualActivityTracker and @c PresentationOrchestratorStateTracker components. It is recommended
 * that the @c VisualStateTrackerFeatureClientBuilder is used in combination with the @c SDKClientBuilder to construct
 * this Feature Client.
 */
class VisualStateTrackerFeatureClient : public sdkClient::FeatureClientInterface {
public:
    /**
     * Create an instance of the VisualStateTrackerFeatureClient, it is recommended that the @c
     * VisualStateTrackerFeatureClientBuilder is used to create and register an instance of this client with the
     * @c SDKClientBuilder or @c SDKClientRegistry
     * @param contextManager Pointer to the @c ContextManagerInterface to allow capabilities to receive context
     * @param endpointBuilder Pointer to the @c EndpointBuilderInterface for registering capabilities
     * @param sdkClientRegistry Reference to the @c SDKClientRegistry used to register
     * @return unique pointer to the created VisualStateTrackerFeatureClient, or nullptr if creation failed
     */
    static std::unique_ptr<VisualStateTrackerFeatureClient> create(
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>&
            endpointBuilder,
        const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry);

    /// @c FeatureClientInterface functions
    /// @{
    bool configure(const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) override;
    void doShutdown() override;
    /// @}

    /**
     * Get a reference to @c PresentationOrchestratorStateTrackerInterface
     *
     * @return shared_ptr to @c PresentationOrchestratorStateTrackerInterface
     */
    std::shared_ptr<alexaClientSDK::presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>
    getPresentationOrchestratorStateTracker() const;

    /// Destructor
    ~VisualStateTrackerFeatureClient();

private:
    /**
     * Constructor
     * @param presentationOrchestratorStateTracker Pointer to the @c PresentationOrchestratorStateTrackerInterface
     * @param shutdownManager Pointer the the @c ShutdownManagerInterface, used to shutdown all components
     */
    VisualStateTrackerFeatureClient(
        std::shared_ptr<
            alexaClientSDK::presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>
            presentationOrchestratorStateTracker,
        std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> shutdownManager);

    /// The Presentation Orchestrator state tracker
    std::shared_ptr<alexaClientSDK::presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>
        m_presentationOrchestratorStateTracker;

    /// The shutdown manager
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> m_shutdownManager;
};
}  // namespace featureClient
}  // namespace alexaClientSDK

#endif  // ACSDK_VISUALSTATETRACKERFEATURECLIENT_VISUALSTATETRACKERFEATURECLIENT_H_
