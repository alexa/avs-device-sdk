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

#ifndef ACSDK_VISUALCHARACTERISTICSFEATURECLIENT_VISUALCHARACTERISTICSFEATURECLIENTBUILDER_H_
#define ACSDK_VISUALCHARACTERISTICSFEATURECLIENT_VISUALCHARACTERISTICSFEATURECLIENTBUILDER_H_

#include <acsdk/SDKClient/FeatureClientBuilderInterface.h>
#include <acsdk/SDKClient/SDKClientRegistry.h>

#include "VisualCharacteristicsFeatureClient.h"

namespace alexaClientSDK {
namespace featureClient {
/**
 * The @c VisualCharacteristicsFeatureClientBuilder builder class is responsible for creating an instance of the @c
 * VisualCharacteristicsFeatureClient and is intended to be used with the @c SDKClientBuilder and @c SDKClientRegistry.
 * It constructs the @c VisualCharacteristics and @c VisualCharacteristicsSerializer components which are intended
 * for use in reporting the visual characteristics for a device.
 *
 * It requires the @c ExceptionEncounteredSenderInterface, @c EndpointBuilderInterface and @c ContextManagerInterface
 * components from the @c SDKClientRegistry which can be provided by @c DefaultClient. If the @c
 * PresentationOrchestratorStateTrackerInterface provided by the @c VisualStateTrackerFeatureClient is available then
 * the @c VisualCharacteristics component will be registered as an observer.
 */
class VisualCharacteristicsFeatureClientBuilder : public sdkClient::FeatureClientBuilderInterface {
public:
    /**
     * Create an instance of the @c VisualCharacteristicsFeatureClientBuilder, this can be passed to the
     * @c SDKClientBuilder
     * @return The newly instantiated @c VisualCharacteristicsFeatureClientBuilder object, or nullptr if creation failed
     */
    static std::unique_ptr<VisualCharacteristicsFeatureClientBuilder> create();

    /**
     * Construct an instance of the @c VisualCharacteristicsFeatureClient
     * @param sdkClientRegistry The @c SDKClientClientRegistry object used for registering components
     * @return An instance of the @c VisualCharacteristicsFeatureClient, or nullptr if construction failed
     */
    std::shared_ptr<VisualCharacteristicsFeatureClient> construct(
        const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry);

    /// @c FeatureClientBuilderInterface functions
    /// @{
    std::string name() override;
    /// @}

private:
    /// Constructor
    VisualCharacteristicsFeatureClientBuilder();
};
}  // namespace featureClient
}  // namespace alexaClientSDK

#endif  // ACSDK_VISUALCHARACTERISTICSFEATURECLIENT_VISUALCHARACTERISTICSFEATURECLIENTBUILDER_H_
