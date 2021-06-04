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

#ifndef ACSDKDONOTDISTURB_DONOTDISTURBCAPABILITYAGENT_H_
#define ACSDKDONOTDISTURB_DONOTDISTURBCAPABILITYAGENT_H_

#include <atomic>
#include <memory>
#include <mutex>

#include <rapidjson/document.h>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/ExceptionErrorType.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/Setting.h>
#include <Settings/SettingEventMetadata.h>
#include <Settings/SettingEventSenderInterface.h>
#include <Settings/Storage/DeviceSettingStorageInterface.h>
#include <acsdkManufactory/Annotated.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace doNotDisturb {

/**
 * Capability Agent to handle Alexa.DoNotDisturb AVS Interface.
 *
 * When DoNotDisturb mode is on AVS blocks some interactions from reaching the device so the customer won't be
 * disturbed. Locally SDK provides only the way to get immediate state of the DND mode, track its changes coming from
 * any source and update it making sure that it will be synchronized with AVS. No other customer experience is affected.
 */
class DoNotDisturbCapabilityAgent
        : public std::enable_shared_from_this<DoNotDisturbCapabilityAgent>
        , public alexaClientSDK::avsCommon::avs::CapabilityAgent
        , public alexaClientSDK::avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public alexaClientSDK::settings::SettingEventSenderInterface {
public:
    /**
     * Destructor.
     */
    ~DoNotDisturbCapabilityAgent() override = default;

    /**
     * Factory method to create a capability agent instance.
     *
     * @param messageSender Interface to send events to AVS.
     * @param settingsStorage The storage interface that will be used to store device settings.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param shutdownNotifier The object to notify this AudioPlayer when to shut down.
     * @param endpointCapabilitiesRegistrar The object with which to register this AudioPlayer's capabilities for the
     * default endpoint.
     * @param connectionManager The object to notify this CA of connection status changes.
     * @param metricRecorder The @c MetricRecorderInterface instance to record metrics.
     * @return A new instance of @c DoNotDisturbCapabilityAgent on success, @c nullptr otherwise.
     */
    static std::shared_ptr<DoNotDisturbCapabilityAgent> createDoNotDisturbCapabilityAgent(
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
        const std::shared_ptr<alexaClientSDK::settings::storage::DeviceSettingStorageInterface>& settingsStorage,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
            exceptionSender,
        const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier,
        const acsdkManufactory::Annotated<
            alexaClientSDK::avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
            alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>&
            endpointCapabilitiesRegistrar,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AVSConnectionManagerInterface>&
            connectionManager,
        const std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder);

    /**
     * Factory method to create a capability agent instance.
     *
     * @deprecated Use createDoNotDisturbCapabilityAgent
     * @param exceptionEncounteredSender Interface to report exceptions to AVS.
     * @param messageSender Interface to send events to AVS.
     * @param settingsStorage The storage interface that will be used to store device settings.
     * @return A new instance of @c DoNotDisturbCapabilityAgent on success, @c nullptr otherwise.
     */
    static std::shared_ptr<DoNotDisturbCapabilityAgent> create(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>
            exceptionEncounteredSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<settings::storage::DeviceSettingStorageInterface> settingsStorage);

    /// @name CapabilityAgent Functions
    /// @{
    alexaClientSDK::avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<alexaClientSDK::avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(
        std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>>
    getCapabilityConfigurations() override;
    /// @}

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /// @name SettingEventSenderInterface Functions
    /// @{
    std::shared_future<bool> sendChangedEvent(const std::string& value) override;
    std::shared_future<bool> sendReportEvent(const std::string& value) override;
    std::shared_future<bool> sendStateReportEvent(const std::string& payload) override;
    void cancel() override;
    /// @}

    /// @name ConnectionStatusObserverInterface Functions
    /// @{
    void onConnectionStatusChanged(const Status status, const ChangedReason reason) override;
    /// @}

    /**
     * Get DND Setting.
     *
     * @todo ACSDK-2279: Change DND to use a setting protocol. For now, expose the setting.
     */
    std::shared_ptr<settings::DoNotDisturbSetting> getDoNotDisturbSetting() const;

    /**
     * Gets the do not disturb events metadata.
     *
     * @return The do not disturb events metadata.
     */
    static settings::SettingEventMetadata getDoNotDisturbEventsMetadata();

private:
    /**
     * Constructor.
     *
     * @param exceptionEncounteredSender Interface to report exceptions to AVS.
     * @param messageSender Interface to send events to AVS.
     * @param connectionManager Interface to notify the CA of connection changes.
     */
    DoNotDisturbCapabilityAgent(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>
            exceptionEncounteredSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager);

    /**
     * Helper method to contain shared logic between new and deprecated factory methods for DoNotDisturb
     * CA.
     *
     * @param messageSender Interface to send events to AVS.
     * @param settingsStorage The storage interface that will be used to store device settings.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param connectionManager Optional object to notify this CA of connection status changes.
     * @return A new instance of @c DoNotDisturbCapabilityAgent on success, @c nullptr otherwise.
     */
    static std::shared_ptr<DoNotDisturbCapabilityAgent> create(
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
        const std::shared_ptr<alexaClientSDK::settings::storage::DeviceSettingStorageInterface>& settingsStorage,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
            exceptionSender,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AVSConnectionManagerInterface>&
            connectionManager = nullptr,
        const std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder =
            nullptr);

    /**
     * Method to initialize the new instance of the capability agent.
     *
     * @param settingsStorage The storage interface that will be used to store device settings.
     * @return True on success, false otherwise.
     */
    bool initialize(
        std::shared_ptr<settings::storage::DeviceSettingStorageInterface> settingsStorage,
        const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder);

    /**
     * Sends a DND event to the AVS.
     *
     * @param eventName Name of the event to send.
     * @param value Valid JSON string representation of the boolean value. I.e. either "true" or "false".
     * @return Future to track the completion status of the message.
     */
    std::shared_future<alexaClientSDK::avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status> sendDNDEvent(
        const std::string& eventName,
        const std::string& value);

    /**
     * Prepares AVS Interface DCF configuration and keeps it internally.
     */
    void generateCapabilityConfiguration();

    /**
     * Handles the Alexa.DoNotDisturb.SetDoNotDisturb AVS Directive.
     *
     * @param info Directive details.
     * @param JSON document containing the parsed directive payload.
     * @return true if operation succeeds and could be reported as such to AVS, false if an error occurred. False
     * implies that exception has been reported to AVS and directive is already processed.
     */
    bool handleSetDoNotDisturbDirective(
        std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        rapidjson::Document& document);

    /// Set of capability configurations that will get published using DCF
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>>
        m_capabilityConfigurations;

    /// The @c MessageSenderInterface used to send event messages.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The @c AVSConnectionManagerInterface used to be notified of connection status changes.
    /// This is stored as a member variable so that the CA can remove itself as an observer on shutdown.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AVSConnectionManagerInterface> m_connectionManager;

    /// The do not disturb mode setting.
    std::shared_ptr<alexaClientSDK::settings::Setting<bool>> m_dndModeSetting;

    /**
     * Flag indicating latest reported connection status. True if SDK is connected to the AVS and ready,
     * false otherwise.
     */
    bool m_isConnected;

    /// Mutex to synchronize operations related to connection state.
    std::mutex m_connectedStateMutex;

    /// Flag indicating whether there were changes made to the DND status while being offline.
    std::atomic_bool m_hasOfflineChanges;

    /// An executor used for serializing requests on agent's own thread of execution.
    alexaClientSDK::avsCommon::utils::threading::Executor m_executor;
};

}  // namespace doNotDisturb
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ACSDKDONOTDISTURB_DONOTDISTURBCAPABILITYAGENT_H_
