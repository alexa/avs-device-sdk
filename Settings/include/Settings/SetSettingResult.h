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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETSETTINGRESULT_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETSETTINGRESULT_H_

#include <ostream>

/**
 * Enumerates possible return values for set settings functions.
 */
enum class SetSettingResult {
    /// The set value is already the current setting value and there is no change in progress.
    NO_CHANGE,
    /// The change request has been enqueued. There will be a follow up notification to inform the operation result.
    ENQUEUED,
    /// The request failed because there is already a value change in process and the setter requested change
    /// to abort if busy.
    BUSY,
    /// The request failed because the setting requested does not exist.
    UNAVAILABLE_SETTING,
    /// The request failed because the value requested is invalid.
    INVALID_VALUE,
    /// The request failed due to some internal error.
    INTERNAL_ERROR,
    /// The request is not supported.
    UNSUPPORTED_OPERATION
};

/**
 * Write a @c SetSettingResult value to the given stream.
 *
 * @param stream The stream to write the value to.
 * @param value The value to write to the stream as a string.
 * @return The stream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const SetSettingResult& value) {
    switch (value) {
        case SetSettingResult::NO_CHANGE:
            stream << "NO_CHANGE";
            return stream;
        case SetSettingResult::ENQUEUED:
            stream << "ENQUEUED";
            return stream;
        case SetSettingResult::BUSY:
            stream << "BUSY";
            return stream;
        case SetSettingResult::UNAVAILABLE_SETTING:
            stream << "UNAVAILABLE_SETTING";
            return stream;
        case SetSettingResult::INVALID_VALUE:
            stream << "INVALID_VALUE";
            return stream;
        case SetSettingResult::INTERNAL_ERROR:
            stream << "INTERNAL_ERROR";
            return stream;
        case SetSettingResult::UNSUPPORTED_OPERATION:
            stream << "UNSUPPORTED_OPERATION";
            return stream;
    }

    stream.setstate(std::ios_base::failbit);
    return stream;
}

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETSETTINGRESULT_H_
