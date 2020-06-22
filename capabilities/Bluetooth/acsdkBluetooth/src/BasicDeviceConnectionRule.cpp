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
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSinkInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSourceInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPControllerInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPTargetInterface.h>

#include "acsdkBluetooth/BasicDeviceConnectionRule.h"

namespace alexaClientSDK {
namespace acsdkBluetooth {
std::shared_ptr<BasicDeviceConnectionRule> BasicDeviceConnectionRule::create() {
    return std::shared_ptr<BasicDeviceConnectionRule>(new BasicDeviceConnectionRule());
}

bool BasicDeviceConnectionRule::shouldExplicitlyConnect() {
    return true;
}

bool BasicDeviceConnectionRule::shouldExplicitlyDisconnect() {
    return true;
}

std::set<std::shared_ptr<BluetoothDeviceInterface>> BasicDeviceConnectionRule::devicesToDisconnect(
    std::map<DeviceCategory, std::set<std::shared_ptr<BluetoothDeviceInterface>>> connectedDevices) {
    /**
     * Bluetooth CapabilityAgent only supports single A2DP device connected at one time.
     * Need to disconnect the connected A2DP device(if available) whenever a new A2DP connects.
     */
    std::set<std::shared_ptr<BluetoothDeviceInterface>> devicesToDisconnect;
    for (const auto& category : m_categories) {
        auto devicesIt = connectedDevices.find(category);
        if (devicesIt != connectedDevices.end()) {
            devicesToDisconnect.insert(devicesIt->second.begin(), devicesIt->second.end());
        }
    }
    return devicesToDisconnect;
}

BasicDeviceConnectionRule::BasicDeviceConnectionRule() {
    m_categories = {DeviceCategory::AUDIO_VIDEO, DeviceCategory::PHONE, DeviceCategory::OTHER, DeviceCategory::UNKNOWN};
    m_profiles = {avsCommon::sdkInterfaces::bluetooth::services::A2DPSinkInterface::UUID,
                  avsCommon::sdkInterfaces::bluetooth::services::A2DPSourceInterface::UUID,
                  avsCommon::sdkInterfaces::bluetooth::services::AVRCPControllerInterface::UUID,
                  avsCommon::sdkInterfaces::bluetooth::services::AVRCPTargetInterface::UUID};
}

std::set<DeviceCategory> BasicDeviceConnectionRule::getDeviceCategories() {
    return m_categories;
}

std::set<std::string> BasicDeviceConnectionRule::getDependentProfiles() {
    return m_profiles;
}

}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK
