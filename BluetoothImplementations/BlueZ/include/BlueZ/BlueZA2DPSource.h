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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZA2DPSOURCE_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZA2DPSOURCE_H_

#include <AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSourceInterface.h>
#include <AVSCommon/Utils/Bluetooth/SDPRecords.h>
#include <AVSCommon/Utils/Bluetooth/FormattedAudioStreamAdapter.h>

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

class BlueZDeviceManager;

/**
 * BlueZ implementation of @c A2DPSourceInterface interface
 */
class BlueZA2DPSource : public avsCommon::sdkInterfaces::bluetooth::services::A2DPSourceInterface {
public:
    /**
     * Factory method to create a new instance of @c BlueZA2DPSource
     * @param deviceManager A @c BlueZDeviceManager this instance belongs to
     * @return A new instance of @c BlueZA2DPSource, nullptr if there was an error creating it.
     */
    static std::shared_ptr<BlueZA2DPSource> create(std::shared_ptr<BlueZDeviceManager> deviceManager);

    /// @name A2DPSourceInterface functions.
    /// @{

    std::shared_ptr<avsCommon::utils::bluetooth::FormattedAudioStreamAdapter> getSourceStream() override;

    /// @}

    /// @name BluetoothServiceInterface functions.
    /// @{

    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::services::SDPRecordInterface> getRecord() override;
    void setup() override;
    void cleanup() override;

    /// @}

private:
    /**
     * Private constructor
     * @param deviceManager A @c BlueZDeviceManager this instance belongs to
     */
    BlueZA2DPSource(std::shared_ptr<BlueZDeviceManager> deviceManager);

    /**
     * Bluetooth service's SDP record containing the common service information.
     */
    std::shared_ptr<avsCommon::utils::bluetooth::A2DPSourceRecord> m_record;

    /**
     * A @c BlueZDeviceManager this instance belongs to
     */
    std::shared_ptr<BlueZDeviceManager> m_deviceManager;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_BLUEZA2DPSOURCE_H_
