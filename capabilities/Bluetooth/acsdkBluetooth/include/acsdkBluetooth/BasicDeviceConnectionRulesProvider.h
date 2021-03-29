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

#ifndef ACSDKBLUETOOTH_BASICDEVICECONNECTIONRULESPROVIDER_H_
#define ACSDKBLUETOOTH_BASICDEVICECONNECTIONRULESPROVIDER_H_

#include <memory>
#include <unordered_set>

#include <acsdkBluetoothInterfaces/BluetoothDeviceConnectionRulesProviderInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceConnectionRuleInterface.h>

namespace alexaClientSDK {
namespace acsdkBluetooth {

/**
 * This class provides the default @c BasicDeviceConnectionRule for the Bluetooth Capability Agent.
 *
 * If you wish to add custom rules, you should implement the @c BluetoothDeviceConnectionRulesProviderInterface and pass
 * that to the Bluetooth CA.
 */
class BasicDeviceConnectionRulesProvider
        : public acsdkBluetoothInterfaces::BluetoothDeviceConnectionRulesProviderInterface {
public:
    static std::shared_ptr<acsdkBluetoothInterfaces::BluetoothDeviceConnectionRulesProviderInterface>
    createBluetoothDeviceConnectionRulesProviderInterface();

    /// @name BluetoothDeviceConnectionRulesProviderInterface
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
    getRules() override;
    /// @}
private:
    /**
     * Constructor.
     * @param rules The set of @c BluetoothDeviceConnectionRuleInterfaces to provide.
     */
    explicit BasicDeviceConnectionRulesProvider(
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
            rules);

    /// The set of @c BluetoothDeviceConnectionRuleInterfaces to provide.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
        m_bluetoothDeviceConnectionRules;
};

}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK

#endif  // ACSDKBLUETOOTH_BASICDEVICECONNECTIONRULESPROVIDER_H_
