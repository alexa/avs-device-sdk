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

#ifndef ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_DEVICEPROPERTYAGGREGATOR_H_
#define ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_DEVICEPROPERTYAGGREGATOR_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <AVSCommon/SDKInterfaces/Diagnostics/DevicePropertyAggregatorInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace diagnostics {

/**
 * Utility class to query for Device Properties.
 */
class DevicePropertyAggregator
        : public avsCommon::sdkInterfaces::diagnostics::DevicePropertyAggregatorInterface
        , public std::enable_shared_from_this<DevicePropertyAggregator> {
public:
    /**
     * Creates a new @c DevicePropertyAggregator.
     *
     * @return a new instance of @c DevicePropertyAggregator.
     */
    static std::shared_ptr<DevicePropertyAggregator> create();

    /// @name DevicePropertyAggregatorInterface
    /// @{
    avsCommon::utils::Optional<std::string> getDeviceProperty(const std::string& propertyKey) override;
    std::unordered_map<std::string, std::string> getAllDeviceProperties() override;
    void setContextManager(std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) override;
    void initializeVolume(std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager) override;
    /// @}

    /// @name AlertObserverInterface Functions
    /// @{
    void onAlertStateChange(
        const std::string& alertToken,
        const std::string& alertType,
        State state,
        const std::string& reason = "") override;
    /// @}

    /// @name AudioPlayerObserverInterface Functions
    /// @{
    void onPlayerActivityChanged(avsCommon::avs::PlayerActivity state, const Context& context) override;
    /// @}

    /// @name ConnectionStatusObserverInterface Functions
    /// @{
    void onConnectionStatusChanged(const Status status, const ChangedReason reason) override;
    /// @}

    /// @name ContextRequesterInterface Functions
    /// @{
    void onContextAvailable(const std::string& jsonContext) override;
    void onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) override;
    /// @}

    /// @name NotificationsObserverInterface Functions
    /// @{
    void onSetIndicator(avsCommon::avs::IndicatorState state) override;
    void onNotificationReceived() override;
    /// @}

    /// @name SpeakerManagerObserverInterface Functions
    /// @{
    void onSpeakerSettingsChanged(
        const Source& source,
        const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type& type,
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings) override;
    /// @}

    /// @name DialogUXStateObserverInterface Functions
    /// @{
    void onDialogUXStateChanged(DialogUXState newState) override;
    /// @}

private:
    /**
     * Constructor.
     */
    DevicePropertyAggregator();

    /**
     * Requests the device context from @c ContextManager.
     *
     * @return The device context as a JSON string. An empty optional will be returned.
     */
    avsCommon::utils::Optional<std::string> getDeviceContextJson();

    /**
     * Initializes the property map with default values.
     * Note: This method is not thread safe.
     */
    void initializePropertyMap();

    /**
     * Updates the property map with the speaker settings passed in.
     * Note: This method is not thread safe.
     *
     * @param type The @c ChannelVolumeInterface::Type enum representing the speaker setting type.
     * @param settings The @c @SpeakerInterface::SpeakerSettings containing the speaker settings values.
     */
    void updateSpeakerSettingsInPropertyMap(
        const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type& type,
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings);

    /// An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
    avsCommon::utils::threading::Executor m_executor;

    /// The property map containing the property key and the corresponding value.
    std::unordered_map<std::string, std::string> m_propertyMap;

    /// The mutex to synchronize device context.
    std::mutex m_deviceContextMutex;

    /// The device context string.
    avsCommon::utils::Optional<std::string> m_deviceContext;

    /// The condition variable to notify when the context is ready.
    std::condition_variable m_contextWakeTrigger;

    /// The @c ContextManager to request the context from.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;
};

}  // namespace diagnostics
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_DEVICEPROPERTYAGGREGATOR_H_
