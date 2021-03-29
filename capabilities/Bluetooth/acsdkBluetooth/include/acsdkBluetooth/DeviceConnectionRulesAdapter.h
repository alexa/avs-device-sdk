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

#ifndef ACSDKBLUETOOTH_DEVICECONNECTIONRULESADAPTER_H_
#define ACSDKBLUETOOTH_DEVICECONNECTIONRULESADAPTER_H_

#include <memory>
#include <unordered_set>

#include <acsdkBluetoothInterfaces/BluetoothDeviceConnectionRulesProviderInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceConnectionRuleInterface.h>

namespace alexaClientSDK {
namespace acsdkBluetooth {

/**
 * This class is provided to maintain backwards compatibility with applications that have not yet implemented a custom
 * @c BluetoothDeviceConnectionRulesProviderInterface (if the default @c BasicDeviceConnectionRulesProvider is
 * insufficient).
 *
 * This class adapts unordered sets of @c BluetoothDeviceConnectionRules into a @c
 * BluetoothDeviceConnectionRulesProviderInterface.
 */
class DeviceConnectionRulesAdapter : public acsdkBluetoothInterfaces::BluetoothDeviceConnectionRulesProviderInterface {
public:
    /**
     * Constructor.
     * @param rules The set of @c BluetoothDeviceConnectionRuleInterfaces to provide.
     */
    DeviceConnectionRulesAdapter(
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
            rules);

    /// @name BluetoothDeviceConnectionRulesProviderInterface
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
    getRules() override;
    /// @}

private:
    /// The set of @c BluetoothDeviceConnectionRuleInterfaces to provide.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
        m_bluetoothDeviceConnectionRules;
};

}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK

#endif  // ACSDKBLUETOOTH_DEVICECONNECTIONRULESADAPTER_H_
