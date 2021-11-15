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

#ifndef ACSDKBLUETOOTHINTERFACES_BLUETOOTHLOCALINTERFACE_H_
#define ACSDKBLUETOOTHINTERFACES_BLUETOOTHLOCALINTERFACE_H_

namespace alexaClientSDK {
namespace acsdkBluetoothInterfaces {

/**
 * Interface to be implemented by class containing business logic for Bluetooth operations.
 */
class BluetoothLocalInterface {
public:
    /**
     * Destructor
     */
    virtual ~BluetoothLocalInterface() = default;

    /**
     * Puts the device into the desired discoverable mode.
     *
     * @param discoverable A bool indicating whether it should be discoverable.
     */
    virtual void setDiscoverableMode(bool discoverable) = 0;

    /**
     * Puts the device into the desired scan mode.
     *
     * @param scanning A bool indicating whether it should be scanning.
     */
    virtual void setScanMode(bool scanning) = 0;

    /**
     * Pair with the device matching the given MAC address.
     *
     * @param mac The MAC address associated with the device.
     */
    virtual void pair(const std::string& addr) = 0;

    /**
     * Unpair with the device matching the given MAC address.
     *
     * @param mac The MAC address associated with the device.
     */
    virtual void unpair(const std::string& addr) = 0;

    /**
     * Connect with the device matching the given MAC address.
     *
     * @param mac The MAC address associated with the device.
     */
    virtual void connect(const std::string& addr) = 0;

    /**
     * Disconnect from the device matching the given MAC address.
     *
     * @param mac The MAC address associated with the device.
     */
    virtual void disconnect(const std::string& addr) = 0;
};

}  // namespace acsdkBluetoothInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKBLUETOOTHINTERFACES_BLUETOOTHLOCALINTERFACE_H_
