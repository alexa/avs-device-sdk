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

#ifndef ACSDK_ALEXAPRESENTATIONFEATURECLIENT_ALEXAPRESENTATIONFEATURECLIENTBUILDER_H_
#define ACSDK_ALEXAPRESENTATIONFEATURECLIENT_ALEXAPRESENTATIONFEATURECLIENTBUILDER_H_

#include <acsdk/APLCapabilityCommonInterfaces/VisualStateProviderInterface.h>
#include <acsdk/SDKClient/FeatureClientBuilderInterface.h>
#include <acsdk/SDKClient/SDKClientRegistry.h>

#include "AlexaPresentationFeatureClient.h"

namespace alexaClientSDK {
namespace featureClient {
/**
 * The @c AlexaPresentationFeatureClientBuilder builder class is responsible for creating an instance of the @c
 * AlexaPresentationFeatureClient and is intended to be used with the @c SDKClientBuilder and @c SDKClientRegistry. It
 * constructs the @c AlexaPresentation and @c AlexaPresentationAPL capabilities.
 *
 * It requires the @c ExceptionEncounteredSenderInterface, @c AVSConnectionManagerInterface, @c
 * EndpointBuilderInterface, @c MetricRecorderInterface and @c ContextManagerInterface components from the @c
 * SDKClientRegistry which can be provided by @c DefaultClient
 */
class AlexaPresentationFeatureClientBuilder : public sdkClient::FeatureClientBuilderInterface {
public:
    /**
     * Create an instance of the @c AlexaPresentationFeatureClientBuilder
     * @param aplVersion The APL Version supported by the device
     * @param stateProviderInterface Pointer to the visual state provider
     * @return Pointer to the feature client builder, or nullptr if creation failed
     */
    static std::unique_ptr<AlexaPresentationFeatureClientBuilder> create(
        std::string aplVersion,
        std::shared_ptr<aplCapabilityCommonInterfaces::VisualStateProviderInterface> stateProviderInterface);

    /**
     * Construct an instance of the @c AlexaPresentationFeatureClient
     * @param sdkClientRegistry The @c SDKClientRegistry object
     * @return An instance of the @c AlexaPresentationFeatureClient, or nullptr if construction failed
     */
    std::shared_ptr<AlexaPresentationFeatureClient> construct(
        const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry);

    /// @c FeatureClientBuilderInterface functions
    /// @{
    std::string name() override;
    /// @}

private:
    /**
     * Constructor
     * @param aplVersion The APL Version supported by the device
     * @param stateProviderInterface Pointer to the visual state provider
     */
    AlexaPresentationFeatureClientBuilder(
        std::string aplVersion,
        std::shared_ptr<aplCapabilityCommonInterfaces::VisualStateProviderInterface> stateProviderInterface);

    /// The APL Version supported by the device
    std::string m_aplVersion;

    /// Reference to the visual state provider interface
    std::shared_ptr<aplCapabilityCommonInterfaces::VisualStateProviderInterface> m_stateProviderInterface;
};
}  // namespace featureClient
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAPRESENTATIONFEATURECLIENT_ALEXAPRESENTATIONFEATURECLIENTBUILDER_H_
