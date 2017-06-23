/*
 * Logger.cpp
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

#include <chrono>
#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Logger/ThreadMoniker.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/// Configuration key for log level
static const std::string CONFIG_KEY_LOG_LEVEL = "logLevel";

Logger::Logger(Level level) : m_level{level} {
}

void Logger::log(Level level, const LogEntry& entry) {
    if (shouldLog(level)) {
        emit(
            level,
            std::chrono::system_clock::now(),
            ThreadMoniker::getThisThreadMoniker().c_str(),
            entry.c_str());
    }
}

void Logger::init(const configuration::ConfigurationNode configuration) {
    std::string name;
    if (configuration.getString(CONFIG_KEY_LOG_LEVEL, &name)) {
        Level level = convertNameToLevel(name);
        if (level != Level::UNKNOWN) {
            setLevel(level);
        } else {
            // Log without ACSDK_* macros to avoid recursive invocation of constructor.
            log(Level::ERROR, LogEntry("Logger", "unknownLogLevel").d("name", name));
        }
    }
}

} // namespace logger
} // namespace avsCommon
} // namespace utils
} // namespace alexaClientSDK

