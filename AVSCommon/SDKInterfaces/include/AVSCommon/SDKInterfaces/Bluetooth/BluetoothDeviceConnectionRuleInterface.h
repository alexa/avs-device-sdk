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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHDEVICECONNECTIONRULEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHDEVICECONNECTIONRULEINTERFACE_H_

#include <map>
#include <set>

#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceInterface.h>
#include <AVSCommon/Utils/Bluetooth/DeviceCategory.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {

/**
 * This interface defines the connection rule the Bluetooth device needs to follow.
 */
class BluetoothDeviceConnectionRuleInterface {
public:
    /**
     * Destructor.
     */
    virtual ~BluetoothDeviceConnectionRuleInterface() = default;

    /**
     * The rule to explicitly connect the Bluetooth device after pair.
     *
     * @return true if the caller needs to handle connect logic.
     */
    virtual bool shouldExplicitlyConnect() = 0;

    /**
     * The rule to explicitly disconnect the Bluetooth device before unpair.
     *
     * @return true if the caller needs to handle disconnect logic.
     */
    virtual bool shouldExplicitlyDisconnect() = 0;

    /**
     * The rule to get a set of Bluetooth devices needed to disconnect when the Bluetooth device connects.
     *
     * @param connectedDevices the current connected devices.
     * @return the set of Bluetooth devices needed to disconnect.
     */
    virtual std::set<std::shared_ptr<BluetoothDeviceInterface>> devicesToDisconnect(
        std::map<DeviceCategory, std::set<std::shared_ptr<BluetoothDeviceInterface>>> connectedDevices) = 0;

    /**
     * Get the set of device categories using the connection rule.
     *
     * @return The set of @c DeviceCategory of the connection rule.
     */
    virtual std::set<DeviceCategory> getDeviceCategories() = 0;

    /**
     * Get the set of profile uuids which support those device categories defined in the connection rule.
     *
     * @return The set of profile uuids.
     */
    virtual std::set<std::string> getDependentProfiles() = 0;
};

}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_BLUETOOTHDEVICECONNECTIONRULEINTERFACE_H_
