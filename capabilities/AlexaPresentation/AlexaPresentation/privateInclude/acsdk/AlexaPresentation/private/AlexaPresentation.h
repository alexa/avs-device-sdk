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

#ifndef ACSDK_ALEXAPRESENTATION_PRIVATE_ALEXAPRESENTATION_H_
#define ACSDK_ALEXAPRESENTATION_PRIVATE_ALEXAPRESENTATION_H_

#include <memory>
#include <queue>
#include <string>
#include <tuple>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include <acsdk/AlexaPresentationInterfaces/AlexaPresentationCapabilityAgentInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/PresentationToken.h>

namespace alexaClientSDK {
namespace alexaPresentation {
/**
 * The AlexaPresentation CA is responsible to report presentation states to AVS.
 */
class AlexaPresentation
        : public alexaClientSDK::avsCommon::avs::CapabilityAgent
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public alexaClientSDK::avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public alexaClientSDK::alexaPresentationInterfaces::AlexaPresentationCapabilityAgentInterface
        , public std::enable_shared_from_this<AlexaPresentation> {
public:
    /**
     * Destructor.
     */
    virtual ~AlexaPresentation() = default;

    /**
     * Create an instance of @c AlexaPresentation.
     *
     * @param exceptionSender The @c ExceptionEncounteredSenderInterface that sends Exception
     * messages to AVS.
     * @param messageSender The @c MessageSenderInterface that sends events to AVS.
     * @param contextManager The @c ContextManagerInterface used to generate system context for events.
     */
    static std::shared_ptr<AlexaPresentation> create(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    /// Device SDK Facing Interfaces
    /// @name CapabilityAgent/DirectiveHandlerInterface Functions
    /// @{
    void handleDirectiveImmediately(std::shared_ptr<alexaClientSDK::avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;
    alexaClientSDK::avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>>
    getCapabilityConfigurations() override;
    /// @}

    /// @name ContextRequesterInterface Functions
    /// @{
    void onContextAvailable(const std::string& jsonContext) override;
    void onContextFailure(const alexaClientSDK::avsCommon::sdkInterfaces::ContextRequestError error) override;
    /// @}

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /// @name AlexaPresentationCapabilityAgentInterface functions
    /// @{
    virtual void onPresentationDismissed(const aplCapabilityCommonInterfaces::PresentationToken& token) override;
    /// @}

    /// Tests Facing interfaces
    /**
     * Set the executor used as the worker thread
     * @param executor The @c Executor to set
     * @note This function should only be used for testing purposes. No call to any other method should
     * be done prior to this call.
     */
    void setExecutor(const std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor>& executor);

private:
    /**
     * Constructor.
     *
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param messageSender The @c MessageSenderInterface that sends events to AVS.
     * @param contextManager The @c ContextManagerInterface used to generate system context for events.
     */
    AlexaPresentation(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    /**
     * Remove a directive from the map of message IDs to DirectiveInfo instances.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective whose message ID is to be removed.
     */
    void removeDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * Send the handling completed notification and clean up the resources.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void setHandlingCompleted(std::shared_ptr<DirectiveInfo> info);

    /**
     * Queue an AVS event to be sent when context is available
     * @param avsNamespace namespace of the event
     * @param name name of the event
     * @param payload payload of the event
     */
    void executeSendEvent(const std::string& avsNamespace, const std::string& name, const std::string& payload);

    /**
     * This function handles any unknown directives received.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleUnknownDirective(std::shared_ptr<DirectiveInfo> info);

    /// The object to use for sending events.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The queue of events to be sent to AVS.
    std::queue<std::tuple<std::string, std::string, std::string>> m_events;

    /// This is the worker thread for the @c AlexaPresentation CA.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> m_executor;
};

}  // namespace alexaPresentation
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAPRESENTATION_PRIVATE_ALEXAPRESENTATION_H_
