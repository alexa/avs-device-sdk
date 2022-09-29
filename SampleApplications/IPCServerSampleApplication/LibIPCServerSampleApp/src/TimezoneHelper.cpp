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

#include "IPCServerSampleApp/TimezoneHelper.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
/// String to identify log entries originating from this file.
#define TAG "TimezoneHelper"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<TimezoneHelper> TimezoneHelper::create(
    const std::shared_ptr<settings::DeviceSettingsManager>& deviceSettingsManager) {
    auto timezoneHelper = std::shared_ptr<TimezoneHelper>(new TimezoneHelper(deviceSettingsManager));
    if (!timezoneHelper) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Failed to create TimezoneHelper"));
        return nullptr;
    }

    deviceSettingsManager->addObserver<settings::TIMEZONE>(timezoneHelper);

    return timezoneHelper;
}

std::chrono::milliseconds TimezoneHelper::getDeviceTimezoneOffset() {
    return m_deviceTimeZoneOffset;
}

TimezoneHelper::TimezoneHelper(const std::shared_ptr<settings::DeviceSettingsManager>& deviceSettingsManager) {
    m_deviceTimeZoneOffset = calculateDeviceTimezoneOffset(
        deviceSettingsManager->getSetting<settings::DeviceSettingsIndex::TIMEZONE>()->get());
}

std::chrono::milliseconds TimezoneHelper::calculateDeviceTimezoneOffset(const std::string& timeZone) {
#ifdef _MSC_VER
    /// Important Note: The MSC version of this function only retrieves the device timezone offset, it does not use
    /// the provided timeZone parameter

    TIME_ZONE_INFORMATION TimeZoneInfo;
    GetTimeZoneInformation(&TimeZoneInfo);
    auto offsetInMinutes = -TimeZoneInfo.Bias - TimeZoneInfo.DaylightBias;
    ACSDK_DEBUG9(LX(__func__).m(std::to_string(offsetInMinutes)));
    return std::chrono::minutes(offsetInMinutes);
#else
    char* prevTZ = getenv("TZ");
    setenv("TZ", timeZone.c_str(), 1);
    time_t t = time(NULL);
    struct tm* structtm = localtime(&t);
    if (prevTZ) {
        setenv("TZ", prevTZ, 1);
    } else {
        unsetenv("TZ");
    }
    if (!structtm) {
        ACSDK_ERROR(LX(__func__).m("localtime returns NULL"));
        return std::chrono::milliseconds::zero();
    }
    return std::chrono::milliseconds(structtm->tm_gmtoff * 1000);
#endif
}

void TimezoneHelper::onSettingNotification(const std::string& value, settings::SettingNotifications) {
    m_deviceTimeZoneOffset = calculateDeviceTimezoneOffset(value);
}
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK