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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_ALERTSCAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_ALERTSCAPABILITYAGENT_H_

#include "Alerts/AlertObserverInterface.h"
#include "Alerts/Alert.h"
#include "Alerts/AlertScheduler.h"
#include "RegistrationManager/CustomerDataHandler.h"

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/Audio/AlertsAudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include <CertifiedSender/CertifiedSender.h>
#include <RegistrationManager/CustomerDataManager.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/Setting.h>
#include <Settings/SettingEventMetadata.h>

#include <chrono>
#include <set>
#include <string>
#include <unordered_set>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {

static const std::chrono::minutes ALERT_PAST_DUE_CUTOFF_MINUTES = std::chrono::minutes(30);

/**
 * This class implements an Alerts capability agent.
 */
class AlertsCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::ConnectionStatusObserverInterface
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::sdkInterfaces::SpeakerManagerObserverInterface
        , public avsCommon::sdkInterfaces::FocusManagerObserverInterface
        , public AlertObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public registrationManager::CustomerDataHandler
        , public std::enable_shared_from_this<AlertsCapabilityAgent> {
public:
    /**
     * Create function.
     *
     * @param messageSender An interface to which this object will send Events to AVS.
     * @param connectionManager An @c AVSConnectionManagerInterface instance to listen for connection status updates.
     * @param certifiedMessageSender An interface to which this object will send guaranteed Events to AVS.
     * @param focusManager An interface with which this object will request and release Alert channel focus.
     * @param speakerManager An interface to control volume of the Alerts.
     * @param contextManager An interface to which this object will send context updates as alert states change.
     * @param exceptionEncounteredSender An interface which allows ExceptionEncountered Events to be sent to AVS.
     * @param alertStorage An interface to store, load, modify and delete Alerts.
     * @param alertsAudioFactory A provider of audio streams specific to Alerts.
     * @param alertRenderer An alert renderer, which Alerts will use to generate user-perceivable effects when active.
     * @param dataManager A dataManager object that will track the CustomerDataHandler.
     * @param alarmVolumeRampSetting The alarm volume ramp setting.
     * @param settingsManager A settingsManager object that manages alarm volume ramp setting.
     * @param metricRecorder The metric recorder.
     * @return A pointer to an object of this type, or nullptr if there were problems during construction.
     */
    static std::shared_ptr<AlertsCapabilityAgent> create(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
        std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<storage::AlertStorageInterface> alertStorage,
        std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface> alertsAudioFactory,
        std::shared_ptr<renderer::RendererInterface> alertRenderer,
        std::shared_ptr<registrationManager::CustomerDataManager> dataManager,
        std::shared_ptr<settings::AlarmVolumeRampSetting> alarmVolumeRampSetting,
        std::shared_ptr<settings::DeviceSettingsManager> settingsManager,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    /// @name CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;

    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;

    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;

    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;

    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;

    void onDeregistered() override;

    void onConnectionStatusChanged(const Status status, const ChangedReason reason) override;

    void onFocusChanged(avsCommon::avs::FocusState focusState, avsCommon::avs::MixingBehavior behavior) override;

    void onAlertStateChange(
        const std::string& token,
        const std::string& alertType,
        AlertObserverInterface::State state,
        const std::string& reason) override;
    /// @}

    /// @name FocusManagerObserverInterface Functions
    /// @{
    void onFocusChanged(const std::string& channelName, avsCommon::avs::FocusState newFocus) override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    /// @name SpeakerManagerObserverInterface Functions
    /// @{
    void onSpeakerSettingsChanged(
        const Source& source,
        const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type& type,
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings) override;
    /// @}

    /**
     * Adds an observer to be notified of alert status changes.
     *
     * @param observer The observer to add.
     */
    void addObserver(std::shared_ptr<AlertObserverInterface> observer);

    /**
     * Removes an observer from being notified of alert status changes.
     *
     * @param observer The observer to remove.
     */
    void removeObserver(std::shared_ptr<AlertObserverInterface> observer);

    /**
     * A function that allows an application to clear all alerts from storage.  This may be useful for a scenario
     * where a user logs out of a device, and another user will log in.  As the first user logs out, their pending
     * alerts should not go off.
     */
    void removeAllAlerts();

    /**
     * This function provides a way for application code to request this object stop any active alert as the result
     * of a user action, such as pressing a physical 'stop' button on the device.
     */
    void onLocalStop();

    /**
     * Clear all scheduled alerts.
     */
    void clearData() override;

    /**
     * Return the alarm volume ramp event metadata.
     *
     * @return The alarm volume ramp event metadata.
     */
    static settings::SettingEventMetadata getAlarmVolumeRampMetadata();

private:
    /**
     * Constructor.
     *
     * @param messageSender An interface to which this object will send Events to AVS.
     * @param certifiedMessageSender An interface to which this object will send guaranteed Events to AVS.
     * @param focusManager An interface with which this object will request and release Alert channel focus.
     * @param speakerManager An interface to control volume of the Alerts.
     * @param contextManager An interface to which this object will send context updates as stored alerts change.
     * @param exceptionEncounteredSender An interface which allows ExceptionEncountered messages to be sent to AVS.
     * @param alertStorage An interface to store, load, modify and delete Alerts.
     * @param alertsAudioFactory A provider of audio streams specific to Alerts.
     * @param alertRenderer An alert renderer, which Alerts will use to generate user-perceivable effects when active.
     * @param dataManager A dataManager object that will track the CustomerDataHandler.
     * @param alarmVolumeRampSetting The alarm volume ramp setting.
     * @param settingsManager A settingsManager object that manages alarm volume ramp setting.
     * @param metricRecorder The metric recorder.
     */
    AlertsCapabilityAgent(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<storage::AlertStorageInterface> alertStorage,
        std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface> alertsAudioFactory,
        std::shared_ptr<renderer::RendererInterface> alertRenderer,
        std::shared_ptr<registrationManager::CustomerDataManager> dataManager,
        std::shared_ptr<settings::AlarmVolumeRampSetting> alarmVolumeRampSetting,
        std::shared_ptr<settings::DeviceSettingsManager> settingsManager,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    void doShutdown() override;

    /**
     * Initializes this object.
     *
     * @return @c true if it succeeds; @c false otherwise.
     */
    bool initialize();

    /**
     * Initializes the alerts for this object.
     *
     * @return True if successful, false otherwise.
     */
    bool initializeAlerts();

    /**
     * @name Executor Thread Functions
     *
     * These functions (and only these functions) are called by @c m_executor on a single worker thread.  All other
     * public functions in this class can be called asynchronously, and pass data to the @c Executor thread through
     * parameters to lambda functions.  No additional synchronization is needed.
     *
     * The other private functions in this class are called from the execute* functions, and so also don't require
     * additional synchronization.
     */
    /// @{

    /**
     * An implementation of this directive handling function, which may be called by the internal executor.
     *
     * @param info The Directive information.
     */
    void executeHandleDirectiveImmediately(std::shared_ptr<DirectiveInfo> info);

    /**
     * A handler function which will be called by our internal executor when the connection status changes.
     *
     * @param status The connection status.
     * @param reason The reason the connection status changed.
     */
    void executeOnConnectionStatusChanged(const Status status, const ChangedReason reason);

    /**
     * A handler function which will be called by our internal executor when the channel focus changes.
     *
     * @param focusState The current focus of the channel.
     */
    void executeOnFocusChanged(avsCommon::avs::FocusState focusState);

    /**
     * A handler function which will be called by our internal executor when the FocusManager reports channel focus
     * changes.
     *
     * @param channelName Name of the channel whose focus is changed.
     * @param focusState The current focus of the channel.
     */
    void executeOnFocusManagerFocusChanged(const std::string& channelName, avsCommon::avs::FocusState focusState);

    /**
     * A handler function which will be called by our internal executor when the SpeakerManager reports volume
     * changes.
     */
    void executeOnSpeakerSettingsChanged(
        const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type& type,
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings);

    /**
     * A handler function which will be called by our internal executor when an alert's status changes.
     *
     * @param alertToken The AVS token identifying the alert.
     * @param alertType The type of the alert.
     * @param state The state of the alert.
     * @param reason The reason the the state changed, if applicable.
     */
    void executeOnAlertStateChange(
        const std::string& alertToken,
        const std::string& alertType,
        AlertObserverInterface::State state,
        const std::string& reason);

    /**
     * A handler function which will be called by our internal executor to add an alert observer.
     *
     * @param observer The observer to add.
     */
    void executeAddObserver(std::shared_ptr<AlertObserverInterface> observer);

    /**
     * A handler function which will be called by our internal executor to remove an alert observer.
     *
     * @param observer The observer to remove.
     */
    void executeRemoveObserver(std::shared_ptr<AlertObserverInterface> observer);

    /**
     * A handler function which will be called by our internal executor to notify observers of alert changes.
     *
     * @param alertToken The AVS token identifying the alert.
     * @param alertType The type of the alert.
     * @param state The state of the alert.
     * @param reason The reason the the state changed, if applicable.
     */
    void executeNotifyObservers(
        const std::string& alertToken,
        const std::string& alertType,
        AlertObserverInterface::State state,
        const std::string& reason = "");

    /**
     * A handler function which will be called by our internal executor to remove all alerts currently being managed.
     */
    void executeRemoveAllAlerts();

    /**
     * A handler function which will be called by our internal executor to handle a local stop.
     */
    void executeOnLocalStop();

    /**
     * A helper function to handle the SetAlert directive.
     *
     * @param directive The AVS Directive.
     * @param payload The payload containing the alert data fields.
     * @param[out] alertToken The AVS alert token, required for us to notify AVS whether the SetAlert failed or
     * succeeded.
     * @return Whether the SetAlert processing was successful.
     */
    bool handleSetAlert(
        const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
        const rapidjson::Document& payload,
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
    bool handleDeleteAlert(
        const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
        const rapidjson::Document& payload,
        std::string* alertToken);

    /**
     * A helper function to handle the DeleteAlerts directive.
     *
     * @param directive The AVS Directive.
     * @param payload The payload containing the alert data fields.
     * @return Whether the DeleteAlerts processing was successful.
     */
    bool handleDeleteAlerts(
        const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
        const rapidjson::Document& payload);

    /**
     * A helper function to handle the SetVolume directive.
     *
     * @param directive The AVS Directive.
     * @param payload The payload containing the alert data fields.
     * @return Whether the SetVolume processing was successful.
     */
    bool handleSetVolume(
        const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
        const rapidjson::Document& payload);

    /**
     * A helper function to handle the AdjustVolume directive.
     *
     * @param directive The AVS Directive.
     * @param payload The payload containing the alert data fields.
     * @return Whether the AdjustVolume processing was successful.
     */
    bool handleAdjustVolume(
        const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
        const rapidjson::Document& payload);

    /**
     * A helper function to handle the SetAlarmVolumeRamp directive.
     *
     * @param directive The AVS Directive.
     * @param payload The payload containing the new value for alarm volume ramp.
     * @return Whether the directive processing was successful.
     */
    bool handleSetAlarmVolumeRamp(
        const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
        const rapidjson::Document& payload);

    /**
     * Utility function to send a single alert related Event to AVS. If isCertified is set to true, then the Event
     * will be guaranteed to be sent to AVS at some point in the future, even if there is no currently active
     * connection.  If it is set to false, and there is no currently active connection, the Event will not be sent.
     *
     * @param eventName The name of the Event to be sent.
     * @param alertToken The token of the Alert being sent to AVS within the Event.
     * @param isCertified Whether the event must be guaranteed to be sent.  See function description for details.
     */
    void sendEvent(const std::string& eventName, const std::string& alertToken, bool isCertified = false);

    /**
     * Utility function to send a multiple alerts related Event to AVS. If isCertified is set to true, then the Event
     * will be guaranteed to be sent to AVS at some point in the future, even if there is no currently active
     * connection.  If it is set to false, and there is no currently active connection, the Event will not be sent.
     *
     * @param eventName The name of the Event to be sent.
     * @param tokenList The list of Alert tokens being sent to AVS within the Event.
     * @param isCertified Whether the event must be guaranteed to be sent.  See function description for details.
     */
    void sendBulkEvent(const std::string& eventName, const std::list<std::string>& tokenList, bool isCertified = false);

    /**
     * The function sends Alerts.VolumeChanged event when forced or there is a difference between current volume and
     * the volume used to update AVS last time.
     *
     * @param forceUpdate should AVS be notified regardless of real difference.
     */
    void updateAVSWithLocalVolumeChanges(int8_t volume, bool forceUpdate);

    /**
     * A utility function to simplify calling the ExceptionEncounteredSender.
     *
     * @param directive The AVS Directive which resulted in the exception.
     * @param errorMessage The error message we will send to AVS.
     */
    void sendProcessingDirectiveException(
        const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
        const std::string& errorMessage);

    /**
     * A utility function to acquire the Alerts channel.
     */
    void acquireChannel();

    /**
     * A utility function to release the Alerts channel.
     */
    void releaseChannel();

    /**
     * A utility function to update the focus manager with the current state of all alerts.
     */
    void updateContextManager();

    /**
     * Creates a Context string for the alerts currently being managed.
     *
     * @return A Context string for the alerts currently being managed.
     */
    std::string getContextString();

    /**
     * Returns current @c SpeakerSettings for the AVS_ALERTS_VOLUME volume type.
     *
     * @param[out] speakerSettings @c SpeakerSetting object to receive value
     * @return true on success, false otherwise
     */
    bool getAlertVolumeSettings(avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* speakerSettings);

    /**
     * Returns current @c SpeakerSettings for the AVS_SPEAKER_VOLUME volume type.
     *
     * @param[out] speakerSettings @c SpeakerSetting object to receive value
     * @return true on success, false otherwise
     */
    bool getSpeakerVolumeSettings(avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* speakerSettings);

    /**
     * Updates the volume of the next alert to sound. Only next alert is affected, the volume of the currently sounding
     * alert is not changed.
     *
     * @param volume Volume for the next alert to sound with
     */
    void setNextAlertVolume(int64_t volume);

    /**
     * @name Executor Thread Variables
     *
     * These variables are only accessed by the @c m_executor, with the exception of initialization, and shutdown.
     * The first thing shutdown does is shutdown the @c m_executor, thus making this safe.
     */
    /// @{

    /// The metric recorder.
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;
    /// The regular MessageSender object.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;
    /// The CertifiedSender object.
    std::shared_ptr<certifiedSender::CertifiedSender> m_certifiedSender;
    /// The FocusManager object.
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> m_focusManager;
    /// The SpeakerManager object.
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> m_speakerManager;
    /// The ContextManager object.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// Set of observers to notify when an alert status changes.  @c m_mutex must be acquired before access.
    std::unordered_set<std::shared_ptr<AlertObserverInterface>> m_observers;

    /// Variable to capture if we are currently connected to AVS.
    bool m_isConnected;

    /// Our helper object that takes care of managing alert persistence and rendering.
    AlertScheduler m_alertScheduler;

    /// @}

    /// This member contains a factory to provide unique audio streams for the various alerts.
    std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface> m_alertsAudioFactory;

    /// Set of capability configurations that will get published using the Capabilities API.
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// Speaker settings used last time to report alerts volume to AVS.
    avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings m_lastReportedSpeakerSettings;

    /// Flag indicating whether Content Channel is active now.
    bool m_contentChannelIsActive;

    /// Flag indicating whether Comms Channel is active now.
    bool m_commsChannelIsActive;

    /// Flag indicating if there is an active alert sounding at the moment.
    bool m_alertIsSounding;

    /**
     * The @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;

    /**
     * The alarm volume ramp setting.
     */
    std::shared_ptr<settings::AlarmVolumeRampSetting> m_alarmVolumeRampSetting;

    /// The settings manager used to retrieve the value of alarm volume ramp setting.
    std::shared_ptr<settings::DeviceSettingsManager> m_settingsManager;
};

}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_ALERTSCAPABILITYAGENT_H_