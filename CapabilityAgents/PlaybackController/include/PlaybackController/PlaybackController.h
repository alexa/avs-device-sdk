/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_PLAYBACKCONTROLLER_INCLUDE_PLAYBACKCONTROLLER_PLAYBACKCONTROLLER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_PLAYBACKCONTROLLER_INCLUDE_PLAYBACKCONTROLLER_PLAYBACKCONTROLLER_H_

#include <memory>
#include <queue>
#include <string>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextRequesterInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/PlaybackHandlerInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include "PlaybackCommand.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace playbackController {

class PlaybackController
        : public avsCommon::sdkInterfaces::ContextRequesterInterface
        , public avsCommon::sdkInterfaces::PlaybackHandlerInterface
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<PlaybackController> {
public:
    /**
     * Create an instance of @c PlaybackController.
     *
     * @param contextManager The @c ContextManagerInterface used to generate system context for events.
     * @param messageSender The @c MessageSenderInterface that sends events to AVS.
     * @return @c nullptr if the inputs are not defined, else a new instance of @c PlaybackController.
     */
    static std::shared_ptr<PlaybackController> create(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /**
     * Destructor.
     */
    virtual ~PlaybackController() = default;

    /// @name ContextRequesterInterface Functions
    /// @{
    void onContextAvailable(const std::string& jsonContext) override;
    void onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) override;
    /// @}

    /// @name PlaybackControllerInterface Functions
    /// @{
    void onButtonPressed(avsCommon::avs::PlaybackButton button) override;
    void onTogglePressed(avsCommon::avs::PlaybackToggle toggle, bool action) override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    /**
     * Manage completion of event being sent.
     *
     * @param PlaybackCommand The @PlaybackButton or @PlaybackToggle that was pressed to generate the message sent.
     * @param messageStatus The status of submitted @c MessageRequest.
     */
    void messageSent(
        const PlaybackCommand&,
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status messageStatus);

private:
    /**
     * Constructor.
     *
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param messageSender The message sender interface that sends events to AVS.
     */
    PlaybackController(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * Process the @c PlaybackCommand for the pressed button.
     *
     * @param The @c PlaybackCommand associated with the button pressed.
     */
    void handleCommand(const PlaybackCommand& command);

    /**
     * @name Executor Thread Variables
     *
     * These member variables are only accessed by functions in the @c m_executor worker thread, and do not require any
     * synchronization.
     */
    /// @{

    /// The @c MessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The queue for storing the commands.
    std::queue<const PlaybackCommand*> m_commands;
    /// @}

    /// Set of capability configurations that will get published using the Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// The @c Executor which queues up operations from asynchronous API calls to the @c PlaybackControllerInterface.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace playbackController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_PLAYBACKCONTROLLER_INCLUDE_PLAYBACKCONTROLLER_PLAYBACKCONTROLLER_H_
