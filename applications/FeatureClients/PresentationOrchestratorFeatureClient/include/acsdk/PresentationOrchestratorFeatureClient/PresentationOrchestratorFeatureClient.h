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

#ifndef ACSDK_PRESENTATIONORCHESTRATORFEATURECLIENT_PRESENTATIONORCHESTRATORFEATURECLIENT_H_
#define ACSDK_PRESENTATIONORCHESTRATORFEATURECLIENT_PRESENTATIONORCHESTRATORFEATURECLIENT_H_

#include <acsdk/PresentationOrchestratorInterfaces/VisualTimeoutManagerInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorClientInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateTrackerInterface.h>
#include <acsdk/SDKClient/FeatureClientInterface.h>
#include <acsdk/SDKClient/SDKClientRegistry.h>
#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsSerializerInterface.h>
#include <acsdkShutdownManagerInterfaces/ShutdownManagerInterface.h>

namespace alexaClientSDK {
namespace featureClient {
/**
 * The @c PresentationOrchestratorFeatureClient is a Feature Client that provides functionality to manage and track the
 * lifecycle of presentations across multiple windows through the use of the @c PresentationOrchestrator, @c
 * PresentationOrchestratorClient and @c VisualTimeoutManager components. It is recommended that the @c
 * PresentationOrchestratorFeatureClientBuilder is used in combination with the
 * @c SDKClientBuilder to construct this Feature Client.
 */
class PresentationOrchestratorFeatureClient : public sdkClient::FeatureClientInterface {
public:
    /**
     * Create an instance of the @c PresentationOrchestratorFeatureClient, it is recommended that the @c
     * PresentationOrchestratorFeatureClientBuilder is used to create and register an instance of this client with the
     * SDKClientRegistry
     * @param presentationOrchestratorStateTracker Reference to the @c PresentationOrchestratorStateTrackerInterface
     * @param sdkClientRegistry  Reference to the @c SDKClientRegistry, used to register components
     * @return Pointer to the newly created @c PresentationOrchestratorFeatureClient, or nullptr on failure
     */
    static std::unique_ptr<PresentationOrchestratorFeatureClient> create(
        const std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>&
            presentationOrchestratorStateTracker,
        const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry);

    /// @c FeatureClientInterface functions
    /// @{
    bool configure(const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) override;
    void doShutdown() override;
    /// @}

    /**
     * Get a reference to @c PresentationOrchestratorClientInterface
     *
     * @return shared_ptr to @c PresentationOrchestratorClientInterface
     */
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorClientInterface>
    getPresentationOrchestratorClient() const;

    /**
     * Get a reference to @c PresentationOrchestratorInterface
     * @return shared_ptr to @c PresentationOrchestratorInterface
     */
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorInterface> getPresentationOrchestrator()
        const;

    /**
     * Get a reference to @c VisualTimeoutManagerInterface
     * @return shared_ptr to @c VisualTimeoutManagerInterface
     */
    std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface> getVisualTimeoutManager() const;

    /// Destructor
    ~PresentationOrchestratorFeatureClient();

private:
    /**
     * Constructor
     */
    PresentationOrchestratorFeatureClient(
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorInterface> presentationOrchestrator,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorClientInterface>
            presentationOrchestratorClient,
        std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface> visualTimeoutManager,
        std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> shutdownManager);

    /// The Presentation Orchestrator interface
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorInterface> m_presentationOrchestrator;

    /// The Presentation Orchestrator client interface
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorClientInterface>
        m_presentationOrchestratorClient;

    /// The Visual Timeout Manager interface
    std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface> m_visualTimeoutManager;

    /// The shutdown manager
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> m_shutdownManager;
};
}  // namespace featureClient
}  // namespace alexaClientSDK
#endif  // ACSDK_PRESENTATIONORCHESTRATORFEATURECLIENT_PRESENTATIONORCHESTRATORFEATURECLIENT_H_
