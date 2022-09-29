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

#ifndef ACSDKBLUETOOTH_BLUETOOTHNOTIFIER_H_
#define ACSDKBLUETOOTH_BLUETOOTHNOTIFIER_H_

#include <memory>

#include <acsdkBluetoothInterfaces/BluetoothDeviceObserverInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothNotifierInterface.h>
#include <acsdk/Notifier/Notifier.h>

namespace alexaClientSDK {
namespace acsdkBluetooth {

/**
 * Relays notifications related to Bluetooth.
 */
class BluetoothNotifier : public notifier::Notifier<acsdkBluetoothInterfaces::BluetoothDeviceObserverInterface> {
public:
    /*
     * Factory method.
     * @return A new instance of @c BluetoothNotifierInterface.
     */
    static std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface> createBluetoothNotifierInterface();
};

}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK

#endif  // ACSDKBLUETOOTH_BLUETOOTHNOTIFIER_H_
