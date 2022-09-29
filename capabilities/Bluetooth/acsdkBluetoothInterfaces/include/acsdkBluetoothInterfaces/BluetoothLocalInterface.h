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
 * Contains functions corresponding to high-level Bluetooth operations on a device.
 * Relative to their connectivity-level counterparts the functions defined by this interface
 * abstract out various details, instead defaulting to certain values (determined by business logic)
 * or providing these values through internal states maintained by the implementing class.
 * Services invoking Bluetooth functionality through this interface should expect relevant
 * business logic (e.g. connection rules) around the call to be applied.
 *
 * For all API calls, the resulting BT state should be received directly from BT connectivity.
 *
 * Clients that wish to deviate from the "norm" for a device or expect singular changes in BT state
 * (e.g. connecting to a device without connection rules) should call BT connectivity APIs directly.
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
     * Default behavior will remove the device from discoverable and scanning modes on a successful
     * pairing attempt.  Refer to an endpoint's corresponding connection rule to determine if an
     * explicit connection attempt should be made afterwards.
     *
     * @param mac The MAC address associated with the device.
     */
    virtual void pair(const std::string& addr) = 0;

    /**
     * Unpair with the device matching the given MAC address.
     *
     * Refer to an endpoint's corresponding connection rule to determine if an explicit disconnect
     * should be made prior.
     *
     * @param mac The MAC address associated with the device.
     */
    virtual void unpair(const std::string& addr) = 0;

    /**
     * Connect with the device matching the given MAC address.
     *
     * Refer to an endpoint's corresponding connection rule to determine which device(s) to
     * disconnect.
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

    /**
     * Sets the pairing pin for the current pairing attempt.  PIN length can range from 4 to 16
     * alphanumeric characters, though most devices will only accept numeric characters in the PIN.
     * Expected call flow is:
     * pair() -> PIN request callback -> setPairingPin()
     *
     * @param addr The MAC address associated with the device
     * @param pin BT pairing pin
     */
    virtual void setPairingPin(const std::string& addr, const std::string& pin) = 0;
};

}  // namespace acsdkBluetoothInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKBLUETOOTHINTERFACES_BLUETOOTHLOCALINTERFACE_H_
