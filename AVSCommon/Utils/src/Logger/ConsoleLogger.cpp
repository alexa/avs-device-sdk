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

#include <cstdio>
#include <ctime>
#include <iostream>
#include <mutex>

#include "AVSCommon/Utils/Logger/ConsoleLogger.h"
#include "AVSCommon/Utils/Logger/LoggerUtils.h"
#include "AVSCommon/Utils/Logger/ThreadMoniker.h"
#include "AVSCommon/Utils/SDKVersion.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/// Configuration key for DefaultLogger settings
static const std::string CONFIG_KEY_DEFAULT_LOGGER = "consoleLogger";

std::shared_ptr<Logger> ConsoleLogger::instance() {
    static std::shared_ptr<Logger> singleConsoletLogger = std::shared_ptr<ConsoleLogger>(new ConsoleLogger);
    return singleConsoletLogger;
}

void ConsoleLogger::emit(
    Level level,
    std::chrono::system_clock::time_point time,
    const char* threadMoniker,
    const char* text) {
    std::lock_guard<std::mutex> lock(m_coutMutex);
    std::cout << m_logFormatter.format(level, time, threadMoniker, text) << std::endl;
}

ConsoleLogger::ConsoleLogger() : Logger(Level::UNKNOWN) {
#ifdef DEBUG
    setLevel(Level::DEBUG0);
#else
    setLevel(Level::INFO);
#endif  // DEBUG
    init(configuration::ConfigurationNode::getRoot()[CONFIG_KEY_DEFAULT_LOGGER]);
    std::string currentVersionLogEntry("sdkVersion: " + avsCommon::utils::sdkVersion::getCurrentVersion());
    emit(
        alexaClientSDK::avsCommon::utils::logger::Level::INFO,
        std::chrono::system_clock::now(),
        ThreadMoniker::getThisThreadMoniker().c_str(),
        currentVersionLogEntry.c_str());
}

std::shared_ptr<Logger> getConsoleLogger() {
    return ConsoleLogger::instance();
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
