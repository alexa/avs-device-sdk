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

#ifndef ACSDKBLUETOOTHINTERFACES_BLUETOOTHNOTIFIERINTERFACE_H_
#define ACSDKBLUETOOTHINTERFACES_BLUETOOTHNOTIFIERINTERFACE_H_

#include <memory>

#include <acsdk/NotifierInterfaces/NotifierInterface.h>

#include "acsdkBluetoothInterfaces/BluetoothDeviceObserverInterface.h"

namespace alexaClientSDK {
namespace acsdkBluetoothInterfaces {

/**
 * Interface for registering to observe Bluetooth notifications.
 */
using BluetoothNotifierInterface = notifierInterfaces::NotifierInterface<BluetoothDeviceObserverInterface>;

}  // namespace acsdkBluetoothInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKBLUETOOTHINTERFACES_BLUETOOTHNOTIFIERINTERFACE_H_
