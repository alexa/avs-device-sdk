/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZAVRCPCONTROLLER_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZAVRCPCONTROLLER_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPControllerInterface.h>
#include <AVSCommon/Utils/Bluetooth/SDPRecords.h>

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/**
 * BlueZ implementation of @c AVRCPControllerInterface interface
 */
class BlueZAVRCPController : public avsCommon::sdkInterfaces::bluetooth::services::AVRCPControllerInterface {
public:
    /**
     * Factory method to create a new instance of @c BlueZAVRCPController.
     *
     * @return A new instance of @c BlueZAVRCPController, nullptr if there was an error creating it.
     */
    static std::shared_ptr<BlueZAVRCPController> create();

    /// @name BluetoothServiceInterface functions.
    /// @{
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::SDPRecordInterface> getRecord() override;
    void setup() override;
    void cleanup() override;
    /// @}

private:
    /**
     * Private constructor
     */
    BlueZAVRCPController();

    /// Bluetooth service's SDP record containing the common service information.
    std::shared_ptr<avsCommon::utils::bluetooth::AVRCPControllerRecord> m_record;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZAVRCPCONTROLLER_H_
