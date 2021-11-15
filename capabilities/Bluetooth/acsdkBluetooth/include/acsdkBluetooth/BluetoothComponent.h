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

#ifndef ACSDKBLUETOOTH_BLUETOOTHCOMPONENT_H_
#define ACSDKBLUETOOTH_BLUETOOTHCOMPONENT_H_

#include <memory>

#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothLocalInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothNotifierInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothDeviceConnectionRulesProviderInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothStorageInterface.h>
#include <acsdkManufactory/Annotated.h>
#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/AudioFocusAnnotation.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/PlaybackRouterInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>

namespace alexaClientSDK {
namespace acsdkBluetooth {

/**
 * Manufactory Component definition for the @c BluetoothNotifierInterface.
 */
using BluetoothComponent = acsdkManufactory::Component<
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothLocalInterface>,
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface>,
    acsdkManufactory::Import<
        std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>>,
    acsdkManufactory::Import<
        std::shared_ptr<acsdkBluetoothInterfaces::BluetoothDeviceConnectionRulesProviderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkBluetoothInterfaces::BluetoothStorageInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::AudioFocusAnnotation,
        avsCommon::sdkInterfaces::FocusManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus>>,
    acsdkManufactory::Import<std::shared_ptr<registrationManager::CustomerDataManagerInterface>>>;
/**
 * Get the @c Manufactory component for creating an instance of @c BluetoothNotifierInterface.
 *
 * @return The @c Manufactory component for creating an instance of @c BluetoothNotifierInterface.
 */
BluetoothComponent getComponent();

}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK

#endif  // ACSDKBLUETOOTH_BLUETOOTHCOMPONENT_H_
