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

#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>

#include "acsdk/PresentationOrchestratorFeatureClient/PresentationOrchestratorFeatureClient.h"
#include "acsdk/PresentationOrchestratorFeatureClient/PresentationOrchestratorFeatureClientBuilder.h"

namespace alexaClientSDK {
namespace featureClient {
/// String to identify log entries originating from this file.
#define TAG "PresentationOrchestratorFeatureClientBuilder"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// String used to identify this feature client builder
static const char PRESENTATION_ORCHESTRATOR_FEATURE_CLIENT_BUILDER[] = "PresentationOrchestratorFeatureClientBuilder";

PresentationOrchestratorFeatureClientBuilder::PresentationOrchestratorFeatureClientBuilder() {
    addRequiredType<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>();
}

std::shared_ptr<PresentationOrchestratorFeatureClient> PresentationOrchestratorFeatureClientBuilder::construct(
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    if (!sdkClientRegistry) {
        ACSDK_CRITICAL(LX("constructFailed").d("reason", "null SDKClientRegistry"));
        return nullptr;
    }

    auto poStateTracker =
        sdkClientRegistry
            ->getComponent<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>();

    return PresentationOrchestratorFeatureClient::create(poStateTracker, sdkClientRegistry);
}

std::unique_ptr<PresentationOrchestratorFeatureClientBuilder> PresentationOrchestratorFeatureClientBuilder::create() {
    return std::unique_ptr<PresentationOrchestratorFeatureClientBuilder>(
        new PresentationOrchestratorFeatureClientBuilder());
}

std::string PresentationOrchestratorFeatureClientBuilder::name() {
    return PRESENTATION_ORCHESTRATOR_FEATURE_CLIENT_BUILDER;
}
}  // namespace featureClient
}  // namespace alexaClientSDK
