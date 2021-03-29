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

#include "acsdkBluetooth/BasicDeviceConnectionRule.h"
#include "acsdkBluetooth/BasicDeviceConnectionRulesProvider.h"

namespace alexaClientSDK {
namespace acsdkBluetooth {

std::shared_ptr<acsdkBluetoothInterfaces::BluetoothDeviceConnectionRulesProviderInterface>
BasicDeviceConnectionRulesProvider::createBluetoothDeviceConnectionRulesProviderInterface() {
    std::unordered_set<
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
        enabledConnectionRules;

#ifdef BLUETOOTH_ENABLED
    enabledConnectionRules.insert(alexaClientSDK::acsdkBluetooth::BasicDeviceConnectionRule::create());
#endif

    return std::shared_ptr<acsdkBluetoothInterfaces::BluetoothDeviceConnectionRulesProviderInterface>(
        new BasicDeviceConnectionRulesProvider(enabledConnectionRules));
}

BasicDeviceConnectionRulesProvider::BasicDeviceConnectionRulesProvider(
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
        rules) {
    m_bluetoothDeviceConnectionRules = rules;
}

std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
BasicDeviceConnectionRulesProvider::getRules() {
    return m_bluetoothDeviceConnectionRules;
}

}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK
