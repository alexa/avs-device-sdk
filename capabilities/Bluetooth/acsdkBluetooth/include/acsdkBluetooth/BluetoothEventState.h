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

#ifndef ACSDKBLUETOOTH_BLUETOOTHEVENTSTATE_H_
#define ACSDKBLUETOOTH_BLUETOOTHEVENTSTATE_H_

#include <string>

#include <AVSCommon/AVS/Requester.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceInterface.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace acsdkBluetooth {
/**
 * A class represents a Bluetooth event needed to send to cloud.
 */
class BluetoothEventState {
public:
    /**
     * Getter for the @c DeviceState.
     *
     * @return The @c DeviceState of the Bluetooth Event.
     */
    avsCommon::sdkInterfaces::bluetooth::DeviceState getDeviceState() const;

    /**
     * Getter for the @c Requester.
     * @return The @c Requester of the Bluetooth Event.
     */
    avsCommon::utils::Optional<avsCommon::avs::Requester> getRequester() const;
    /**
     * Getter for the profile name.
     * @return The profile name of the Bluetooth Event.
     */
    avsCommon::utils::Optional<std::string> getProfileName() const;

    /**
     * Constructor
     * @param state The @c DeviceState.
     * @param requester The @c Requester.
     * @param profileName The profileName.
     */
    BluetoothEventState(
        avsCommon::sdkInterfaces::bluetooth::DeviceState state,
        avsCommon::utils::Optional<avsCommon::avs::Requester> requester,
        avsCommon::utils::Optional<std::string> profileName);

private:
    /// The @c DeviceState used to indicate the device state of the Bluetooth event.
    avsCommon::sdkInterfaces::bluetooth::DeviceState m_state;

    /// The @c Requester used to indicate the event requester of the Bluetooth event.
    avsCommon::utils::Optional<avsCommon::avs::Requester> m_requester;

    /// Used to indicate the profile name of the Bluetooth event.
    avsCommon::utils::Optional<std::string> m_profileName;
};

}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK
#endif  // ACSDKBLUETOOTH_BLUETOOTHEVENTSTATE_H_
