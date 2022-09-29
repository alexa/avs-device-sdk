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

#ifndef ACSDK_ALEXAPRESENTATION_ALEXAPRESENTATIONFACTORY_H_
#define ACSDK_ALEXAPRESENTATION_ALEXAPRESENTATIONFACTORY_H_

#include <memory>
#include <string>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include <acsdk/AlexaPresentationInterfaces/AlexaPresentationCapabilityAgentInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/PresentationToken.h>

namespace alexaClientSDK {
namespace alexaPresentation {
/**
 * The AlexaPresentationFactory is responsible to create the objects that interacts with @c AlexaPresentation CA
 */
class AlexaPresentationFactory {
private:
    /**
     * This object contains the interfaces to interact with the AlexaPresentation Capability Agent.
     */
    struct AlexaPresentationAgentData {
        /// An interface used to handle AlexaPresentation capability agent.
        std::shared_ptr<alexaPresentationInterfaces::AlexaPresentationCapabilityAgentInterface> alexaPresentation;
        /// An interface used to provide accesss to the version and configurations of capability agent.
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface> capabilityConfiguration;
        /// Instance of @c RequiresShutdown used for cleaning up the capability agent during shutdown.
        std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown;
    };

public:
    /**
     * Destructor.
     */
    ~AlexaPresentationFactory() = default;

    /**
     * Create an instance of @c AlexaPresentation.
     *
     * @param exceptionSender The @c ExceptionEncounteredSenderInterface that sends Exception
     * messages to AVS.
     * @param messageSender The @c MessageSenderInterface that sends events to AVS.
     * @param contextManager The @c ContextManagerInterface used to generate system context for events.
     */
    static avsCommon::utils::Optional<AlexaPresentationAgentData> create(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);
};

}  // namespace alexaPresentation
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAPRESENTATION_ALEXAPRESENTATIONFACTORY_H_
