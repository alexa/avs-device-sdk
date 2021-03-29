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

#include <acsdkManufactory/ComponentAccumulator.h>
#include <acsdkManufactory/FactorySequencer.h>
#include <BlueZ/BlueZBluetoothDeviceManager.h>
#include <BlueZ/PulseAudioBluetoothInitializer.h>

#include "acsdkBluetoothImplementation/BluetoothImplementationComponent.h"

namespace alexaClientSDK {
namespace acsdkBluetoothImplementation {

using namespace acsdkManufactory;
using namespace avsCommon::sdkInterfaces::bluetooth;
using namespace avsCommon::utils::bluetooth;
using namespace bluetoothImplementations::blueZ;

BluetoothImplementationComponent getComponent() {
    return ComponentAccumulator<>()
        .addRetainedFactory(BluetoothEventBus::createBluetoothEventBus)
        .addRetainedFactory(FactorySequencer<
                            std::shared_ptr<BluetoothDeviceManagerInterface>,
                            std::shared_ptr<BluetoothEventBus>,
                            std::shared_ptr<PulseAudioBluetoothInitializer>>::
                                get(BlueZBluetoothDeviceManager::createBluetoothDeviceManagerInterface))
#ifdef BLUETOOTH_BLUEZ_PULSEAUDIO_OVERRIDE_ENDPOINTS
        .addRetainedFactory(PulseAudioBluetoothInitializer::create)
#else
        .addInstance<std::shared_ptr<bluetoothImplementations::blueZ::PulseAudioBluetoothInitializer>>(nullptr)
#endif  // BLUETOOTH_BLUEZ_PULSEAUDIO_OVERRIDE_ENDPOINTS
        ;
}

}  // namespace acsdkBluetoothImplementation
}  // namespace alexaClientSDK
