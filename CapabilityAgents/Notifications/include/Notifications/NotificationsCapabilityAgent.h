/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONSCAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONSCAPABILITYAGENT_H_

#include <memory>
#include <unordered_set>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/Audio/NotificationsAudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/NotificationsObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <RegistrationManager/CustomerDataHandler.h>

#include "NotificationIndicator.h"
#include "NotificationRendererInterface.h"
#include "NotificationRendererObserverInterface.h"
#include "NotificationsStorageInterface.h"
#include "NotificationsCapabilityAgentState.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace notifications {

/**
 * This class implements the @c Notifications capability agent.
 *
 * @see https://developer.amazon.com/docs/alexa-voice-service/notifications.html
 *
 * @note For instances of this class to be cleaned up correctly, @c shutdown() must be called.
 * @note This class makes use of a global configuration to a database file, meaning that it is best used
 * as a singleton.
 */
class NotificationsCapabilityAgent
        : public NotificationRendererObserverInterface
        , public avsCommon::avs::CapabilityAgent
        , public avsCommon::utils::RequiresShutdown
        , public registrationManager::CustomerDataHandler
        , public std::enable_shared_from_this<NotificationsCapabilityAgent> {
public:
    /**
     * Creates a new @c NotificationsCapabilityAgent instance.
     *
     * @param notificationsStorage The storage interface to the NotificationIndicator database.
     * @param renderer The instance of the @c NotificationRendererInterface used to play assets associated with
     * notifications.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param notificationsAudioFactory The audio factory object to produce the default notification sound.
     * @param observers The set of observers that will be notified of IndicatorState changes.
     * @param dataManager A dataManager object that will track the CustomerDataHandler.
     * @return A @c std::shared_ptr to the new @c NotificationsCapabilityAgent instance.
     */
    static std::shared_ptr<NotificationsCapabilityAgent> create(
        std::shared_ptr<NotificationsStorageInterface> notificationsStorage,
        std::shared_ptr<NotificationRendererInterface> renderer,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::sdkInterfaces::audio::NotificationsAudioFactoryInterface> notificationsAudioFactory,
        std::shared_ptr<registrationManager::CustomerDataManager> dataManager);

    /**
     * Adds a NotificationsObserver to the set of observers. This observer will be notified when a SetIndicator
     * directive arrives.
     *
     * @param observer The observer to add.
     */
    void addObserver(std::shared_ptr<avsCommon::sdkInterfaces::NotificationsObserverInterface> observer);

    /**
     * Removes a NotificationsObserver from the set of observers.
     *
     * @param observer The observer to remove.
     */
    void removeObserver(std::shared_ptr<avsCommon::sdkInterfaces::NotificationsObserverInterface> observer);

    /// @name CapabilityAgent/DirectiveHandlerInterface Functions
    /// @{
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    /// @}

    /// @name StateProviderInterface Functions
    /// @{
    void provideState(const avsCommon::avs::NamespaceAndName& stateProviderName, unsigned int stateRequestToken)
        override;
    /// @}

    /// @name NotificationRendererObserverInterface Functions
    /// @{
    /**
     * This function sets off logic to handle a Notification having been completely rendered.
     */
    void onNotificationRenderingFinished() override;
    /// @}

    /**
     * Clear all notifications saved in the device
     */
    void clearData() override;

private:
    /**
     * Constructor.
     *
     * @param notificationsStorage The storage interface to the NotificationIndicator database.
     * @param renderer The instance of the @c NotificationRendererInterface used to play assets associated with
     * notifications.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param notificationsAudioFactory The audio factory object to produce the default notification sound.
     * @param observers The set of observers that will be notified of IndicatorState changes.
     * @param dataManager A dataManager object that will track the CustomerDataHandler.
     */
    NotificationsCapabilityAgent(
        std::shared_ptr<NotificationsStorageInterface> notificationsStorage,
        std::shared_ptr<NotificationRendererInterface> renderer,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::sdkInterfaces::audio::NotificationsAudioFactoryInterface> notificationsAudioFactory,
        std::shared_ptr<registrationManager::CustomerDataManager> dataManager);

    /**
     * Utility to set some member variables and setup the database.
     * Sets m_isEnabled to true if there are NotificationIndicators stored in the database.
     * Creates the NotificationIndicator database if it doesn't exist, else opens it.
     */
    bool init();

    /**
     * This method deserializes a @c Directive's payload into a @c rapidjson::Document.
     *
     * @param info The @c DirectiveInfo to read the payload string from.
     * @param[out] document The @c rapidjson::Document to parse the payload into.
     * @return @c true if parsing was successful, else @c false.
     */
    bool parseDirectivePayload(std::shared_ptr<DirectiveInfo> info, rapidjson::Document* document);

    /**
     * This method handles a SetIndicator directive.
     *
     * @param info The @c DirectiveInfo to read the payload string from.
     */
    void handleSetIndicatorDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * This method handles a ClearIndicator directive.
     *
     * @param info The @c DirectiveInfo to read the payload string from.
     */
    void handleClearIndicatorDirective(std::shared_ptr<DirectiveInfo> info);

    /// @name RequiresShutdown Methods
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * Cleans up the Directive that has been completely handled.
     *
     * @param info The directive to clean up.
     */
    void setHandlingCompleted(std::shared_ptr<DirectiveInfo> info);

    /**
     * @name Executor Thread Methods
     *
     * These methods (and only these methods) are called by @c m_executor on a single worker thread.  All other
     * methods in this class can be called asynchronously, and pass data to the @c Executor thread through parameters
     * to lambda functions.  No additional synchronization is needed.
     */
    /// @{

    /**
     * Consolidates code for attempting to render a notification.
     * This method checks the playAudioIndicator field of the NotificationIndicator and, if it is set,
     * changes the State to PLAYING, updates m_assetId, and calls m_renderer->renderNotification().
     *
     * @param notificationIndicator The notificationIndicator to attempt to render.
     */
    void executeRenderNotification(const NotificationIndicator& notificationIndicator);

    /**
     * Consolidates code for getting and setting IndicatorState.
     * This method gets the current IndicatorState and compares it with the persistVisualIndicator field
     * of the next NotificationIndicator. If they differ, IndicatorState is updated and observers are notified.
     *
     * @param nextNotificationIndicator The NotificationIndicator to check the current IndicatorState against.
     */
    void executePossibleIndicatorStateChange(const avsCommon::avs::IndicatorState& nextIndicatorState);

    /**
     * Sets the state of the NotificationsCapabilityAgent. All state changes should go through this method.
     *
     * @param newState The state to transition to.
     */
    void executeSetState(NotificationsCapabilityAgentState newState);

    /**
     * Provides relevant state. Called when m_isEnabled or the current IndicatorState changes.
     *
     * @param sendToken flag indicating whether @c stateRequestToken contains a valid token which should be passed
     *     along to @c ContextManager.  This flag defaults to @c false.
     * @param stateRequestToken The token @c ContextManager passed to the @c provideState() call, which will be passed
     *     along to the ContextManager::setState() call.  This parameter is not used if @c sendToken is @c false.
     */
    void executeProvideState(bool sendToken = false, unsigned int stateRequestToken = 0);

    /**
     * Notifies NotificationObservers of the stored IndicatorState.
     *
     * @param state The IndicatorState that warrants notification.
     */
    void notifyObservers(avsCommon::avs::IndicatorState state);

    /**
     * Handles necessary start-up behavior such as the initial observer notify() of the current indicator state
     * and the playing of any remaining NotificationIndicators.
     */
    void executeInit();

    /**
     * Delegates to the correct executePlayFinished<num>Queued() method based on queue size.
     */
    void executeOnPlayFinished();

    /**
     * @name Situation methods.
     *
     * These methods deal with different situations that may arise during the lifetime of a
     * NotificationsCapabilityAgent. They should all run on an executor thread and account for any possible State.
     */
    /// @{

    /**
     * Situation: On start-up, the NotificationIndicator queue may not be empty. The next NotificationIndicator
     * should play and the IndicatorState should be appropriately set.
     */
    void executeStartQueueNotEmpty();

    /**
     * Situation: A SetIndicator directive came down. This should be called from handleSetIndicatorDirective().
     *
     * @param nextNotificationIndicator The next indicator to enqueue and process.
     */
    void executeSetIndicator(
        const NotificationIndicator& nextNotificationIndicator,
        std::shared_ptr<DirectiveInfo> info);

    /**
     * Situation: A ClearIndicator directive came down. This should be called from handleClearIndicatorDirective().
     */
    void executeClearIndicator(std::shared_ptr<DirectiveInfo> info);

    /**
     * Situation: The renderer has completed rendering a Notification and the queue is empty.
     */
    void executePlayFinishedZeroQueued();

    /**
     * Situation: The renderer has completed rendering a Notification and the queue still has one NotificationIndicator
     * in it. Because items are popped from the queue only after processing, this means that the play that just finished
     * corresponds to the item still in the queue. It should be popped with a state transition to IDLE.
     */
    void executePlayFinishedOneQueued();

    /**
     * Situation: The renderer has completed rendering a Notification and the queue has more than one
     * NotificationIndicator left in it.
     */
    void executePlayFinishedMultipleQueued();

    /**
     * Situation: The NotificationsCapabilityAgent needs to clean up and shutdown.
     */
    void executeShutdown();

    /// @}
    /// @}

    /// Stores notification indicators in the order they are received and the visual indicator state.
    std::shared_ptr<NotificationsStorageInterface> m_notificationsStorage;

    /// The @c ContextManager that needs to be updated of the state.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// NotificationRendererInterface instance to play audio indicator.
    std::shared_ptr<NotificationRendererInterface> m_renderer;

    /// The audio factory to produce the default notification sound.
    std::shared_ptr<avsCommon::sdkInterfaces::audio::NotificationsAudioFactoryInterface> m_notificationsAudioFactory;

    /// Holds the current assetId to check against incoming SetIndicator directives.
    std::string m_currentAssetId;

    /// If this flag is true, there are pending notifications that the user has not opened.
    /// Changes to this member variable should be followed by provideState() or executeProvideState().
    bool m_isEnabled;

    /// Set of observers that may be interested in notification indicators.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::NotificationsObserverInterface>> m_observers;

    NotificationsCapabilityAgentState m_currentState;

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;

    /// A condition variable to notify doShutdown() that the state change to SHUTDOWN has completed successfully.
    std::condition_variable m_shutdownTrigger;

    /// Serialize access to m_shutdownTrigger.
    std::mutex m_shutdownMutex;
};

}  // namespace notifications
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONSCAPABILITYAGENT_H_
