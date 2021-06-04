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

#include "Settings/CloudControlledSettingProtocol.h"

namespace alexaClientSDK {
namespace settings {

/// String to identify log entries originating from this file.
static const std::string TAG("CloudControlledSettingProtocol");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<CloudControlledSettingProtocol> CloudControlledSettingProtocol::create(
    const SettingEventMetadata& metadata,
    std::shared_ptr<SettingEventSenderInterface> eventSender,
    std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder) {
    auto sharedProtocol = SharedAVSSettingProtocol::create(
        metadata, eventSender, settingStorage, connectionManager, metricRecorder, true);
    if (!sharedProtocol) {
        ACSDK_ERROR(LX("createFailed").d("reason", "cannot create shared Protocol"));
        return nullptr;
    }
    return std::unique_ptr<CloudControlledSettingProtocol>(
        new CloudControlledSettingProtocol(std::move(sharedProtocol)));
}

CloudControlledSettingProtocol::CloudControlledSettingProtocol(
    std::unique_ptr<SharedAVSSettingProtocol> sharedProtocol) :
        m_protocolImpl{std::move(sharedProtocol)} {
}

SetSettingResult CloudControlledSettingProtocol::localChange(
    SettingProtocolInterface::ApplyChangeFunction applyChange,
    SettingProtocolInterface::RevertChangeFunction revertChange,
    SettingProtocolInterface::SettingNotificationFunction notifyObservers) {
    ACSDK_ERROR(LX("localChangeFailed").d("reason", "unsupportedOperation"));
    return SetSettingResult::UNSUPPORTED_OPERATION;
}

bool CloudControlledSettingProtocol::avsChange(
    SettingProtocolInterface::ApplyChangeFunction applyChange,
    SettingProtocolInterface::RevertChangeFunction revertChange,
    SettingProtocolInterface::SettingNotificationFunction notifyObservers) {
    return m_protocolImpl->avsChange(applyChange, revertChange, notifyObservers);
}

bool CloudControlledSettingProtocol::restoreValue(
    SettingProtocolInterface::ApplyDbChangeFunction applyChange,
    SettingProtocolInterface::SettingNotificationFunction notifyObservers) {
    return m_protocolImpl->restoreValue(applyChange, notifyObservers);
}

bool CloudControlledSettingProtocol::clearData() {
    return m_protocolImpl->clearData();
}

}  // namespace settings
}  // namespace alexaClientSDK