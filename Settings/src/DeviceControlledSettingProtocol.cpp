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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "Settings/DeviceControlledSettingProtocol.h"

namespace alexaClientSDK {
namespace settings {

/// String to identify log entries originating from this file.
static const std::string TAG("DeviceControlledSettingProtocol");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<DeviceControlledSettingProtocol> DeviceControlledSettingProtocol::create(
    const SettingEventMetadata& metadata,
    std::shared_ptr<SettingEventSenderInterface> eventSender,
    std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder) {
    auto sharedProtocol =
        SharedAVSSettingProtocol::create(metadata, eventSender, settingStorage, connectionManager, metricRecorder);
    if (!sharedProtocol) {
        ACSDK_ERROR(LX("createFailed").d("reason", "cannotCreateProtocolImplementation"));
        return nullptr;
    }
    return std::unique_ptr<DeviceControlledSettingProtocol>(
        new DeviceControlledSettingProtocol(std::move(sharedProtocol)));
}

SetSettingResult DeviceControlledSettingProtocol::localChange(
    SettingProtocolInterface::ApplyChangeFunction applyChange,
    SettingProtocolInterface::RevertChangeFunction revertChange,
    SettingProtocolInterface::SettingNotificationFunction notifyObservers) {
    return m_protocolImpl->localChange(applyChange, revertChange, notifyObservers);
}

bool DeviceControlledSettingProtocol::avsChange(
    SettingProtocolInterface::ApplyChangeFunction applyChange,
    SettingProtocolInterface::RevertChangeFunction revertChange,
    SettingProtocolInterface::SettingNotificationFunction notifyObservers) {
    ACSDK_ERROR(LX("avsChangeFailed").d("reason", "unsupportedOperation"));
    return false;
}

bool DeviceControlledSettingProtocol::restoreValue(
    SettingProtocolInterface::ApplyDbChangeFunction applyChange,
    SettingProtocolInterface::SettingNotificationFunction notifyObservers) {
    return m_protocolImpl->restoreValue(applyChange, notifyObservers);
}

bool DeviceControlledSettingProtocol::clearData() {
    return m_protocolImpl->clearData();
}

DeviceControlledSettingProtocol::DeviceControlledSettingProtocol(
    std::unique_ptr<SharedAVSSettingProtocol> sharedProtocol) :
        m_protocolImpl{std::move(sharedProtocol)} {
}

}  // namespace settings
}  // namespace alexaClientSDK
