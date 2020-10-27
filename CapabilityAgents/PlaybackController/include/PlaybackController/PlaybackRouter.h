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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_PLAYBACKCONTROLLER_INCLUDE_PLAYBACKCONTROLLER_PLAYBACKROUTER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_PLAYBACKCONTROLLER_INCLUDE_PLAYBACKCONTROLLER_PLAYBACKROUTER_H_

#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/PlaybackHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/PlaybackRouterInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace playbackController {

class PlaybackRouter
        : public avsCommon::sdkInterfaces::PlaybackRouterInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<PlaybackRouter> {
public:
    /**
     * Create an instance of @ PlaybackRouterInterface.
     *
     * @param defaultHandler The default playback handler.
     * @param shutdownNotifier The @c ShutdownNotifierInterface to notify this instance when to shut down.
     * @return A @c std::shared_ptr to the new @ PlaybackRouterInterface instance.
     */
    static std::shared_ptr<PlaybackRouterInterface> createPlaybackRouterInterface(
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackHandlerInterface> defaultHandler,
        std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> shutdownNotifier);

    /**
     * Create an instance of @ PlaybackRouter.
     *
     * @param defaultHandler The default playback handler.
     * @deprecated Use createPlaybackRouterInterface.
     * @return A @c std::shared_ptr to the new @ PlaybackRouter instance.
     */
    static std::shared_ptr<PlaybackRouter> create(
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackHandlerInterface> defaultHandler);

    /**
     * Destructor.
     */
    virtual ~PlaybackRouter() = default;

    /// @name @c PlaybackRouterInterface functions.
    /// @{
    virtual void buttonPressed(avsCommon::avs::PlaybackButton button) override;
    virtual void togglePressed(avsCommon::avs::PlaybackToggle toggle, bool action) override;
    virtual void switchToDefaultHandler() override;
    virtual void setHandler(
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackHandlerInterface> handler,
        std::shared_ptr<avsCommon::sdkInterfaces::LocalPlaybackHandlerInterface> localHandler = nullptr) override;
    virtual void useDefaultHandlerWith(
        std::shared_ptr<avsCommon::sdkInterfaces::LocalPlaybackHandlerInterface> localHandler) override;
    virtual bool localOperation(avsCommon::sdkInterfaces::LocalPlaybackHandlerInterface::PlaybackOperation op) override;
    virtual bool localSeekTo(std::chrono::milliseconds location, bool fromStart) override;
    /// @}

private:
    /**
     * Constructor.
     */
    PlaybackRouter(std::shared_ptr<avsCommon::sdkInterfaces::PlaybackHandlerInterface> defaultHandler);

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /// The active button press handler.
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackHandlerInterface> m_handler;

    /// The activelocal handler.
    std::shared_ptr<avsCommon::sdkInterfaces::LocalPlaybackHandlerInterface> m_localHandler;

    /// The default handler to be used after @c switchToDefaultHandler has been called.
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackHandlerInterface> m_defaultHandler;

    /// Mutex protecting the observer pointer.
    std::mutex m_handlerMutex;
};

}  // namespace playbackController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_PLAYBACKCONTROLLER_INCLUDE_PLAYBACKCONTROLLER_PLAYBACKROUTER_H_
