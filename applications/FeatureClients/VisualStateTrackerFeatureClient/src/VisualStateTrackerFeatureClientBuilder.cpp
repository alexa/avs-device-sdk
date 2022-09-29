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

#include "acsdk/VisualStateTrackerFeatureClient/VisualStateTrackerFeatureClientBuilder.h"

#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>

#include "acsdk/VisualStateTrackerFeatureClient/VisualStateTrackerFeatureClient.h"

namespace alexaClientSDK {
namespace featureClient {
/// String to identify log entries originating from this file.
#define TAG "VisualStateTrackerFeatureClientBuilder"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// String used to identify this feature client builder
static const char VISUAL_STATE_TRACKER_FEATURE_CLIENT_BUILDER[] = "VisualStateTrackerFeatureClientBuilder";

VisualStateTrackerFeatureClientBuilder::VisualStateTrackerFeatureClientBuilder() {
    addRequiredType<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>();
    addRequiredType<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>();
}

std::shared_ptr<VisualStateTrackerFeatureClient> VisualStateTrackerFeatureClientBuilder::construct(
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    if (!sdkClientRegistry) {
        ACSDK_CRITICAL(LX("constructFailed").d("reason", "null SDKClientRegistry"));
        return nullptr;
    }

    auto contextManager =
        sdkClientRegistry->getComponent<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>();
    auto defaultEndpointBuilder =
        sdkClientRegistry
            ->getComponent<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>();

    return VisualStateTrackerFeatureClient::create(contextManager, defaultEndpointBuilder, sdkClientRegistry);
}

std::unique_ptr<VisualStateTrackerFeatureClientBuilder> VisualStateTrackerFeatureClientBuilder::create() {
    return std::unique_ptr<VisualStateTrackerFeatureClientBuilder>(new VisualStateTrackerFeatureClientBuilder());
}

std::string VisualStateTrackerFeatureClientBuilder::name() {
    return VISUAL_STATE_TRACKER_FEATURE_CLIENT_BUILDER;
}
}  // namespace featureClient
}  // namespace alexaClientSDK
