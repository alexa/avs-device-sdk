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

#ifndef ACSDK_PRESENTATIONORCHESTRATORFEATURECLIENT_PRESENTATIONORCHESTRATORFEATURECLIENTBUILDER_H_
#define ACSDK_PRESENTATIONORCHESTRATORFEATURECLIENT_PRESENTATIONORCHESTRATORFEATURECLIENTBUILDER_H_

#include <acsdk/SDKClient/FeatureClientBuilderInterface.h>
#include <acsdk/SDKClient/SDKClientRegistry.h>

#include "PresentationOrchestratorFeatureClient.h"

namespace alexaClientSDK {
namespace featureClient {
/**
 * The @c PresentationOrchestratorFeatureClientBuilder builder class is responsible for creating an instance of the @c
 * PresentationOrchestratorFeatureClient and is intended to be used with the @c SDKClientBuilder and @c
 * SDKClientRegistry. It constructs the @c PresentationOrchestrator, @c PresentationOrchestratorClient and @c
 * VisualTimeoutManager components which are intended for use in managing the lifecycle of presentations across windows
 * and tracking their state.
 *
 * It requires the @c PresentationOrchestratorStateTrackerInterface component from the @c SDKClientRegistry which can be
 * provided by @c VisualStateTrackerFeatureClient
 */
class PresentationOrchestratorFeatureClientBuilder : public sdkClient::FeatureClientBuilderInterface {
public:
    /**
     * Create an instance of the @c VisualCharacteristicsFeatureClientBuilder, this can be passed to the
     * @c SDKClientBuilder
     * @return The newly instantiated @c VisualCharacteristicsFeatureClientBuilder object, or nullptr if creation failed
     */
    static std::unique_ptr<PresentationOrchestratorFeatureClientBuilder> create();

    /**
     * Construct an instance of the @c PresentationOrchestratorFeatureClient
     * @param sdkClientRegistry The @c SDKClientClientRegistry object
     * @return An instance of the @c PresentationOrchestratorFeatureClient, or nullptr if construction failed
     */
    std::shared_ptr<PresentationOrchestratorFeatureClient> construct(
        const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry);

    /// @c FeatureClientBuilderInterface functions
    /// @{
    std::string name() override;
    /// @}

private:
    /// Constructor
    PresentationOrchestratorFeatureClientBuilder();
};
}  // namespace featureClient
}  // namespace alexaClientSDK

#endif  // ACSDK_PRESENTATIONORCHESTRATORFEATURECLIENT_PRESENTATIONORCHESTRATORFEATURECLIENTBUILDER_H_
