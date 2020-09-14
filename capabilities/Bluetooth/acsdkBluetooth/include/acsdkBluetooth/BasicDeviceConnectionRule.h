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

#ifndef ACSDKBLUETOOTH_BASICDEVICECONNECTIONRULE_H_
#define ACSDKBLUETOOTH_BASICDEVICECONNECTIONRULE_H_

#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceConnectionRuleInterface.h>

namespace alexaClientSDK {
namespace acsdkBluetooth {

using namespace avsCommon::sdkInterfaces::bluetooth;
/**
 * A class represents the basic connection rule the following Bluetooth device needs to follow:
 * 1) DeviceCategory::PHONE
 * 2) DeviceCategory::AUDIO_VIDEO
 * 3) DeviceCategory::OTHER
 * 4) DeviceCategory::UNKNOWN
 *
 * This rule is created by default and enforces Bluetooth devices falling into the above DeviceCategory to follow.
 * Any rule change for a certain DeviceCategory in the above list might need to refactor the Bluetooth CapabilityAgent.
 */
class BasicDeviceConnectionRule : public BluetoothDeviceConnectionRuleInterface {
public:
    /**
     * A factory method to create a new instance of @c BasicDeviceConnectionRule.
     * @return an instance of @c BasicConnectionRule.
     */
    static std::shared_ptr<BasicDeviceConnectionRule> create();

    /// @name BluetoothDeviceConnectionRuleInterface
    /// @{
    bool shouldExplicitlyConnect() override;
    bool shouldExplicitlyDisconnect() override;
    std::set<std::shared_ptr<BluetoothDeviceInterface>> devicesToDisconnect(
        std::map<DeviceCategory, std::set<std::shared_ptr<BluetoothDeviceInterface>>> connectedDevices) override;
    std::set<DeviceCategory> getDeviceCategories() override;
    std::set<std::string> getDependentProfiles() override;
    /// @}

private:
    /**
     * Constructor.
     */
    BasicDeviceConnectionRule();

    /// Represent the set of categories used in the rule.
    std::set<DeviceCategory> m_categories;

    /// Represent the set of profiles the categories rely on.
    std::set<std::string> m_profiles;
};

}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK
#endif  // ACSDKBLUETOOTH_BASICDEVICECONNECTIONRULE_H_
