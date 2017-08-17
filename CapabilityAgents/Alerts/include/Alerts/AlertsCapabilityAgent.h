/*
 * AlertsCapabilityAgent.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_ALERTS_CAPABILITY_AGENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_ALERTS_CAPABILITY_AGENT_H_

#include "Alerts/AlertObserverInterface.h"
#include "Alerts/Alert.h"
#include "Alerts/Storage/AlertStorageInterface.h"

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>

#include <chrono>
#include <set>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {

static const std::chrono::minutes ALERT_PAST_DUE_CUTOFF_MINUTES = std::chrono::minutes(30);

/**
 * This class implements an Alerts capability agent.
 *
 * This class will not send Events to AVS until enableSendEvents() is called.
 */
class AlertsCapabilityAgent : public avsCommon::avs::CapabilityAgent,
                              public avsCommon::sdkInterfaces::ConnectionStatusObserverInterface,
                              public AlertObserverInterface,
                              public avsCommon::utils::RequiresShutdown,
                              public std::enable_shared_from_this<AlertsCapabilityAgent> {
public:
    /**
     * Create function.
     *
     * TODO : ACSDK-394 to change the observer model to support adding / removing multiple observers.
     *
     * @param messageSender An interface to which this object will send Events to AVS.
     * @param focusManager An interface with which this object will request and release Alert channel focus.
     * @param contextManager An interface to which this object will send context updates as stored alerts change.
     * @param exceptionEncounteredSender An interface which allows ExceptionEncountered messages to be sent to AVS.
     * @param renderer An alert renderer, which Alerts will use to generate user-perceivable effects when active.
     * @param alertStorage An interface to store, load, modify and delete Alerts.
     * @param observer An observer which will be notified of any Alert events which occur.
     * @return An pointer to an object of this type, or nullptr if there were problems during construction.
     */
    static std::shared_ptr<AlertsCapabilityAgent> create(
            std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
            std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
            std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
            std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
            std::shared_ptr<renderer::RendererInterface> renderer,
            std::shared_ptr<storage::AlertStorageInterface> alertStorage,
            std::shared_ptr<AlertObserverInterface> observer);

    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;

    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;

    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;

    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;

    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;

    void onDeregistered() override;

    /**
     * Calling this function allows this object to send Events to AVS.
     * This is useful for workflows where other Events need to be sent before this object's Events.
     */
    void enableSendEvents();

    /**
     * Calling this function prevents this object from sending Events to AVS.
     * This allows the system to send other Events which may need to be sent before this object begins sending Events.
     */
    void disableSendEvents();

    void onConnectionStatusChanged(const Status status, const ChangedReason reason) override;

    void onFocusChanged(avsCommon::avs::FocusState focusState) override;

    void onAlertStateChange(
            const std::string & token, AlertObserverInterface::State state, const std::string & reason) override;

    /**
     * This function provides a way for application code to request this object stop any active alert as the result
     * of a user action, such as pressing a physical 'stop' button on the device.
     */
    void onLocalStop();

    /**
     * A function that allows an application to clear all alerts from storage.  This may be useful for a scenario
     * where a user logs out of a device, and another user will log in.  As the first user logs out, their pending
     * alerts should not go off.
     */
    void removeAllAlerts();

private:
    /**
     * Constructor.
     *
     * @param messageSender An interface to which this object will send Events to AVS.
     * @param focusManager An interface with which this object will request and release Alert channel focus.
     * @param contextManager An interface to which this object will send context updates as stored alerts change.
     * @param exceptionEncounteredSender An interface which allows ExceptionEncountered messages to be sent to AVS.
     * @param renderer An alert renderer, which Alerts will use to generate user-perceivable effects when active.
     * @param alertStorage An interface to store, load, modify and delete Alerts.
     * @param observer An observer which will be notified of any Alert events which occur.
     */
    AlertsCapabilityAgent(
            std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
            std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
            std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
            std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
            std::shared_ptr<renderer::RendererInterface> renderer,
            std::shared_ptr<storage::AlertStorageInterface> alertStorage,
            std::shared_ptr<AlertObserverInterface> observer);

    void doShutdown() override;

    /**
     * Initializes this object.
     */
    bool initialize();

    /**
     * Initializes the default sounds for this object.
     *
     * @param configurationRoot The configuration object parsed during SDK initialization.
     */
    bool initializeDefaultSounds(const avsCommon::utils::configuration::ConfigurationNode & configurationRoot);

    /**
     * Initializes the alerts for this object.
     *
     * @param configurationRoot The configuration object parsed during SDK initialization.
     */
    bool initializeAlerts(const avsCommon::utils::configuration::ConfigurationNode & configurationRoot);

    /**
     * An implementation of this directive handling function, which may be called by the internal executor.
     *
     * @param info The Directive information.
     */
    void executeHandleDirectiveImmediately(std::shared_ptr<DirectiveInfo> info);

    /**
     * A helper function to handle the SetAlert directive.
     *
     * @param directive The AVS Directive.
     * @param payload The payload containing the alert data fields.
     * @param[out] alertToken The AVS alert token, required for us to notify AVS whether the SetAlert failed or
     * succeeded.
     * @return Whether the SetAlert processing was successful.
     */
    bool handleSetAlert(const std::shared_ptr<avsCommon::avs::AVSDirective> & directive,
                        const rapidjson::Document & payload,
                        std::string* alertToken);

    /**
     * A helper function to handle the DeleteAlert directive.
     *
     * @param directive The AVS Directive.
     * @param payload The payload containing the alert data fields.
     * @param[out] alertToken The AVS alert token, required for us to notify AVS whether the DeleteAlert failed or
     * succeeded.
     * @return Whether the DeleteAlert processing was successful.
     */
    bool handleDeleteAlert(const std::shared_ptr<avsCommon::avs::AVSDirective> & directive,
                           const rapidjson::Document & payload,
                           std::string* alertToken);

    /**
     * Utility function to send an Event to AVS.  All current Events per AVS documentation are with respect to
     * a single Alert, so the parameter is the given Alert token.
     *
     * @param eventName The name of the Event to be sent.
     * @param alertToken The token of the Alert being sent to AVS within the Event.
     */
    void sendEvent(const std::string & eventName, const std::string & alertToken);

    /**
     * A utility function to simplify calling the ExceptionEncounteredSender.
     *
     * @param directive The AVS Directive which resulted in the exception.
     * @param errorMessage The error message we will send to AVS.
     */
    void sendProcessingDirectiveException(
            const std::shared_ptr<avsCommon::avs::AVSDirective> & directive, const std::string & errorMessage);

    /**
     * A utility function to update the ContextManager with the current state of all Alerts.
     * This function requires that m_mutex be locked.
     */
    void updateContextManagerLocked();

    /**
     * A utility function to enqueue on the executor a request that we determine the next alert to be rendered.
     */
    void scheduleNextAlertForRendering();

    /**
     * A utility function to determine the next alert to be rendered, and sets a timer waiting for it.
     */
    void executeScheduleNextAlertForRendering();

    /**
     * A utility function to handle sending Alert focus changes via the executor.
     *
     * @param alertToken The AVS Token for the alert.
     * @param focusState The focus state of the alert.
     */
    void executeSendAlertFocusChangeEvent(const std::string & alertToken, avsCommon::avs::FocusState focusState);

    /**
     * A utility function to handle sending notifications to the Alert observer via the executor.
     *
     * @param alertToken The AVS Token for the alert.
     * @param state The alert state.
     * @param reason The optional reason for the alert state occurring.
     */
    void executeNotifyObserver(
            const std::string & alertToken, AlertObserverInterface::State state, const std::string & reason = "");

    /**
     * A utility function to acquire the Alerts channel.
     *
     * @return Whether we were successful making the request.
     */
    bool acquireChannel();

    /**
     * A utility function to release the Alerts channel.
     */
    void releaseChannel();

    /**
     * A utility function to activate the next alert in our queue.
     * This function requires that m_mutex be locked.
     */
    void activateNextAlertLocked();

    /**
     * A utility function to retreive a scheduled alert given its token.
     * This function requires that m_mutex be locked.
     *
     * @param token The token for the alert.
     * @return The scheduled Alert, otherwise nullptr.
     */
    std::shared_ptr<Alert> getScheduledAlertByTokenLocked(const std::string & token);

    /**
     * A utility function to update an alert to snooze.
     * This function requires that m_mutex be locked.
     *
     * @param alert The alert to be snoozed.
     * @param updatedTime The new time the alert should occur.
     * @return Whether the snooze was successful.
     */
    bool snoozeAlertLocked(std::shared_ptr<Alert> alert, const std::string & updatedTime);

    /**
     * A utility function to inspect all scheduled alerts, and remove those which are evaluated to be past-due.
     * This also processes any alerts stored in the m_pastDueAlerts container.
     */
    void filterPastDueAlerts();

    /**
     * A utility function to factor out common code when deactivating an alert.
     *
     * @param stopReason The reason the alert is being stopped.
     */
    void deactivateActiveAlertHelper(Alert::StopReason stopReason);

    /// The MessageSender object.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;
    /// The FocusManager object.
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> m_focusManager;
    /// The ContextManager object.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;
    /// The AlertStorage object.
    std::shared_ptr<storage::AlertStorageInterface> m_alertStorage;
    /// The AlertRenderer object.
    std::shared_ptr<renderer::RendererInterface> m_alertRenderer;

    /// The mutex to control accessing local variables in this object.
    std::mutex m_mutex;

    /// The alert, if any, which is currently active.
    std::shared_ptr<Alert> m_activeAlert;
    /// All alerts which are scheduled to occur, ordered ascending by time.
    std::set<std::shared_ptr<Alert>, Alert::TimeComparator> m_scheduledAlerts;
    /// Alerts which are evaluated as past-due at startup.  These will be cleaned up once Events can be sent.
    std::vector<std::shared_ptr<Alert>> m_pastDueAlerts;

    /// Variable to capture if we are currently connected to AVS.
    bool m_isConnected;
    /// Variable to capture if we may send Events.
    bool m_sendEventsEnabled;
    /// Our current focus state.
    avsCommon::avs::FocusState m_focusState;
    // The timer for the next alert to go off, if one is not already active.
    avsCommon::utils::timing::Timer m_scheduledAlertTimer;
    /// An observer of this class that we should notify when an alert activates, and transitions states.
    std::shared_ptr<AlertObserverInterface> m_observer;

    /// The @c Executor which queues up operations from asynchronous API calls to the AlertsCapabilityAgent interface.
    avsCommon::utils::threading::Executor m_caExecutor;
    /// The @c Executor which serially and asynchronously handles operations with regard to the next active alert.
    avsCommon::utils::threading::Executor m_alertSchedulerExecutor;
};

} // namespace alerts
} // namespace capabilityAgents
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_ALERTS_CAPABILITY_AGENT_H_
