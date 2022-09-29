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

#ifndef ACSDK_ALEXAPRESENTATIONFEATURECLIENT_ALEXAPRESENTATIONFEATURECLIENT_H_
#define ACSDK_ALEXAPRESENTATIONFEATURECLIENT_ALEXAPRESENTATIONFEATURECLIENT_H_

#include <memory>

#include <acsdk/AlexaPresentationInterfaces/AlexaPresentationCapabilityAgentInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentNotifierInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentObserverInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/VisualStateProviderInterface.h>
#include <acsdk/SDKClient/FeatureClientInterface.h>
#include <acsdk/SDKClient/SDKClientRegistry.h>
#include <acsdkShutdownManagerInterfaces/ShutdownManagerInterface.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

namespace alexaClientSDK {
namespace featureClient {
/**
 * The @c AlexaPresentationFeatureClient is a Feature Client that adds support for APL directives through the use of @c
 * Alexa.Presentation, @c Alexa.Presentation.APL components. It is recommended that the @c
 * AlexaPresentationFeatureClientBuilder is used in combination with the
 * @c SDKClientBuilder to construct this Feature Client.
 */
class AlexaPresentationFeatureClient : public sdkClient::FeatureClientInterface {
public:
    /**
     * Create the AlexaPresentationFeatureClient, this client must be provided to the @c ClientBuilder or added to an
     * existing @c SDKClientRegistry
     * @param aplVersion The max supported APL Version
     * @param stateProviderInterface The visual state provider interface
     * @param exceptionSender Pointer to the @c ExceptionEncounteredSenderInterface
     * @param connectionManager Pointer to the @c AVSConnectionManagerInterface
     * @param contextManager Pointer to the @c ContextManagerInterface
     * @param endpointBuilder Pointer to the @c EndpointBuilderInterface
     * @param metricRecorder @c The MetricRecorderInterface - may be null if metrics are not required
     * @param sdkClientRegistry @c Pointer to the @c SDKClientRegistry - may be null if not using the SDKClientRegistry
     * @return pointer to the AlexaPresentationFeatureClient, or nullptr if creation failed
     */
    static std::unique_ptr<AlexaPresentationFeatureClient> create(
        std::string aplVersion,
        std::shared_ptr<aplCapabilityCommonInterfaces::VisualStateProviderInterface> stateProviderInterface,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
            exceptionSender,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AVSConnectionManagerInterface>&
            connectionManager,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>&
            endpointBuilder,
        const std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
        const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry);

    /// @c FeatureClientInterface functions
    /// @{
    bool configure(const std::shared_ptr<alexaClientSDK::sdkClient::SDKClientRegistry>& sdkClientRegistry) override;
    void doShutdown() override;
    /// @}

    /**
     * Get a reference to the @c APLCapabilityAgentInterface which is used to communicate with the APL capability agent.
     *
     * @return shared_ptr to the @c APLCapabilityAgentInterface
     */
    std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> getAPLCapabilityAgent() const;

    /**
     * Get a reference to the @c AlexaPresentationCapabilityAgentInterface which is used to communicate with the
     * Alexa.Presentation capability agent.
     *
     * @return shared_ptr to the @c AlexaPresentationCapabilityAgentInterface
     */
    std::shared_ptr<alexaPresentationInterfaces::AlexaPresentationCapabilityAgentInterface>
    getAlexaPresentationCapabilityAgent() const;

    /**
     * Adds an observer to be notified of APL capability agent changes.
     *
     * @param observer The @c APLCapabilityAgentObserverInterface to add; duplicate or null pointers will be ignored.
     */
    void addAPLCapabilityAgentObserver(
        std::weak_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentObserverInterface> observer);

    /**
     * Removes an observer to be notified of APL capability agent changes.
     *
     * @param observer The @c APLCapabilityAgentObserverInterface to remove, no action will be taken if observer does
     * not exist.
     */
    void removeAPLCapabilityAgentObserver(
        std::weak_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentObserverInterface> observer);

    /// Destructor
    ~AlexaPresentationFeatureClient();

private:
    /**
     * Constructor
     * @param alexaPresentationCA Pointer to the @c AlexaPresentationCapabilityAgentInterface
     * @param aplCapabilityAgent Pointer to the @c APLCapabilityAgentInterface
     * @param aplCapabilityAgentNotifier Pointer to the @c APLCapabilityAgentNotifierInterface
     * @param shutdownManager Pointer to the @c ShutdownManagerInterface
     */
    AlexaPresentationFeatureClient(
        std::shared_ptr<alexaPresentationInterfaces::AlexaPresentationCapabilityAgentInterface> alexaPresentationCA,
        std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> aplCapabilityAgent,
        std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentNotifierInterface> aplCapabilityAgentNotifier,
        std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> shutdownManager);

    /// The Alexa Presentation CA interface
    std::shared_ptr<alexaPresentationInterfaces::AlexaPresentationCapabilityAgentInterface> m_alexaPresentationCA;

    /// The Alexa Presentation APL CA interface
    std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> m_aplCapabilityAgent;

    /// An Interface used to register observers for the AlexaPresentationAPL capability agent.
    std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentNotifierInterface> m_aplCapabilityAgentNotifier;

    /// The shutdown manager
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> m_shutdownManager;
};
}  // namespace featureClient
}  // namespace alexaClientSDK
#endif  // ACSDK_ALEXAPRESENTATIONFEATURECLIENT_ALEXAPRESENTATIONFEATURECLIENT_H_
