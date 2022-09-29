/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     https://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdk/AlexaPresentationAPL/private/AlexaPresentationAPL.h"
#include "acsdk/AlexaPresentationAPL/AlexaPresentationAPLFactory.h"

namespace alexaClientSDK {
namespace aplCapabilityAgent {
/// String to identify log entries originating from this file.
#define TAG "AlexaPresentationAPLFactory"
/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

avsCommon::utils::Optional<AlexaPresentationAPLFactory::AlexaPresentationAPLAgentData> AlexaPresentationAPLFactory::
    create(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::string APLVersion,
        std::shared_ptr<alexaClientSDK::aplCapabilityCommonInterfaces::VisualStateProviderInterface>
            visualStateProvider) {
    auto alexaPresentationAPL = AlexaPresentationAPL::create(
        exceptionSender, metricRecorder, messageSender, contextManager, APLVersion, visualStateProvider);

    if (alexaPresentationAPL) {
        AlexaPresentationAPLFactory::AlexaPresentationAPLAgentData alexaPresentationAPLData;
        alexaPresentationAPLData.aplCapabilityAgent = alexaPresentationAPL;
        alexaPresentationAPLData.capabilityAgent = alexaPresentationAPL;
        alexaPresentationAPLData.capabilityAgentNotifier = alexaPresentationAPL;
        alexaPresentationAPLData.capabilityConfiguration = alexaPresentationAPL;
        alexaPresentationAPLData.requiresShutdown = alexaPresentationAPL;
        return avsCommon::utils::Optional<AlexaPresentationAPLFactory::AlexaPresentationAPLAgentData>(
            alexaPresentationAPLData);
    }
    return avsCommon::utils::Optional<AlexaPresentationAPLFactory::AlexaPresentationAPLAgentData>();
}
}  // namespace aplCapabilityAgent
}  // namespace alexaClientSDK