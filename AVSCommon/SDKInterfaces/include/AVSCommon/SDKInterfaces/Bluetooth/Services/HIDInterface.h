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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_HIDINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_HIDINTERFACE_H_

#include "AVSCommon/SDKInterfaces/Bluetooth/Services/BluetoothServiceInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {
namespace services {

/**
 * Interface to support human interface device(such as keyboards, mics) profile.
 */
class HIDInterface : public BluetoothServiceInterface {
public:
    /// The Service UUID.
    static constexpr const char* UUID = "00001124-0000-1000-8000-00805f9b34fb";

    /// The Service Name.
    static constexpr const char* NAME = "HumanInterfaceDeviceService";
};

}  // namespace services
}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  //  ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_HIDINTERFACE_H_