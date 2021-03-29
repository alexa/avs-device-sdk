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

#ifndef ACSDKBLUETOOTHINTERFACES_BLUETOOTHDEVICECONNECTIONRULESPROVIDERINTERFACE_H_
#define ACSDKBLUETOOTHINTERFACES_BLUETOOTHDEVICECONNECTIONRULESPROVIDERINTERFACE_H_

#include <memory>

#include <acsdkBluetoothInterfaces/BluetoothDeviceObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceConnectionRuleInterface.h>

namespace alexaClientSDK {
namespace acsdkBluetoothInterfaces {

/**
 * This interface provides the @c BluetoothDeviceConnectionRuleInterface(s) used by the
 * Bluetooth Capability Agent to define the connection rules to follow for device types.
 */
class BluetoothDeviceConnectionRulesProviderInterface {
public:
    /**
     * Returns the unordered set of @c BluetoothDeviceConnectionRuleInterfaces added to this provider.
     *
     * @return The set of rules.
     */
    virtual std::unordered_set<
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
    getRules() = 0;

    // Destructor.
    virtual ~BluetoothDeviceConnectionRulesProviderInterface() = default;
};

}  // namespace acsdkBluetoothInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKBLUETOOTHINTERFACES_BLUETOOTHDEVICECONNECTIONRULESPROVIDERINTERFACE_H_
