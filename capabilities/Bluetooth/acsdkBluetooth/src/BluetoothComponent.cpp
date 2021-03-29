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

#include <acsdkBluetoothInterfaces/BluetoothNotifierInterface.h>
#include <acsdkManufactory/ComponentAccumulator.h>

#include "acsdkBluetooth/Bluetooth.h"
#include "acsdkBluetooth/BluetoothComponent.h"
#include "acsdkBluetooth/BluetoothNotifier.h"

namespace alexaClientSDK {
namespace acsdkBluetooth {

BluetoothComponent getComponent() {
    return acsdkManufactory::ComponentAccumulator<>()
#ifdef BLUETOOTH_ENABLED
        .addRetainedFactory(BluetoothNotifier::createBluetoothNotifierInterface)
        .addRetainedFactory(BluetoothMediaInputTransformer::create)
        .addRequiredFactory(Bluetooth::createBluetoothCapabilityAgent)
#else
        .addInstance(std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface>(nullptr))
#endif
        ;
}

}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK
