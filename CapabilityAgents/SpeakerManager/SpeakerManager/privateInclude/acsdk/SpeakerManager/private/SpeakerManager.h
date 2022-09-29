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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_SPEAKERMANAGER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_SPEAKERMANAGER_H_

#include <future>
#include <map>
#include <memory>
#include <unordered_set>
#include <vector>

#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <acsdk/SpeakerManager/SpeakerManagerConfigInterface.h>
#include <acsdk/SpeakerManager/SpeakerManagerStorageInterface.h>
#include <acsdk/SpeakerManager/SpeakerManagerStorageState.h>
#include <acsdk/SpeakerManager/private/SpeakerManagerConfigHelper.h>
#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/Utils/functional/hash.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/RetryTimer.h>
#include <AVSCommon/Utils/WaitEvent.h>

namespace alexaClientSDK {
namespace speakerManager {

/**
 * @brief Capability Agent for Speaker API.
 *
 * This class implements a @c CapabilityAgent that handles the AVS @c Speaker API.
 *
 * The @c SpeakerManager can handle multiple @c ChannelVolumeInterface objects and dedupe them with
 * the same getId() value. The @c ChannelVolumeInterface are grouped by their respective
 * @c ChannelVolumeInterface::Type, and the volume and mute state will be consistent across each type.
 * For example, to change the volume of all @c ChannelVolumeInterface objects of a specific type:
 *
 * @code{.cpp}
 *     // Use local setVolume API.
 *     std::future<bool> result = speakerManager->setVolume(type, AVS_SET_VOLUME_MAX);
 *     // Optionally, wait for the operation to complete.
 *     if (result.valid()) {
 *          result.wait();
 *     }
 * @endcode
 *
 * Clients may extend the @c ChannelVolumeInterface::Type enum if multiple independent volume controls are needed.
 *
 * @ingroup Lib_acsdkSpeakerManager
 */
class SpeakerManager
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::sdkInterfaces::SpeakerManagerInterface
        , public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Create an instance of @c SpeakerManagerInterface. @c ChannelVolumeInterfaces can be registered via
     * addChannelVolumeInterface.
     *
     * @param storage A @c MiscStorageInterface to access persistent configuration.
     * @param contextManager A @c ContextManagerInterface to manage the context.
     * @param messageSender A @c MessageSenderInterface to send messages to AVS.
     * @param exceptionEncounteredSender An @c ExceptionEncounteredSenderInterface to send
     * directive processing exceptions to AVS.
     * @param shutdownNotifier A @c ShutdownNotifierInterface to notify the SpeakerManager when it's time to shut down.
     * @param endpointCapabilitiesRegistrar The @c EndpointCapabilitiesRegistrarInterface for the default endpoint
     * (annotated with DefaultEndpointAnnotation), so that the SpeakerManager can register itself as a capability
     * with the default endpoint.
     * @param metricRecorder The metric recorder.
     */
    static std::shared_ptr<SpeakerManagerInterface> createSpeakerManagerCapabilityAgent(
        std::shared_ptr<SpeakerManagerConfigInterface> config,
        std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> storage,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier,
        const std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>&
            endpointCapabilitiesRegistrar,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) noexcept;

    /**
     * Create an instance of @c SpeakerManager, and register the @c ChannelVolumeInterfaces that will be controlled
     * by it. ChannelVolumeInterfaces will be grouped by @c ChannelVolumeInterface::Type.
     *
     * @param storage A @c SpeakerManagerStorageInterface to access persistent configuration.
     * @param volumeInterfaces The @c ChannelVolumeInterfaces to register.
     * @param contextManager A @c ContextManagerInterface to manage the context.
     * @param messageSender A @c MessageSenderInterface to send messages to AVS.
     * @param exceptionEncounteredSender An @c ExceptionEncounteredSenderInterface to send
     * directive processing exceptions to AVS.
     * @param metricRecorder The metric recorder.
     */
    static std::shared_ptr<SpeakerManager> create(
        std::shared_ptr<SpeakerManagerConfigInterface> config,
        std::shared_ptr<SpeakerManagerStorageInterface> storage,
        const std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>>& volumeInterfaces,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder = nullptr) noexcept;

    /// @name CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /// @name SpeakerManagerInterface Functions
    /// @{
    std::future<bool> setVolume(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        int8_t volume,
        const NotificationProperties& properties) override;
    std::future<bool> adjustVolume(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        int8_t delta,
        const NotificationProperties& properties) override;
    std::future<bool> setMute(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        bool mute,
        const NotificationProperties& properties) override;
    std::future<bool> setVolume(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        int8_t volume,
        bool forceNoNotifications = false,
        avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source source =
            avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source::LOCAL_API) override;
    std::future<bool> adjustVolume(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        int8_t delta,
        bool forceNoNotifications = false,
        avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source source =
            avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source::LOCAL_API) override;
    std::future<bool> setMute(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        bool mute,
        bool forceNoNotifications = false,
        avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source source =
            avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source::LOCAL_API) override;

#ifdef ENABLE_MAXVOLUME_SETTING
    std::future<bool> setMaximumVolumeLimit(const int8_t maximumVolumeLimit) override;
#endif
    std::future<bool> getSpeakerSettings(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings) override;
    void onExternalSpeakerSettingsUpdate(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& speakerSettings,
        const NotificationProperties& properties) override;
    void addSpeakerManagerObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer) override;
    void removeSpeakerManagerObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer) override;
    void addChannelVolumeInterface(
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> channelVolumeInterface) override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

private:
    /**
     * Constructor. Called after validation has occurred on parameters.
     *
     * @param speakerManagerStorage The @c SpeakerManagerStorageInterace to register.
     * @param groupVolumeInterfaces The @c ChannelVolumeInterfaces to register.
     * @param contextManager A @c ContextManagerInterface to manage the context.
     * @param messageSender A @c MessageSenderInterface to send messages to AVS.
     * @param exceptionEncounteredSender An @c ExceptionEncounteredSenderInterface to send.
     * directive processing exceptions to AVS.
     * @param metricRecorder The metric recorder.
     */
    SpeakerManager(
        std::shared_ptr<SpeakerManagerConfigInterface> speakerManagerConfig,
        std::shared_ptr<SpeakerManagerStorageInterface> speakerManagerStorage,
        const std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>>& groupVolumeInterfaces,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) noexcept;

    /// Hash functor to use identifier of @c ChannelVolumeInterface as the key in SpeakerSet.
    struct ChannelVolumeInterfaceHash {
    public:
        size_t operator()(const std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>& key) const noexcept {
            if (nullptr == key) {
                /// This should never happen because the only way to add a ChannelVolumeInterface into the SpeakerSet
                /// has a guard of nullptr.
                return 0;
            }
            return key->getId();
        }
    };

    /// Comparator to compare two shared_ptrs of @c ChannelVolumeInterface objects in SpeakerSet.
    struct ChannelVolumeInterfaceEqualTo {
    public:
        bool operator()(
            const std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> channelVolumeInterface1,
            std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> channelVolumeInterface2) const noexcept {
            if (!channelVolumeInterface1 || !channelVolumeInterface2) {
                /// This should never happen because the only way to add a ChannelVolumeInterface into the SpeakerSet
                /// has a guard of nullptr.
                return true;
            }
            return channelVolumeInterface1->getId() == channelVolumeInterface2->getId();
        }
    };

    /// Alias for a set of @c ChannelVolumeInterface keyed by the id.
    using SpeakerSet = std::unordered_set<
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>,
        ChannelVolumeInterfaceHash,
        ChannelVolumeInterfaceEqualTo>;

    /**
     * Internal function to add @c ChannelVolumeInterface object into SpeakerMap.
     * Invalid element(nullptr or ChannelVolumeInterface with the same getId() value) is not allowed to be added into
     * the map.
     *
     * @param channelVolumeInterface The @c ChannelVolumeInterface object.
     */
    void addChannelVolumeInterfaceIntoSpeakerMap(
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> channelVolumeInterface) noexcept;

    /**
     * Parses the payload from a string into a rapidjson document.
     *
     * @param payload The payload as a string.
     * @param document The document that will contain the payload.
     * @return A bool indicating the results of the operation.
     */
    bool parseDirectivePayload(std::string payload, rapidjson::Document* document) noexcept;

    /**
     * Performs clean-up after a successful handling of a directive.
     *
     * @param info The current directive being processed.
     */
    void executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) noexcept;

    /**
     * Removes the directive after it's been processed.
     *
     * @param info The current directive being processed.
     */
    void removeDirective(std::shared_ptr<DirectiveInfo> info) noexcept;

    /**
     * Sends an exception to AVS.
     *
     * @param info The current directive being processed.
     * @param message The exception message.
     * @param type The type of exception.
     */
    void sendExceptionEncountered(
        std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
        const std::string& message,
        avsCommon::avs::ExceptionErrorType type) noexcept;

    /**
     * Function to update the state of the ContextManager.
     *
     * @param type The @c ChannelVolumeInterface Type that is being updated.
     * @param settings The SpeakerSettings to update the ContextManager with.
     * @return Whether the ContextManager was successfully updated.
     */
    bool updateContextManager(
        const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type& type,
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings) noexcept;

    /**
     * Sends <Volume/Mute>Changed events to AVS. The events are identical except for the name.
     *
     * @param eventName The name of the event.
     * @param settings The current speaker settings.
     */
    void executeSendSpeakerSettingsChangedEvent(
        const std::string& eventName,
        avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings settings) noexcept;

    /**
     * Function to set the volume for a specific @c Type. This runs on a worker thread.
     * Upon success, a VolumeChanged event will be sent to AVS.
     *
     * @param type The type of @c ChannelVolumeInterface to modify volume for.
     * @param volume The volume to change.
     * @param properties Notification properties that specify how the volume change will be notified.
     * @return A bool indicating success.
     */
    bool executeSetVolume(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        int8_t volume,
        const avsCommon::sdkInterfaces::SpeakerManagerInterface::NotificationProperties& properties) noexcept;

    /**
     * Function to restore the volume from a mute state. This runs on a worker thread and will not send an event or
     * notify an observer. Upon success, a VolumeChanged event will be sent to AVS.
     *
     * @param type The type of @c ChannelVolumeInterface to modify volume for.
     * @param source Whether the call is a result from an AVS directive or local interaction.
     * @return A bool indicating success.
     */
    bool executeRestoreVolume(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source source) noexcept;

    /**
     * Function to adjust the volume for a specific @c Type. This runs on a worker thread.
     * Upon success, a VolumeChanged event will be sent to AVS.
     *
     * @param type The type of @c ChannelVolumeInterface to modify volume for.
     * @param delta The delta to change the volume by.
     * @param properties Notification properties that specify how the volume change will be notified.
     * @return A bool indicating success.
     */
    bool executeAdjustVolume(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        int8_t delta,
        const avsCommon::sdkInterfaces::SpeakerManagerInterface::NotificationProperties& properties) noexcept;

    /**
     * Function to set the mute for a specific @c Type. This runs on a worker thread.
     * Upon success, a MuteChanged event will be sent to AVS.
     *
     * @param type The type of @c ChannelVolumeInterface to modify mute for.
     * @param mute Whether to mute/unmute.
     * @param properties Notification properties that specify how the volume change will be notified.
     * @return A bool indicating success.
     */
    bool executeSetMute(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        bool mute,
        const avsCommon::sdkInterfaces::SpeakerManagerInterface::NotificationProperties& properties) noexcept;

#ifdef ENABLE_MAXVOLUME_SETTING
    /**
     * Function to set a limit on the maximum volume. This runs on a worker thread.
     *
     * @param type The type of speaker to set a limit on the maximum volume.
     * @return A bool indicating success.
     */
    bool executeSetMaximumVolumeLimit(const int8_t maximumVolumeLimit) noexcept;
#endif  // ENABLE_MAXVOLUME_SETTING

    /**
     * Function to get the speaker settings for a specific @c ChannelVolumeInterface Type.
     * This runs on a worker thread.
     *
     * @param type The type of speaker to get speaker settings.
     * @param[out] settings The settings if successful.
     * @return A bool indicating success.
     */
    bool executeGetSpeakerSettings(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings) noexcept;

    /**
     * Function to set the speaker settings for a specific @c ChannelVolumeInterface Type.
     * This runs on a worker thread.
     *
     * @param type The type of speaker to set speaker settings.
     * @param settings The new settings.
     * @return A bool indicating success.
     */
    bool executeSetSpeakerSettings(
        const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings) noexcept;

    /**
     * Function that initializes and populates @c m_speakerSettings for the given @c type's speaker settings.
     *
     * @param type The type of speaker to initialize
     * @return A bool indicating success.
     */
    bool executeInitializeSpeakerSettings(avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type) noexcept;

    /**
     * Function to send events when settings have changed.
     * This runs on a worker thread.
     *
     * @param settings The new settings.
     * @param eventName The event name to send.
     * @param source Whether the call is a result from an AVS directive or local interaction.
     * @param type The Speaker type.
     */
    void executeNotifySettingsChanged(
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings,
        const std::string& eventName,
        const avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source& source,
        const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type& type) noexcept;

    /**
     * Function to notify the observer when a @c SpeakerSettings change has occurred.
     *
     * @param source. This indicates the origin of the call.
     * @param type. This indicates the type of speaker that was modified.
     * @param settings. This indicates the current settings after the change.
     */
    void executeNotifyObserver(
        const avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source& source,
        const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type& type,
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings) noexcept;

    /**
     * Persist channel configuration.
     */
    void executePersistConfiguration() noexcept;

    /**
     * Helper method to convert internally stored channel state into config format.
     *
     * @param type Channel type.
     * @param storageState Config container.
     */
    void convertSettingsToChannelState(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        SpeakerManagerStorageState::ChannelState* storageState) noexcept;

    /**
     * Get the maximum volume limit.
     *
     * @return The maximum volume limit.
     */
    int8_t getMaximumVolumeLimit() noexcept;

    /**
     * Applies Settings to All Speakers
     * Attempts to synchronize by backing off using a retry timeout table
     * @tparam Task The type of task to execute.
     * @tparam Args The argument types for the task to execute.
     * @param task A callable type representing a task.
     * @param args The arguments to call the task with.
     * @return A bool indicating success.
     */
    bool retryAndApplySettings(const std::function<bool()>& task) noexcept;

    /**
     * Configure channel volume and mute status defaults.
     *
     * @param type Channel type.
     * @param state Channel state.
     * @return A bool indicating success.
     */
    void presetChannelDefaults(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        const SpeakerManagerStorageState::ChannelState& state) noexcept;

    /**
     * Configures channels with default values.
     */
    void loadConfiguration() noexcept;

    /**
     * Updates volume and mute status on managed channels according to configured settings.
     */
    void updateChannelSettings() noexcept;

    /**
     * Updates managed channels according to configured settings.
     * @param type Channel type.
     */
    void updateChannelSettings(avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type) noexcept;

    /**
     * Helper function to adjust input volume into acceptable range.
     *
     * @param volume Input volume.
     * @return Volume within correct limits.
     */
    int8_t adjustVolumeRange(int64_t volume) noexcept;

    /// Component's configuration access.
    SpeakerManagerConfigHelper m_config;

    /// The metric recorder.
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c MessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// the @c volume to restore to when unmuting at 0 volume
    uint8_t m_minUnmuteVolume;

    /// An unordered_map contains ChannelVolumeInterfaces keyed by @c Type. Only internal function
    /// addChannelVolumeInterfaceIntoSpeakerMap can insert an element into this map to ensure that no invalid element
    /// is added. The @c ChannelVolumeInterface in the map is deduped by the getId() value.
    std::unordered_map<
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type,
        SpeakerSet,
        avsCommon::utils::functional::EnumClassHash>
        m_speakerMap;

    /// The observers to be notified whenever any of the @c SpeakerSetting changing APIs are called.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface>> m_observers;

    /// Set of capability configurations that will get published using the Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// Object used to wait for event transmission cancellation.
    avsCommon::utils::WaitEvent m_waitCancelEvent;

    /// Retry Timer object.
    avsCommon::utils::RetryTimer m_retryTimer;

    /// The number of retries that will be done on an event in case of setting synchronization failure.
    const std::size_t m_maxRetries;

    /// maximumVolumeLimit The maximum volume level speakers in this system can reach.
    int8_t m_maximumVolumeLimit;

    /// Persistent Storage flag set from configuration
    bool m_persistentStorage;

    /// Restore mute state flag from configuration
    bool m_restoreMuteState;

    /// Mapping of each speaker type to its speaker settings.
    std::map<
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type,
        avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings>
        m_speakerSettings;

    /// An executor to perform operations on a worker thread.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace speakerManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_SPEAKERMANAGER_H_
