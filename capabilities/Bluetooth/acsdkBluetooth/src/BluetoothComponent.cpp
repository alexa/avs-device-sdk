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

std::shared_ptr<acsdkBluetoothInterfaces::BluetoothLocalInterface> createBluetooth(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothStorageInterface> bluetoothStorage,
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> deviceManager,
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus,
    std::shared_ptr<registrationManager::CustomerDataManagerInterface> customerDataManager,
    std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>
        audioPipelineFactory,
    acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::AudioFocusAnnotation,
        avsCommon::sdkInterfaces::FocusManagerInterface> audioFocusManager,
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> shutdownNotifier,
    acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface> endpointCapabilitiesRegistrar,
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothDeviceConnectionRulesProviderInterface> connectionRulesProvider,
    std::shared_ptr<BluetoothMediaInputTransformer> mediaInputTransformer,
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface> bluetoothNotifier) {
    return std::shared_ptr<acsdkBluetoothInterfaces::BluetoothLocalInterface>(Bluetooth::createBluetoothCapabilityAgent(
        contextManager,
        messageSender,
        exceptionEncounteredSender,
        bluetoothStorage,
        deviceManager,
        eventBus,
        customerDataManager,
        audioPipelineFactory,
        audioFocusManager,
        shutdownNotifier,
        endpointCapabilitiesRegistrar,
        connectionRulesProvider,
        mediaInputTransformer,
        bluetoothNotifier));
}

BluetoothComponent getComponent() {
    return acsdkManufactory::ComponentAccumulator<>()
#ifdef BLUETOOTH_ENABLED
        .addRetainedFactory(BluetoothNotifier::createBluetoothNotifierInterface)
        .addRetainedFactory(BluetoothMediaInputTransformer::create)
        .addRequiredFactory(createBluetooth)
#else
        .addInstance(std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface>(nullptr))
        .addInstance(std::shared_ptr<acsdkBluetoothInterfaces::BluetoothLocalInterface>(nullptr))
#endif
        ;
}

}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK
