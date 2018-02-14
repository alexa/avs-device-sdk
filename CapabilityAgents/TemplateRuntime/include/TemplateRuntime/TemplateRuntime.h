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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_INCLUDE_TEMPLATERUNTIME_TEMPLATERUNTIME_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_INCLUDE_TEMPLATERUNTIME_TEMPLATERUNTIME_H_

#include <memory>
#include <queue>
#include <string>
#include <unordered_set>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/AudioPlayerInterface.h>
#include <AVSCommon/SDKInterfaces/AudioPlayerObserverInterface.h>
#include <AVSCommon/SDKInterfaces/TemplateRuntimeObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace templateRuntime {

/**
 * This class implements a @c CapabilityAgent that handles the AVS @c TemplateRuntime API.  The
 * @c TemplateRuntime CA is responsible for handling the directives with the TemplateRuntime namespace.  Due
 * to the fact that the @c RenderPlayerInfo directives are closely related to the @c AudioPlayer, the @c TemplateRuntime
 * CA is an observer to the AudioPlayer and will be synchronizing the @c RenderPlayerInfo directives with the
 * corresponding @c AudioItem being handled in the @c AudioPlayer.
 *
 * The clients who are interested in any TemplateRuntime directives can subscribe themselves as an observer, and the
 * clients will be notified via the TemplateRuntimeObserverInterface.
 */
class TemplateRuntime
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::utils::RequiresShutdown
        , public avsCommon::sdkInterfaces::AudioPlayerObserverInterface
        , public std::enable_shared_from_this<TemplateRuntime> {
public:
    /**
     * Create an instance of @c TemplateRuntime.
     *
     * @param audioPlayerInterface The object to use for subscribing @c TemplateRuntime as an observer of
     * the @c AudioPlayer.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @return @c nullptr if the inputs are not defined, else a new instance of @c TemplateRuntime.
     */
    static std::shared_ptr<TemplateRuntime> create(
        std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerInterface> audioPlayerInterface,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender);

    /**
     * Destructor.
     */
    virtual ~TemplateRuntime() = default;

    /// @name CapabilityAgent/DirectiveHandlerInterface Functions
    /// @{
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    /// @}

    /// @name AudioPlayerObserverInterface Functions
    /// @{
    void onPlayerActivityChanged(avsCommon::avs::PlayerActivity state, const Context& context) override;
    /// @}

    /**
     * This function adds an observer to @c TemplateRuntime so that it will get notified for renderTemplateCard or
     * renderPlayerInfoCard.
     *
     * @param observer The @c TemplateRuntimeObserverInterface
     */
    void addObserver(std::shared_ptr<avsCommon::sdkInterfaces::TemplateRuntimeObserverInterface> observer);

    /**
     * This function removes an observer from @c TemplateRuntime so that it will no longer be notified of
     * renderTemplateCard or renderPlayerInfoCard callbacks.
     *
     * @param observer The @c TemplateRuntimeObserverInterface
     */
    void removeObserver(std::shared_ptr<avsCommon::sdkInterfaces::TemplateRuntimeObserverInterface> observer);

private:
    /**
     * Utility structure to correspond a directive with its audioItemId.
     */
    struct AudioItemPair {
        /**
         * Default Constructor.
         */
        AudioItemPair() = default;

        /**
         * Constructor.
         *
         * @param itemId The ID for the @c AudioItem.
         * @param renderPlayerInfoDirective The @c RenderPlayerInfo directive that corresponds to the audioItemId.
         */
        AudioItemPair(
            std::string itemId,
            std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> renderPlayerInfoDirective) :
                audioItemId{itemId},
                directive{renderPlayerInfoDirective} {};

        /// The ID of the @c AudioItem.
        std::string audioItemId;

        /// The directive corresponding to the audioItemId.
        std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> directive;
    };

    /**
     * Constructor.
     *
     * @param audioPlayerInterface The object to use for subscribing @c TemplateRuntime as an observer of
     * AudioPlayer.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     */
    TemplateRuntime(
        std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerInterface> audioPlayerInterface,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender);

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

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
     * This function handles a @c RenderTemplate directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleRenderTemplateDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles a @c RenderPlayerInfo directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleRenderPlayerInfoDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles any unknown directives received by @c TemplateRuntime CA.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleUnknownDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * This is an internal function that handles updating the @c m_audioItemInExecution when the @c AudioPlayer
     * notifies the @c TemplateRuntime CA of any changes in the @c AudioPlayer audio state.  This function is
     * intended to be used in the context of @c m_executor worker thread.
     *
     * @param state The @c PlayerActivity of the @c AudioPlayer.
     * @param context The @c Context of the @c AudioPlayer at the time of the notification.
     */
    void executeAudioPlayerInfoUpdates(avsCommon::avs::PlayerActivity state, const Context& context);

    /**
     * This function handles the notification of the renderPlayerInfoCard callbacks to all the observers.  This function
     * is intended to be used in the context of @c m_executor worker thread.
     */
    void executeRenderPlayerInfoCallbacks();

    /**
     * @name Executor Thread Variables
     *
     * These member variables are only accessed by functions in the @c m_executor worker thread, and do not require any
     * synchronization.
     */
    /// @{
    /// A set of observers to be notified when a @c RenderTemplate or @c RenderPlayerInfo direective is recevied
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::TemplateRuntimeObserverInterface>> m_observers;

    /*
     * This is used to store the current executing @c AudioItem based on the callbacks from the
     * AudioPlayerObserverInterface.
     */
    AudioItemPair m_audioItemInExecution;

    /*
     * This queue is for storing the @c RenderPlayerInfo directives when its audioItemId does not match the audioItemId
     * in execution in the @c AudioPlayer.
     */
    std::queue<AudioItemPair> m_audioItems;

    /// This is to store the @c AudioPlayerInfo to be passed to the observers in the renderPlayerInfoCard callback.
    avsCommon::sdkInterfaces::TemplateRuntimeObserverInterface::AudioPlayerInfo m_audioPlayerInfo;

    /// A flag to check if @c RenderTemplate is the last directive received.
    bool m_isRenderTemplateLastReceived;
    /// @}

    /*
     * This is an interface to the @c AudioPlayer.  The @c TemplateRuntime CA used this interface to add and remove
     * itself as an observer to the @c AudioPlayer.  The interface is also used to query the latest offset of the audio
     * playback in the @c AudioPlayer.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::AudioPlayerInterface> m_audioPlayerInterface;

    /// This is the worker thread for the @c TemplateRuntime CA.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace templateRuntime
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_INCLUDE_TEMPLATERUNTIME_TEMPLATERUNTIME_H_
