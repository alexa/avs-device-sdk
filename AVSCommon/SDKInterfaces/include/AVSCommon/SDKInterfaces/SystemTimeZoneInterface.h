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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SYSTEMTIMEZONEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SYSTEMTIMEZONEINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This interface is used by the device to set the system time zone.
 *
 * The @c setTimeZone function is responsible for validating and applying the new timezone value. The timezone value
 * should be in the Olson format defined <a href="https://www.iana.org/time-zones"> here. </a>
 *
 * The setTimezone function will be for AVS triggered changes as well as local changes.
 */
class SystemTimeZoneInterface {
public:
    /**
     * Set the system time zone to the given @c timeZone value.
     *
     * @param timeZone The new time zone to be used by the system.
     * @return @c true if it succeeds to set the new timezone; @c false otherwise.
     */
    virtual bool setTimezone(const std::string& timeZone) = 0;

    /**
     * Default destructor.
     */
    virtual ~SystemTimeZoneInterface() = default;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SYSTEMTIMEZONEINTERFACE_H_
