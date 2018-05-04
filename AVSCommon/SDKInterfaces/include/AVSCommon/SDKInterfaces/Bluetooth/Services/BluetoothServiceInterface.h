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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_BLUETOOTHSERVICEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_BLUETOOTHSERVICEINTERFACE_H_

#include <memory>

#include "AVSCommon/SDKInterfaces/Bluetooth/Services/SDPRecordInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {
namespace services {

/// Interface representing a BluetoothService.
class BluetoothServiceInterface {
public:
    /**
     * Returns an SDPRecord for the service.
     *
     * @return A @c SDPRecordInterface for the service.
     */
    virtual std::shared_ptr<SDPRecordInterface> getRecord() = 0;

    /// Destructor.
    virtual ~BluetoothServiceInterface() = default;

    /// Called for any necessary setup of the service.
    virtual void setup() = 0;

    /// Called for any necessary cleanup of the service.
    virtual void cleanup() = 0;
};

}  // namespace services
}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_BLUETOOTHSERVICEINTERFACE_H_
