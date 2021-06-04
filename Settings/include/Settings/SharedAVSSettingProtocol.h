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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SHAREDAVSSETTINGPROTOCOL_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SHAREDAVSSETTINGPROTOCOL_H_

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include "Settings/SetSettingResult.h"
#include "Settings/SettingConnectionObserver.h"
#include "Settings/SettingEventMetadata.h"
#include "Settings/SettingEventSenderInterface.h"
#include "Settings/SettingObserverInterface.h"
#include "Settings/SettingProtocolInterface.h"
#include "Settings/SettingStatus.h"
#include "Settings/Storage/DeviceSettingStorageInterface.h"

namespace alexaClientSDK {
namespace settings {

/**
 * Implement the logic of shared setting protocol. In the shared setting protocol, a change to the setting value can
 * be originated by an AVS or device UI request.
 */
class SharedAVSSettingProtocol : public SettingProtocolInterface {
public:
    /**
     * Create a shared protocol object.
     *
     * @param metadata The setting metadata used to generate a unique database key.
     * @param eventSender Object used to send events to avs in order to report changes to the device.
     * @param settingStorage The setting storage object.
     * @param connectionManager An @c AVSConnectionManagerInterface instance to listen for connection status updates.
     * @param metricRecorder The @c MetricRecorderInterface to record metrics.
     * @param isDefaultCloudAuthoritative Indicates if the default value for the setting should be cloud
     * authoritative or not.  If it is, for the first time when the setting is created, the default value of the
     * setting will be sent to AVS using a report event.  If it is not cloud authoritative, then the default value of
     * the setting will be sent to AVS using a changed event.
     * @return A pointer to the new @c SharedAVSSettingProtocol object if it succeeds; @c nullptr otherwise.
     */
    static std::unique_ptr<SharedAVSSettingProtocol> create(
        const SettingEventMetadata& metadata,
        std::shared_ptr<SettingEventSenderInterface> eventSender,
        std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
        std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
        const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
        bool isDefaultCloudAuthoritative = false);

    /// @name SettingProtocolInterface methods.
    /// @{
    SetSettingResult localChange(
        ApplyChangeFunction applyChange,
        RevertChangeFunction revertChange,
        SettingNotificationFunction notifyObservers) override;

    bool avsChange(
        ApplyChangeFunction applyChange,
        RevertChangeFunction revertChange,
        SettingNotificationFunction notifyObservers) override;

    bool restoreValue(ApplyDbChangeFunction applyChange, SettingNotificationFunction notifyObservers) override;
    bool clearData() override;
    ///@}

    /**
     * The callback method to be called whenever there is a change in connection status.
     * @param isConnected If true, indicates that the device is connected to AVS, otherwise false.
     */
    void connectionStatusChangeCallback(bool isConnected);

    /**
     * Destructor.
     * Deregisters from connection notifications.
     */
    ~SharedAVSSettingProtocol();

private:
    /**
     * Contains information used to fulfill a request to modify a setting.
     */
    struct Request {
        /**
         * Constructor.
         *
         * @param applyFn Callback function type used for applying new setting values.
         * @param revertFn Callback function type used to revert the last setting value change.
         * @param notifyFn Callback function type used to notify observers of whether the request succeeded or failed.
         */
        Request(ApplyChangeFunction applyFn, RevertChangeFunction revertFn, SettingNotificationFunction notifyFn);

        /// Callback function type used for applying new setting values.
        ApplyChangeFunction applyChange;

        /// Callback function type used to revert the last setting value change.
        RevertChangeFunction revertChange;

        /// Callback function type used to notify observers of whether the request succeeded or failed.
        SettingNotificationFunction notifyObservers;
    };

    /**
     * Constructor.
     * @param settingKey The setting key used to access the setting storage.
     * @param eventSender Object used to send events to avs in order to report changes to the device.
     * @param settingStorage The setting storage object.
     * @param connectionManager An @c AVSConnectionManagerInterface instance to listen for connection status updates.
     * @param metricRecorder The @c MetricRecorderInterface to record metrics.
     * @param isDefaultCloudAuthoritative Indicates if the default value for the setting should be cloud
     * authoritative or not.  If it is, for the first time when the setting is created, the default value of the
     * setting will be sent to AVS using a report event.  If it is not cloud authoritative, then the default value of
     * the setting will be sent to AVS using a changed event.
     */
    SharedAVSSettingProtocol(
        const std::string& settingKey,
        std::shared_ptr<SettingEventSenderInterface> eventSender,
        std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
        std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
        const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
        bool isDefaultCloudAuthoritative);

    /**
     * Sends AVS events for non-synchronized settings.
     */
    void executeSynchronizeOnConnected();

    /// The setting key used to access the setting storage.
    const std::string m_key;

    /// A flag to indicate if the default value for this setting is cloud-authoritative or device-authoritative.
    const bool m_isDefaultCloudAuthoritative;

    /// Object used to send events to avs in order to report changes to the device.
    std::shared_ptr<SettingEventSenderInterface> m_eventSender;

    /// The setting storage object.
    std::shared_ptr<storage::DeviceSettingStorageInterface> m_storage;

    /// The AVS connection manager object.
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> m_connectionManager;

    /// The connection observer object.
    std::shared_ptr<SettingConnectionObserver> m_connectionObserver;

    /// The change request to be applied. This value is null if there is no task scheduled to process the request.
    std::unique_ptr<Request> m_pendingRequest;

    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /// The mutex used to serialize access to @c m_pendingRequest
    std::mutex m_requestLock;

    /// Executor used to handle events in sequence.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SHAREDAVSSETTINGPROTOCOL_H_
