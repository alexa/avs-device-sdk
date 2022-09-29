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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_TIMEZONEHELPER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_TIMEZONEHELPER_H_

#include <chrono>
#include <string>

#include <Settings/DeviceSettingsManager.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
/**
 * Utility class to calculate the timezone offset for the device
 */
class TimezoneHelper
        : public settings::SettingObserverInterface<settings::TimeZoneSetting>
        , public std::enable_shared_from_this<TimezoneHelper> {
public:
    /**
     * Create an instance of the @c TimezoneHelper
     * @param deviceSettingsManager Reference to the DeviceSettingsManager
     * @return instance of the TimezoneHelper
     */
    static std::shared_ptr<TimezoneHelper> create(
        const std::shared_ptr<settings::DeviceSettingsManager>& deviceSettingsManager);

    /**
     * Gets Device Time Zone Offset.
     */
    std::chrono::milliseconds getDeviceTimezoneOffset();

    /// @c SettingObserverInterface functions
    /// @{
    void onSettingNotification(const std::string& value, settings::SettingNotifications notification) override;
    /// @}

private:
    /**
     * Constructor
     * @param deviceSettingsManager Reference to the DeviceSettingsManager
     */
    TimezoneHelper(const std::shared_ptr<settings::DeviceSettingsManager>& deviceSettingsManager);

    /**
     * Calculates the offset
     * @param timeZone The timezone name
     * @return The offset in milliseconds, or 0 if the timezone could not be identified
     */
    std::chrono::milliseconds calculateDeviceTimezoneOffset(const std::string& timeZone);

    /**
     * The timezone offset
     */
    std::chrono::milliseconds m_deviceTimeZoneOffset;
};
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_TIMEZONEHELPER_H_
