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
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

#include "acsdk/AlexaPresentationFeatureClient/AlexaPresentationFeatureClient.h"
#include "acsdk/AlexaPresentationFeatureClient/AlexaPresentationFeatureClientBuilder.h"

namespace alexaClientSDK {
namespace featureClient {
/// String to identify log entries originating from this file.
#define TAG "AlexaPresentationFeatureClientBuilder"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Name to identify this feature client builder
static const char ALEXA_PRESENTATION_FEATURE_CLIENT_BUILDER[] = "AlexaPresentationFeatureClientBuilder";

AlexaPresentationFeatureClientBuilder::AlexaPresentationFeatureClientBuilder(
    std::string aplVersion,
    std::shared_ptr<aplCapabilityCommonInterfaces::VisualStateProviderInterface> stateProviderInterface) :
        m_aplVersion(std::move(aplVersion)),
        m_stateProviderInterface(std::move(stateProviderInterface)) {
    addRequiredType<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>();
    addRequiredType<alexaClientSDK::avsCommon::sdkInterfaces::AVSConnectionManagerInterface>();
    addRequiredType<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>();
    addRequiredType<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>();
    addRequiredType<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface>();
}

std::shared_ptr<AlexaPresentationFeatureClient> AlexaPresentationFeatureClientBuilder::construct(
    const std::shared_ptr<sdkClient::SDKClientRegistry>& sdkClientRegistry) {
    if (!sdkClientRegistry) {
        ACSDK_CRITICAL(LX("constructFailed").d("reason", "null SDKClientRegistry"));
        return nullptr;
    }

    auto exceptionSender =
        sdkClientRegistry
            ->getComponent<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>();
    auto connectionManager =
        sdkClientRegistry->getComponent<alexaClientSDK::avsCommon::sdkInterfaces::AVSConnectionManagerInterface>();
    auto contextManager =
        sdkClientRegistry->getComponent<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>();
    auto endpointBuilder =
        sdkClientRegistry
            ->getComponent<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>();
    auto metricRecorder =
        sdkClientRegistry->getComponent<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface>();

    return AlexaPresentationFeatureClient::create(
        m_aplVersion,
        m_stateProviderInterface,
        exceptionSender,
        connectionManager,
        contextManager,
        endpointBuilder,
        metricRecorder,
        sdkClientRegistry);
}

std::unique_ptr<AlexaPresentationFeatureClientBuilder> AlexaPresentationFeatureClientBuilder::create(
    std::string aplVersion,
    std::shared_ptr<aplCapabilityCommonInterfaces::VisualStateProviderInterface> stateProviderInterface) {
    return std::unique_ptr<AlexaPresentationFeatureClientBuilder>(
        new AlexaPresentationFeatureClientBuilder(std::move(aplVersion), std::move(stateProviderInterface)));
}

std::string AlexaPresentationFeatureClientBuilder::name() {
    return ALEXA_PRESENTATION_FEATURE_CLIENT_BUILDER;
}
}  // namespace featureClient
}  // namespace alexaClientSDK