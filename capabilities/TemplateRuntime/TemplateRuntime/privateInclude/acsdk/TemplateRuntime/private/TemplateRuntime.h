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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIME_PRIVATEINCLUDE_ACSDK_TEMPLATERUNTIME_PRIVATE_TEMPLATERUNTIME_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIME_PRIVATEINCLUDE_ACSDK_TEMPLATERUNTIME_PRIVATE_TEMPLATERUNTIME_H_

#include <deque>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsObserverInterface.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderInterface.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderRegistrarInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include <acsdk/Notifier/Notifier.h>

#include <acsdk/TemplateRuntimeInterfaces/TemplateRuntimeInterface.h>
#include <acsdk/TemplateRuntimeInterfaces/TemplateRuntimeObserverInterface.h>

namespace alexaClientSDK {
namespace templateRuntime {

/**
 * This class implements a @c CapabilityAgent that handles the AVS @c TemplateRuntime API.  The
 * @c TemplateRuntime CA is responsible for handling the directives with the TemplateRuntime namespace.  Due
 * to the fact that the @c RenderPlayerInfo directives are closely related to the @c AudioPlayer, the @c TemplateRuntime
 * CA is an observer to the AudioPlayer and will be synchronizing the @c RenderPlayerInfo directives with the
 * corresponding @c AudioItem being handled in the @c AudioPlayer.
 *
 * The @c TemplateRuntime CA is also an observer to the @c DialogUXState to determine the end of a interaction so
 * that it would know when to clear a @c RenderTemplate displayCard.
 *
 * The clients who are interested in any TemplateRuntime directives can subscribe themselves as an observer, and the
 * clients will be notified via the TemplateRuntimeObserverInterface.
 */
class TemplateRuntime
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::utils::RequiresShutdown
        , public templateRuntimeInterfaces::TemplateRuntimeInterface
        , public avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public std::enable_shared_from_this<TemplateRuntime> {
public:
    /**
     * Create an instance of @c TemplateRuntime.
     *
     * @param renderPlayerInfoCardsInterfaces A set of objects to use for subscribing @c TemplateRuntime as an
     * observer of changes for RenderPlayerInfoCards.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @return @c nullptr if the inputs are not defined, else a new instance of @c TemplateRuntime.
     */
    static std::shared_ptr<TemplateRuntime> create(
        const std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface>>&
            renderPlayerInfoCardsInterfaces,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender);

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

    /// @name RenderPlayerInfoCardsObserverInterface Functions
    /// @{
    void onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity state, const Context& context) override;
    /// @}

    /// @name TemplateRuntimeInterface Functions
    /// @{
    void addObserver(std::weak_ptr<templateRuntimeInterfaces::TemplateRuntimeObserverInterface> observer) override;
    void removeObserver(std::weak_ptr<templateRuntimeInterfaces::TemplateRuntimeObserverInterface> observer) override;
    void addRenderPlayerInfoCardsProvider(
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface> cardsProvider) override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

private:
    /// Alias for Notifier based on @c TemplateRuntimeObserverInterface.
    using TemplateRuntimeNotifier = notifier::Notifier<templateRuntimeInterfaces::TemplateRuntimeObserverInterface>;

    /**
     * Constructor.
     *
     * @param renderPlayerInfoCardsInterfaces A set of objects to use for subscribing @c TemplateRuntime as an
     * observer of changes for RenderPlayerInfoCards.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     */
    TemplateRuntime(
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface>>
            renderPlayerInfoCardsInterfaces,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender);

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
    void setHandlingCompleted(const std::shared_ptr<DirectiveInfo>& info);

    /**
     * This function handles a @c RenderTemplate directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleRenderTemplateDirective(const std::shared_ptr<DirectiveInfo>& info);

    /**
     * This function handles a @c RenderPlayerInfo directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleRenderPlayerInfoDirective(const std::shared_ptr<DirectiveInfo>& info);

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
    void executeRenderPlayerInfoCallback();

    /**
     * This function handles the notification of the renderTemplateCard callbacks to all the observers.  This function
     * is intended to be used in the context of @c m_executor worker thread.
     */
    void executeRenderTemplateCallback();

    /**
     * This is an internal function that is called when the agent is ready to notify the @TemplateRuntime
     * observers to display a card.
     */
    void executeDisplayCard(std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info);

    /**
     * @name Executor Thread Variables
     *
     * These member variables are only accessed by functions in the @c m_executor worker thread, and do not require any
     * synchronization.
     */
    /// @{
    /// A set of observers to be notified when a @c RenderTemplate or @c RenderPlayerInfo directive is received
    std::list<std::weak_ptr<templateRuntimeInterfaces::TemplateRuntimeObserverInterface>> m_observers;

    /**
     * This is a map that is used to store the current executing @c AudioItem based on the callbacks from the
     * @c RenderPlayerInfoCardsProviderInterface.
     */
    std::unordered_map<std::shared_ptr<avsCommon::sdkInterfaces::MediaPropertiesInterface>, AudioItemPair>
        m_audioItemsInExecution;

    /// The current active RenderPlayerInfoCards provider that has the matching audioItemId.
    std::shared_ptr<avsCommon::sdkInterfaces::MediaPropertiesInterface> m_activeRenderPlayerInfoCardsProvider;

    /**
     * This queue is for storing the @c RenderPlayerInfo directives when its audioItemId does not match the audioItemId
     * in execution in the @c AudioPlayer.
     */
    std::deque<AudioItemPair> m_audioItems;

    /// This map is to store the @c AudioPlayerInfo to be passed to the observers in the renderPlayerInfoCard callback.
    std::unordered_map<
        std::shared_ptr<avsCommon::sdkInterfaces::MediaPropertiesInterface>,
        templateRuntimeInterfaces::TemplateRuntimeObserverInterface::AudioPlayerInfo>
        m_audioPlayerInfo;

    /// The directive corresponding to the RenderTemplate directive.
    std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> m_lastDisplayedDirective;

    /// A flag to check if @c RenderTemplate is the last directive received.
    bool m_isRenderTemplateLastReceived;
    /// @}

    /**
     * This is a set of interfaces to the @c RenderPlayerInfoCardsProviderInterface.  The @c TemplateRuntime CA
     * used this interface to add and remove itself as an observer.
     */
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface>>
        m_renderPlayerInfoCardsInterfaces;

    /// Pointer to the TemplateRuntime notifier.
    std::shared_ptr<TemplateRuntimeNotifier> m_notifier;

    /// Set of capability configurations that will get published using the Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// This is the worker thread for the @c TemplateRuntime CA.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace templateRuntime
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIME_PRIVATEINCLUDE_ACSDK_TEMPLATERUNTIME_PRIVATE_TEMPLATERUNTIME_H_
