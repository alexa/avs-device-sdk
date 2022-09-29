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

#ifndef ACSDK_VISUALCHARACTERISTICSFEATURECLIENT_VISUALCHARACTERISTICSFEATURECLIENT_H_
#define ACSDK_VISUALCHARACTERISTICSFEATURECLIENT_VISUALCHARACTERISTICSFEATURECLIENT_H_

#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateObserverInterface.h>
#include <acsdk/SDKClient/FeatureClientInterface.h>
#include <acsdk/SDKClient/SDKClientRegistry.h>
#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsInterface.h>
#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsSerializerInterface.h>
#include <acsdkShutdownManagerInterfaces/ShutdownManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>

namespace alexaClientSDK {
namespace featureClient {
/**
 * The @c VisualCharacteristicsFeatureClient is a Feature Client that provides functionality to report the visual
 * characteristics of a device through the use of the @c VisualCharacteristics and @c VisualCharacteristicsSerializer
 * components. It is recommended that the @c VisualCharacteristicsFeatureClientBuilder is used in combination with the
 * @c SDKClientBuilder to construct this Feature Client.
 */
class VisualCharacteristicsFeatureClient : public sdkClient::FeatureClientInterface {
public:
    /**
     * Create an instance of the VisualCharacteristicsFeatureClient, users should prefer to use the
     * VisualCharacteristicsFeatureClientBuilder to create and register an instance of this client with the SDKClient
     * @param exceptionSender Reference to the @c ExceptionEncounteredSenderInterface
     * @param contextManager Reference to the @c ContextManagerInterface
     * @param endpointBuilder Reference to the @c EndpointBuilderInterface
     * @return unique pointer to the created VisualCharacteristicsFeatureClient, or nullptr if creation failed
     */
    static std::unique_ptr<VisualCharacteristicsFeatureClient> create(
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
            exceptionSender,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>&
            endpointBuilder,
        const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry);

    /**
     * Get a reference to @c VisualCharacteristicsInterface
     *
     * @return shared_ptr to  @c VisualCharacteristicsInterface
     */
    std::shared_ptr<alexaClientSDK::visualCharacteristicsInterfaces::VisualCharacteristicsInterface>
    getVisualCharacteristics() const;

    /**
     * Get a reference to @c VisualCharacteristicsSerializerInterface
     *
     * @return shared_ptr to  @c VisualCharacteristicsSerializerInterface
     */
    std::shared_ptr<alexaClientSDK::visualCharacteristicsInterfaces::VisualCharacteristicsSerializerInterface>
    getVisualCharacteristicsSerializer() const;

    /// @c FeatureClientInterface functions
    /// @{
    bool configure(const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) override;
    void doShutdown() override;
    /// @}

    /// Destructor
    ~VisualCharacteristicsFeatureClient();

private:
    /**
     * Constructor
     */
    VisualCharacteristicsFeatureClient(
        std::shared_ptr<visualCharacteristicsInterfaces::VisualCharacteristicsInterface> visualCharacteristics,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateObserverInterface>
            visualCharacteristicsPOStateObserver,
        std::shared_ptr<visualCharacteristicsInterfaces::VisualCharacteristicsSerializerInterface>
            visualCharacteristicsSerializer,
        std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> shutdownManager);

    /// The VisualCharacteristics capability agent.
    std::shared_ptr<visualCharacteristicsInterfaces::VisualCharacteristicsInterface> m_visualCharacteristics;

    /// The state observer interface exposed by visual characteristics
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateObserverInterface>
        m_visualCharacteristicsPOStateObserver;

    /// The VisualCharacteristics serializer.
    std::shared_ptr<visualCharacteristicsInterfaces::VisualCharacteristicsSerializerInterface>
        m_visualCharacteristicsSerializer;

    /// The shutdown manager
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> m_shutdownManager;
};
}  // namespace featureClient
}  // namespace alexaClientSDK
#endif  // ACSDK_VISUALCHARACTERISTICSFEATURECLIENT_VISUALCHARACTERISTICSFEATURECLIENT_H_
