/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SOFTWAREINFOSENDEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SOFTWAREINFOSENDEROBSERVERINTERFACE_H_

#include <cstdint>
#include <limits>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

namespace softwareInfo {

/// Type to use to communicate a firmware version.
typedef int32_t FirmwareVersion;

/// The invalid firmware version.
static const FirmwareVersion INVALID_FIRMWARE_VERSION = 0;

static const FirmwareVersion MAX_FIRMWARE_VERSION = std::numeric_limits<FirmwareVersion>::max();

/**
 * Determine whether a firmware version is valid.
 *
 * @param version The version to check.
 * @return Whether the specified firmware version is valid.
 */
inline bool isValidFirmwareVersion(FirmwareVersion version) {
    return version > 0;
}

};  // namespace softwareInfo

/**
 * This interface receives notifications from @c SoftwareInfoSender instances.
 */
class SoftwareInfoSenderObserverInterface {
public:
    virtual ~SoftwareInfoSenderObserverInterface() = default;

    /**
     * Notification that the firmware version has been accepted by AVS.
     *
     * @param The firmware version that was accepted.
     */
    virtual void onFirmwareVersionAccepted(softwareInfo::FirmwareVersion firmwareVersion) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SOFTWAREINFOSENDEROBSERVERINTERFACE_H_
