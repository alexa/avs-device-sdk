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

#ifndef ACSDKDEVICESETUPINTERFACES_DEVICESETUPINTERFACE_H_
#define ACSDKDEVICESETUPINTERFACES_DEVICESETUPINTERFACE_H_

#include <future>
#include <memory>
#include <ostream>
#include <string>

namespace alexaClientSDK {
namespace acsdkDeviceSetupInterfaces {

/// Indicates if setup was assisted by another device or application.
enum class AssistedSetup {
    /// Setup occurred only on the device.
    NONE,
    /// Setup occurred with assistance from the companion app.
    ALEXA_COMPANION_APPLICATION
};

/**
 * Converts the @c AssistedSetup enum to a string.
 *
 * @param assistedSetup The @c AssistedSetup to convert.
 * @return A string representation of the @c AssistedSetup.
 */
inline std::string assistedSetupToString(AssistedSetup assistedSetup) {
    switch (assistedSetup) {
        case AssistedSetup::NONE:
            return "NONE";
        case AssistedSetup::ALEXA_COMPANION_APPLICATION:
            return "ALEXA_COMPANION_APPLICATION";
    }

    return "UNKNOWN";
}

/**
 * Overload for the @c AssistedSetup enum. This will write the @c AssistedSetup as a string to the provided stream.
 *
 * @param stream An ostream to send the @c AssistedSetup as a string.
 * @param assisstedSetup The @c AssistedSetup to convert.
 * @return The stream.
 */
inline std::ostream& operator<<(std::ostream& stream, AssistedSetup assistedSetup) {
    return stream << assistedSetupToString(assistedSetup);
}

/// Satisfies the AVS DeviceSetup Interface.
class DeviceSetupInterface {
public:
    /// Destructor.
    virtual ~DeviceSetupInterface() = default;

    /**
     * Sends the DeviceSetup.SetupCompleted event to the cloud. Retry logic and guarantee of delivery are not
     * expected. There may be a long roundtrip of the event, so it is recommended that clients wait with a timeout on
     * the future.
     *
     * @param assistedSetup Indicates what type of assistance was used.
     * @return A future indicating @c true if the event was sent successfully, @c false otherwise.
     */
    virtual std::future<bool> sendDeviceSetupComplete(AssistedSetup assistedSetup) = 0;
};

}  // namespace acsdkDeviceSetupInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKDEVICESETUPINTERFACES_DEVICESETUPINTERFACE_H_
