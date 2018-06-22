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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGSTRINGFORMATTER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGSTRINGFORMATTER_H_

#include <chrono>

#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Timing/SafeCTimeAccess.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/**
 * A class used to format log strings.
 */
class LogStringFormatter {
public:
    LogStringFormatter();

    /**
     * Formats a log message into a printable string with other metadata regarding the log message.
     *
     * @param level The severity Level of this log line.
     * @param time The time that the event to log occurred.
     * @param threadMoniker Moniker of the thread that generated the event.
     * @param text The text of the entry to log.
     * @return The formatted string.
     */
    std::string format(
        Level level,
        std::chrono::system_clock::time_point time,
        const char* threadMoniker,
        const char* text);

private:
    std::shared_ptr<timing::SafeCTimeAccess> m_safeCTimeAccess;
};

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGSTRINGFORMATTER_H_
