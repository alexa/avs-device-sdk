/*
 * ExampleLogger.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "ExampleLogger/ExampleLogger.h"

namespace alexaClientSDK {
namespace avsUtils {
namespace logger {
namespace examples {

/// Format string for strftime() to produce date and time in the format "YYYY-MM-DD HH:MM:SS".
static const char* STRFTIME_FORMAT_STRING = "%Y-%m-%d %H:%M:%S";

/// Size of buffer needed to hold "YYYY-MM-DD HH:MM:SS" and a null terminator.
static const int DATE_AND_TIME_STRING_SIZE = 20;

/// Separator between date/time and millis.
static const char TIME_AND_MILLIS_SEPARATOR = '.';

// Format string for sprintf() to produce millis in the format "nnn".
static const char* MILLIS_FORMAT_STRING = "%03d";

/// Size of buffer needed to hold "nnn" (milliseconds value) and a null terminator
static const int MILLIS_STRING_SIZE = 4;

/// Separator string between milliseconds value and ExampleLogger name.
static const std::string MILLIS_AND_NAME_SEPARATOR = " [";

/// Separator between ExampleLogger name and thread ID in log lines.
static const char NAME_AND_THREAD_SEPARATOR = ':';

/// Separator between thread ID and level indicator in log lines.
static const std::string THREAD_AND_LEVEL_SEPARATOR = "] ";

/// Separator between level indicator and text in log lines.
static const char LEVEL_AND_TEXT_SEPARATOR = ' ';

/// End Of String (null terminator).
static const char EOS = '\0';

std::mutex ExampleLogger::m_mutex;

ExampleLogger::ExampleLogger(const std::string &name, Level level, std::ostream& stream) :
        // Parenthesis are used for initializing @c m_stream to work-around a bug in the C++ specification.  see:
        // http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1288
        Logger(level), m_name{name}, m_stream(stream) {
}

void ExampleLogger::emit(
        Level level,
        std::chrono::system_clock::time_point time,
        const char *threadId,
        const char *text) {

    char dateTimeString[DATE_AND_TIME_STRING_SIZE];
    auto timeAsTime_t = std::chrono::system_clock::to_time_t(time);
    auto timeAsTmPtr = std::gmtime(&timeAsTime_t);
    if (timeAsTmPtr) {
        strftime(dateTimeString, sizeof(dateTimeString), STRFTIME_FORMAT_STRING, timeAsTmPtr);
    } else {
        dateTimeString[0] = EOS;
    }
    auto timeMillisPart = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count() % 1000);
    char millisString[MILLIS_STRING_SIZE];
    sprintf(millisString, MILLIS_FORMAT_STRING, timeMillisPart);

    std::lock_guard<std::mutex> lock(m_mutex);

    m_stream
            << dateTimeString
            << TIME_AND_MILLIS_SEPARATOR
            << millisString
            << MILLIS_AND_NAME_SEPARATOR
            << m_name
            << NAME_AND_THREAD_SEPARATOR
            << threadId
            << THREAD_AND_LEVEL_SEPARATOR
            << convertLevelToChar(level)
            << LEVEL_AND_TEXT_SEPARATOR
            << text
            << std::endl;
}

} // examples
} // logger
} // avsUtils
} // // alexaClientSDK