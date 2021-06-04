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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_DEVICECONTROLLEDSETTINGPROTOCOL_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_DEVICECONTROLLEDSETTINGPROTOCOL_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include "Settings/SetSettingResult.h"
#include "Settings/SettingEventMetadata.h"
#include "Settings/SettingEventSenderInterface.h"
#include "Settings/SettingProtocolInterface.h"
#include "Settings/SharedAVSSettingProtocol.h"
#include "Settings/Storage/DeviceSettingStorageInterface.h"

namespace alexaClientSDK {
namespace settings {

/**
 * Implement the logic of device controlled setting protocol.
 */
class DeviceControlledSettingProtocol : public SettingProtocolInterface {
public:
    /**
     * Create a device controlled protocol object.
     *
     * @param metadata The setting metadata used to generate a unique database key.
     * @param eventSender Object used to send events to avs in order to report changes to the device.
     * @param settingStorage The setting storage object.
     * @param connectionManager An @c AVSConnectionManagerInterface instance to listen for connection status updates.
     * @param metricRecorder An @c MetricRecorderInterface instance to log metrics.
     * @return A pointer to the new @c SharedAVSSettingProtocol object if it succeeds; @c nullptr otherwise.
     */
    static std::unique_ptr<DeviceControlledSettingProtocol> create(
        const SettingEventMetadata& metadata,
        std::shared_ptr<SettingEventSenderInterface> eventSender,
        std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
        std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
        const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder);

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
     * Destructor.
     */
    ~DeviceControlledSettingProtocol() = default;

private:
    /**
     * Constructor.
     * @param sharedProtocol The underlying protocol implementation. The device controlled setting uses the shared
     * protocol to implement local changes.
     */
    DeviceControlledSettingProtocol(std::unique_ptr<SharedAVSSettingProtocol> sharedProtocol);

    /// Pointer to the shared protocol implementation.
    std::unique_ptr<SharedAVSSettingProtocol> m_protocolImpl;
};

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_DEVICECONTROLLEDSETTINGPROTOCOL_H_
