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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_A2DPSOURCEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_A2DPSOURCEINTERFACE_H_

#include <memory>

#include "AVSCommon/SDKInterfaces/Bluetooth/Services/BluetoothServiceInterface.h"
#include <AVSCommon/Utils/Bluetooth/FormattedAudioStreamAdapter.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {
namespace services {

/**
 * Interface to support A2DP streaming from bluetooth device to SDK
 */
class A2DPSourceInterface : public BluetoothServiceInterface {
public:
    /// The Service UUID.
    static constexpr const char* UUID = "0000110a-0000-1000-8000-00805f9b34fb";

    /// The Service Name.
    static constexpr const char* NAME = "AudioSource";

    /**
     * Returns the stream containing the decoded raw PCM data sent by the connected device.
     *
     * @return A shared_ptr to a @c FormattedAudioStreamAdapter object to be consumed.
     */
    virtual std::shared_ptr<avsCommon::utils::bluetooth::FormattedAudioStreamAdapter> getSourceStream() = 0;

    /**
     * Destructor.
     */
    virtual ~A2DPSourceInterface() = default;
};

}  // namespace services
}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  //  ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_A2DPSOURCEINTERFACE_H_
