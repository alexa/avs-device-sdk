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

#ifndef ACSDK_VISUALSTATETRACKERFEATURECLIENT_VISUALSTATETRACKERFEATURECLIENTBUILDER_H_
#define ACSDK_VISUALSTATETRACKERFEATURECLIENT_VISUALSTATETRACKERFEATURECLIENTBUILDER_H_

#include <acsdk/SDKClient/FeatureClientBuilderInterface.h>
#include <acsdk/SDKClient/SDKClientRegistry.h>

#include "VisualStateTrackerFeatureClient.h"

namespace alexaClientSDK {
namespace featureClient {
/**
 * The @c VisualStateTrackerFeatureClient builder class is responsible for creating an instance of the @c
 * VisualStateTrackerFeatureClient and is intended to be used with the @c SDKClientBuilder and @c SDKClientRegistry.
 * It constructs the @c VisualActivityTracker and @c PresentationOrchestratorStateTracker components which are intended
 * for use in tracking the state of visual presentations.
 *
 * It requires the @c EndpointBuilderInterface and @c ContextManagerInterface components from the @c SDKClientRegistry
 * which can be provided by @c DefaultClient
 */
class VisualStateTrackerFeatureClientBuilder : public sdkClient::FeatureClientBuilderInterface {
public:
    /**
     * Create an instance of the VisualStateTrackerFeatureClientBuilder which can be passed to the @c SDKClientBuilder
     * @return The @c VisualStateTrackerFeatureClientBuilder object, or nullptr if creation failed
     */
    static std::unique_ptr<VisualStateTrackerFeatureClientBuilder> create();

    /**
     * Construct an instance of the @c VisualStateTrackerFeatureClient
     * @param sdkClientRegistry The @c SDKClientClientRegistry object
     * @return An instance of the @c VisualStateTrackerFeatureClient, or nullptr if construction failed
     */
    std::shared_ptr<VisualStateTrackerFeatureClient> construct(
        const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry);

    /// @c FeatureClientBuilderInterface functions
    /// @{
    std::string name() override;
    /// @}

private:
    /// Constructor
    VisualStateTrackerFeatureClientBuilder();
};
}  // namespace featureClient
}  // namespace alexaClientSDK

#endif  // ACSDK_VISUALSTATETRACKERFEATURECLIENT_VISUALSTATETRACKERFEATURECLIENTBUILDER_H_
