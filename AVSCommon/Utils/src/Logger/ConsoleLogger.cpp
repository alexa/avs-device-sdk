/*
 * ConsoleLogger.cpp
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

#include <cstdio>
#include <ctime>
#include <iostream>
#include <mutex>

#include "AVSCommon/Utils/Logger/ConsoleLogger.h"
#include "AVSCommon/Utils/Logger/LoggerUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/// Configuration key for DefaultLogger settings
static const std::string CONFIG_KEY_DEFAULT_LOGGER = "consoleLogger";

Logger& ConsoleLogger::instance() {
    static ConsoleLogger singleConsoletLogger;
    return singleConsoletLogger;
}

/// Mutex to serialize output of log lines to @c std::cout.
static std::mutex g_coutMutex;

void ConsoleLogger::emit(
    Level level,
    std::chrono::system_clock::time_point time,
    const char* threadMoniker,
    const char* text) {
    std::lock_guard<std::mutex> lock(g_coutMutex);
    std::cout << formatLogString(level, time, threadMoniker, text) << std::endl;
}

ConsoleLogger::ConsoleLogger() :
#ifdef DEBUG
        Logger(Level::DEBUG0)
#else
        Logger(Level::INFO)
#endif  // DEBUG
{
    init(configuration::ConfigurationNode::getRoot()[CONFIG_KEY_DEFAULT_LOGGER]);
}

Logger& getConsoleLogger() {
    return ConsoleLogger::instance();
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
