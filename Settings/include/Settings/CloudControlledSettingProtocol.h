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

#include <memory>

#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

#include "Settings/SetSettingResult.h"
#include "Settings/SettingEventMetadata.h"
#include "Settings/SettingEventSenderInterface.h"
#include "Settings/SettingProtocolInterface.h"
#include "Settings/SharedAVSSettingProtocol.h"
#include "Settings/Storage/DeviceSettingStorageInterface.h"

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_CLOUDCONTROLLEDSETTINGPROTOCOL_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_CLOUDCONTROLLEDSETTINGPROTOCOL_H_

namespace alexaClientSDK {
namespace settings {

/**
 * Implement the protocol for cloud controlled settings.
 */
class CloudControlledSettingProtocol : public SettingProtocolInterface {
public:
    /**
     * Create a cloud controlled protocol object.
     *
     * @param metadata Specifies the parameters used to construct setting changed and report event.
     * @param eventSender Object used to send events to AVS in order to report changes from the device.
     * @param settingStorage The setting storage object.
     * @param connectionManager An @c AVSConnectionManagerInterface instance to listen for connection status updates.
     * @param metricRecorder An @c MetricRecorderInterface instance to log metrics.
     * @return A pointer to the new @c CloudControlledSettingProtocol object if it succeeds; @c nullptr otherwise.
     */
    static std::unique_ptr<CloudControlledSettingProtocol> create(
        const SettingEventMetadata& metadata,
        std::shared_ptr<SettingEventSenderInterface> eventSender,
        std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
        std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
        const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder);

    /**
     * Destructor.
     */
    ~CloudControlledSettingProtocol() = default;

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

private:
    /**
     * Constructor.
     *
     * @param sharedProtocol The underlying protocol implementation. The cloud controlled setting uses the shared AVS
     * setting protocol to implement avs changes.
     */
    CloudControlledSettingProtocol(std::unique_ptr<SharedAVSSettingProtocol> sharedProtocol);

    /// Pointer to the shared protocol implementation.
    std::unique_ptr<SharedAVSSettingProtocol> m_protocolImpl;
};

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_CLOUDCONTROLLEDSETTINGPROTOCOL_H_
