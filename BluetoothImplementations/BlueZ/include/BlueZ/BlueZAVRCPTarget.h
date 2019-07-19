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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZAVRCPTARGET_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZAVRCPTARGET_H_

#include <memory>
#include <mutex>

#include <AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPTargetInterface.h>
#include <AVSCommon/Utils/Bluetooth/SDPRecords.h>
#include "BlueZ/BlueZBluetoothDevice.h"
#include "BlueZ/BlueZUtils.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/// A BlueZ implementation of the @c AVRCPTargetInterface.
class BlueZAVRCPTarget : public avsCommon::sdkInterfaces::bluetooth::services::AVRCPTargetInterface {
public:
    /**
     * Creates a BlueZAVRCPTarget instance.
     *
     * @param device A pointer to an instance of a @c DBusProxy for an org.bluez.MediaControl1 interface.
     * @return An instance of AVRCPTarget if successful, else a nullptr.
     */
    static std::shared_ptr<BlueZAVRCPTarget> create(std::shared_ptr<DBusProxy> mediaControlProxy);

    /// @name BluetoothServiceInterface methods
    /// @{
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::SDPRecordInterface> getRecord() override;
    void setup() override;
    void cleanup() override;
    /// @}

    /// @name AVRCPTargetInterface methods
    /// @{
    bool play() override;
    bool pause() override;
    bool next() override;
    bool previous() override;
    /// @}

private:
    /// Constructor.
    BlueZAVRCPTarget(std::shared_ptr<DBusProxy> mediaControlProxy);

    /// The @c SDPRecord associated with this Service. We don't currently parse the version.
    std::shared_ptr<avsCommon::utils::bluetooth::AVRCPTargetRecord> m_record;

    /// Serialize commands.
    std::mutex m_cmdMutex;

    /// A proxy for the BlueZ MediaControl interface.
    std::shared_ptr<DBusProxy> m_mediaControlProxy;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZAVRCPTARGET_H_
