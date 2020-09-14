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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_DEVICECATEGORY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_DEVICECATEGORY_H_

#include <ostream>
#include <string>

/**
 * Represent the paired device category.
 */
enum class DeviceCategory {
    // the paired device belongs to peripheral family
    REMOTE_CONTROL,
    // the paired device belongs to gadget family
    GADGET,
    // the paired device is a audio/video major class.
    AUDIO_VIDEO,
    // the paired device is a telephony device.
    PHONE,
    // the paired device falls into none of the above category.
    OTHER,
    // the paired device has no information.
    UNKNOWN
};

/**
 * Converts the @c DeviceCategory enum to a string.
 *
 * @param The @c DeviceCategory to convert.
 * @return A string representation of the @c DeviceCategory.
 */
inline std::string deviceCategoryToString(const DeviceCategory& category) {
    switch (category) {
        case DeviceCategory::REMOTE_CONTROL:
            return "REMOTE_CONTROL";
        case DeviceCategory::GADGET:
            return "GADGET";
        case DeviceCategory::AUDIO_VIDEO:
            return "AUDIO_VIDEO";
        case DeviceCategory::PHONE:
            return "PHONE";
        case DeviceCategory::OTHER:
            return "OTHER";
        case DeviceCategory::UNKNOWN:
            return "UNKNOWN";
    }

    return "UNKNOWN";
}

/**
 * Converts the string to @c DeviceCategory enum.
 *
 * @param A string representation of the @c DeviceCategory.
 * @return The @c DeviceCategory.
 */
inline DeviceCategory stringToDeviceCategory(const std::string& category) {
    if (category == "REMOTE_CONTROL") return DeviceCategory::REMOTE_CONTROL;
    if (category == "GADGET") return DeviceCategory::GADGET;
    if (category == "AUDIO_VIDEO") return DeviceCategory::AUDIO_VIDEO;
    if (category == "PHONE") return DeviceCategory::PHONE;
    if (category == "OTHER") return DeviceCategory::OTHER;
    if (category == "UNKNOWN") return DeviceCategory::UNKNOWN;

    return DeviceCategory::UNKNOWN;
}

/**
 * Overload for the @c DeviceCategory enum. This will write the @c DeviceCategory as a string to the provided stream.
 *
 * @param An ostream to send the @c DeviceCategory as a string.
 * @param The @c DeviceCategory to convert.
 * @return The stream.
 */
inline std::ostream& operator<<(std::ostream& stream, const DeviceCategory category) {
    return stream << deviceCategoryToString(category);
}

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_DEVICECATEGORY_H_