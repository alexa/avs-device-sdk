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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_SDPRECORDINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_SDPRECORDINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {
namespace services {

/// Used to implement ServiceDiscoveryProtocol records. This allows identification of the service.
class SDPRecordInterface {
public:
    /**
     * The base UUID of a Bluetooth service. All service UUIDs are calculated from this.
     * Services have a short uuid assigned that is either uuid16 or uuid32. For example:
     *
     * AudioSource (A2DP Source)
     * Short UUID: 110a
     * UUID: 0000110a-0000-1000-8000-00805f9b34fb
     */
    static const std::string BASE_UUID() {
        return "00000000-0000-1000-8000-00805f9b34fb";
    }

    /// Destructor.
    virtual ~SDPRecordInterface() = default;

    /// @return The name of the service.
    virtual std::string getName() const = 0;

    /// @return The UUID of the service.
    virtual std::string getUuid() const = 0;

    /// @return The version of the service.
    virtual std::string getVersion() const = 0;
};

}  // namespace services
}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  //  ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_SDPRECORDINTERFACE_H_
