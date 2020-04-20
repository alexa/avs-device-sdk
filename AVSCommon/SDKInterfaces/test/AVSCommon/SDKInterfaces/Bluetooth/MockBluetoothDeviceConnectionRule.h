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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_MOCKBLUETOOTHDEVICECONNECTIONRULE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_MOCKBLUETOOTHDEVICECONNECTIONRULE_H_

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceConnectionRuleInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {
namespace test {

/**
 * Mock class that implements BluetoothDeviceConnectionRuleInterface.
 */
class MockBluetoothDeviceConnectionRule : public BluetoothDeviceConnectionRuleInterface {
public:
    MockBluetoothDeviceConnectionRule(std::set<DeviceCategory> categories, std::set<std::string> profiles);
    bool shouldExplicitlyConnect() override;
    bool shouldExplicitlyDisconnect() override;
    std::set<std::shared_ptr<BluetoothDeviceInterface>> devicesToDisconnect(
        std::map<DeviceCategory, std::set<std::shared_ptr<BluetoothDeviceInterface>>> connectedDevices) override;
    std::set<DeviceCategory> getDeviceCategories() override;
    std::set<std::string> getDependentProfiles() override;

    /**
     * Helper function to test shouldExplicitlyConnect() method.
     *
     * @param explicitlyConnect A bool indicating if a device needs to explicitly connect.
     */
    void setExplicitlyConnect(bool explicitlyConnect);

    /**
     * Helper function to test shouldExplicitlyDisconnect() method.
     *
     * @param explicitlyDisconnect A bool indicating if a device needs to explicitly connect.
     */
    void setExplicitlyDisconnect(bool explicitlyDisconnect);

    /**
     * Helper function to test devicesToDisconnect(Map<DeviceCategory, Set<BluetoothDeviceInterface>>) method.
     *
     * @param devices a set of devices needed to disconnect.
     */
    void setDevicesToDisconnect(std::set<std::shared_ptr<BluetoothDeviceInterface>> devices);

protected:
    std::set<DeviceCategory> m_categories;

    std::set<std::string> m_profiles;

    bool m_explicitlyConnect;

    bool m_explicitlyDisconnect;

    std::set<std::shared_ptr<BluetoothDeviceInterface>> m_disconnectedDevices;
};

MockBluetoothDeviceConnectionRule::MockBluetoothDeviceConnectionRule(
    std::set<DeviceCategory> categories,
    std::set<std::string> profiles) :
        m_categories{categories},
        m_profiles{profiles},
        m_explicitlyConnect{false},
        m_explicitlyDisconnect{false} {
}

std::set<DeviceCategory> MockBluetoothDeviceConnectionRule::getDeviceCategories() {
    return m_categories;
}

std::set<std::string> MockBluetoothDeviceConnectionRule::getDependentProfiles() {
    return m_profiles;
}

bool MockBluetoothDeviceConnectionRule::shouldExplicitlyConnect() {
    return m_explicitlyConnect;
}

bool MockBluetoothDeviceConnectionRule::shouldExplicitlyDisconnect() {
    return m_explicitlyDisconnect;
}

std::set<std::shared_ptr<BluetoothDeviceInterface>> MockBluetoothDeviceConnectionRule::devicesToDisconnect(
    std::map<DeviceCategory, std::set<std::shared_ptr<BluetoothDeviceInterface>>> connectedDevices) {
    return m_disconnectedDevices;
}

void MockBluetoothDeviceConnectionRule::setExplicitlyConnect(bool explicitlyConnect) {
    m_explicitlyConnect = explicitlyConnect;
}

void MockBluetoothDeviceConnectionRule::setExplicitlyDisconnect(bool explicitlyDisconnect) {
    m_explicitlyDisconnect = explicitlyDisconnect;
}

void MockBluetoothDeviceConnectionRule::setDevicesToDisconnect(
    std::set<std::shared_ptr<BluetoothDeviceInterface>> devices) {
    m_disconnectedDevices = devices;
}

}  // namespace test
}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_MOCKBLUETOOTHDEVICECONNECTIONRULE_H_