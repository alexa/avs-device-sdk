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

#include "acsdk/VisualCharacteristicsFeatureClient/VisualCharacteristicsFeatureClient.h"
#include "acsdk/VisualCharacteristicsFeatureClient/VisualCharacteristicsFeatureClientBuilder.h"

namespace alexaClientSDK {
namespace featureClient {
/// String to identify log entries originating from this file.
#define TAG "VisualCharacteristicsFeatureClientBuilder"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// String used to identify this feature client builder
static const char VISUAL_CHARACTERISTICS_FEATURE_CLIENT_BUILDER[] = "VisualCharacteristicsFeatureClientBuilder";

VisualCharacteristicsFeatureClientBuilder::VisualCharacteristicsFeatureClientBuilder() {
    addRequiredType<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>();
    addRequiredType<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>();
    addRequiredType<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>();
}

std::shared_ptr<VisualCharacteristicsFeatureClient> VisualCharacteristicsFeatureClientBuilder::construct(
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    if (!sdkClientRegistry) {
        ACSDK_CRITICAL(LX("constructFailed").d("reason", "null SDKClientRegistry"));
        return nullptr;
    }

    auto exceptionSender =
        sdkClientRegistry
            ->getComponent<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>();
    auto contextManager =
        sdkClientRegistry->getComponent<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>();
    auto endpointBuilder =
        sdkClientRegistry
            ->getComponent<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>();

    return VisualCharacteristicsFeatureClient::create(
        exceptionSender, contextManager, endpointBuilder, sdkClientRegistry);
}

std::unique_ptr<VisualCharacteristicsFeatureClientBuilder> VisualCharacteristicsFeatureClientBuilder::create() {
    return std::unique_ptr<VisualCharacteristicsFeatureClientBuilder>(new VisualCharacteristicsFeatureClientBuilder());
}

std::string VisualCharacteristicsFeatureClientBuilder::name() {
    return VISUAL_CHARACTERISTICS_FEATURE_CLIENT_BUILDER;
}
}  // namespace featureClient
}  // namespace alexaClientSDK
