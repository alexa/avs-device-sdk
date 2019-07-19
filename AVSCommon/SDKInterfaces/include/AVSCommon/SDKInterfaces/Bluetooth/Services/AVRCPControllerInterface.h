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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_AVRCPCONTROLLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_AVRCPCONTROLLERINTERFACE_H_

#include "AVSCommon/SDKInterfaces/Bluetooth/Services/BluetoothServiceInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {
namespace services {

/// Used to implement an instance of AVRCPController (Audio/Video Remote Control Profile).
class AVRCPControllerInterface : public BluetoothServiceInterface {
public:
    /**
     * The Service UUID.
     * 110e is the legacy UUID used as both the identifier for the AVRCP Profile as well as the AVRCP Controller service
     * before v1.3. 110f is the UUID used for AVRCP Controller service in newer versions of AVRCP.
     * However, the 110e record must always be present, in later versions of AVRCP for backwards compabitibility.
     * We will use 110e as the identifying record.
     */
    static constexpr const char* UUID = "0000110e-0000-1000-8000-00805f9b34fb";

    /// The Service Name.
    static constexpr const char* NAME = "A/V_RemoteControl";

    /**
     * Destructor.
     */
    virtual ~AVRCPControllerInterface() = default;
};

}  // namespace services
}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  //  ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_AVRCPCONTROLLERINTERFACE_H_
