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

#ifndef ACSDKBLUETOOTHIMPLEMENTATION_BLUETOOTHIMPLEMENTATIONCOMPONENT_H_
#define ACSDKBLUETOOTHIMPLEMENTATION_BLUETOOTHIMPLEMENTATIONCOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceManagerInterface.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEventBus.h>

namespace alexaClientSDK {
namespace acsdkBluetoothImplementation {

/**
 * Manufactory Component definition for the BlueZ implementation of @c BluetoothDeviceManagerInterface.
 */
using BluetoothImplementationComponent = acsdkManufactory::Component<
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface>,
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus>>;
/**
 * Get the @c Manufactory component for creating an instance of @c BluetoothDeviceManagerInterface.
 *
 * @return The @c Manufactory component for creating an instance of @c BluetoothDeviceManagerInterface.
 */
BluetoothImplementationComponent getComponent();

}  // namespace acsdkBluetoothImplementation
}  // namespace alexaClientSDK

#endif  // ACSDKBLUETOOTHIMPLEMENTATION_BLUETOOTHIMPLEMENTATIONCOMPONENT_H_