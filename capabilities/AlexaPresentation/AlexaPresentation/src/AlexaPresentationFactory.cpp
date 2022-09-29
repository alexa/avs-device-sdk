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

#include <ostream>

#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdk/AlexaPresentation/private/AlexaPresentation.h"
#include "acsdk/AlexaPresentation/AlexaPresentationFactory.h"

namespace alexaClientSDK {
namespace alexaPresentation {
/// String to identify log entries originating from this file.
#define TAG "AlexaPresentationFactory"
/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

avsCommon::utils::Optional<AlexaPresentationFactory::AlexaPresentationAgentData> AlexaPresentationFactory::create(
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) {
    auto alexaPresentation = AlexaPresentation::create(exceptionSender, messageSender, contextManager);

    if (alexaPresentation) {
        AlexaPresentationFactory::AlexaPresentationAgentData alexaPresentationData;
        alexaPresentationData.alexaPresentation = alexaPresentation;
        alexaPresentationData.requiresShutdown = alexaPresentation;
        alexaPresentationData.capabilityConfiguration = alexaPresentation;
        return avsCommon::utils::Optional<AlexaPresentationFactory::AlexaPresentationAgentData>(alexaPresentationData);
    }
    return avsCommon::utils::Optional<AlexaPresentationFactory::AlexaPresentationAgentData>();
}
}  // namespace alexaPresentation
}  // namespace alexaClientSDK