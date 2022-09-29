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

#ifndef ACSDK_ALEXAPRESENTATIONAPL_ALEXAPRESENTATIONAPLFACTORY_H_
#define ACSDK_ALEXAPRESENTATIONAPL_ALEXAPRESENTATIONAPLFACTORY_H_

#include <memory>
#include <string>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include <acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentNotifierInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/VisualStateProviderInterface.h>

namespace alexaClientSDK {
namespace aplCapabilityAgent {
/**
 * The AlexaPresentationAPLFactory is responsible to create the objects that interacts with @c AlexaPresentationAPL CA
 */
class AlexaPresentationAPLFactory {
private:
    /**
     * This object contains the interfaces to interact with the AlexaPresentationAPL Capability Agent.
     */
    struct AlexaPresentationAPLAgentData {
        /// An interface used to communicate with AlexaPresentationAPL capability agent.
        std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> aplCapabilityAgent;
        /// An Interface used to register observers for the AlexaPresentationAPL capability agent.
        std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentNotifierInterface> capabilityAgentNotifier;
        /// An Interface used to receive AlexaPresentationAPL directives.
        std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent> capabilityAgent;
        /// An interface used to provide accesss to the version and configurations of capability agent.
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface> capabilityConfiguration;
        /// Instance of @c RequiresShutdown used for cleaning up the capability agent during shutdown.
        std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown;
    };

public:
    /**
     * Destructor.
     */
    ~AlexaPresentationAPLFactory() = default;

    /**
     * Create an instance of @c AlexaPresentationAPL.
     *
     * @param exceptionSender The @c ExceptionEncounteredSenderInterface that sends Exception
     * messages to AVS.
     * @param metricRecorder The object @c MetricRecorderInterface that records metrics.
     * @param messageSender The @c MessageSenderInterface that sends events to AVS.
     * @param contextManager The @c ContextManagerInterface used to generate system context for events.
     * @param APLVersion The APLVersion supported by the runtime component.
     * @param visualStateProvider The @c VisualStateProviderInterface used to request visual context.
     */
    static avsCommon::utils::Optional<AlexaPresentationAPLAgentData> create(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::string APLVersion,
        std::shared_ptr<alexaClientSDK::aplCapabilityCommonInterfaces::VisualStateProviderInterface>
            visualStateProvider = nullptr);
};

}  // namespace aplCapabilityAgent
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAPRESENTATIONAPL_ALEXAPRESENTATIONAPLFACTORY_H_
