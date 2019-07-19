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

#include <algorithm>
#include <chrono>
#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Logger/ThreadMoniker.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/// Configuration key for root level "logger" object.
static const std::string CONFIG_KEY_LOGGER = "logger";

/// Configuration key for "logLevel" values under "logger" and other per-module objects.
static const std::string CONFIG_KEY_LOG_LEVEL = "logLevel";

static constexpr auto AT_EXIT_THREAD_ID = "0";

Logger::Logger(Level level) : m_level{level} {
}

void Logger::log(Level level, const LogEntry& entry) {
    if (shouldLog(level)) {
        emit(level, std::chrono::system_clock::now(), ThreadMoniker::getThisThreadMoniker().c_str(), entry.c_str());
    }
}

void Logger::init(const configuration::ConfigurationNode configuration) {
    if (!initLogLevel(configuration)) {
        initLogLevel(configuration::ConfigurationNode::getRoot()[CONFIG_KEY_LOGGER]);
    }
}

bool Logger::initLogLevel(const configuration::ConfigurationNode configuration) {
    std::string name;
    if (configuration.getString(CONFIG_KEY_LOG_LEVEL, &name)) {
        Level level = convertNameToLevel(name);
        if (level != Level::UNKNOWN) {
            setLevel(level);
            return true;
        } else {
            // Log without ACSDK_* macros to avoid recursive invocation of constructor.
            log(Level::ERROR, LogEntry("Logger", "unknownLogLevel").d("name", name));
        }
    }
    return false;
}

void Logger::setLevel(Level level) {
    // notify observers of logLevel changes
    if (m_level != level) {
        m_level = level;
        notifyObserversOnLogLevelChanged();
    }
#ifndef ACSDK_DEBUG_LOG_ENABLED
    if (m_level <= Level::DEBUG0) {
        // Log without ACSDK_* macros to avoid recursive invocation of constructor.
        log(Level::WARN,
            LogEntry("Logger", "debugLogLevelSpecifiedWhenDebugLogsCompiledOut")
                .d("level", m_level)
                .m("\n"
                   "\nWARNING: By default DEBUG logs are compiled out of RELEASE builds."
                   "\nRebuild with the cmake parameter -DCMAKE_BUILD_TYPE=DEBUG to enable debug logs."
                   "\n"));
    }
#endif
}

void Logger::addLogLevelObserver(LogLevelObserverInterface* observer) {
    {
        std::lock_guard<std::mutex> lock(m_observersMutex);
        m_observers.push_back(observer);
    }

    // notify this observer of current logLevel right away
    observer->onLogLevelChanged(m_level);
}

void Logger::removeLogLevelObserver(LogLevelObserverInterface* observer) {
    std::lock_guard<std::mutex> lock(m_observersMutex);

    m_observers.erase(std::remove(m_observers.begin(), m_observers.end(), observer), m_observers.end());
}

void Logger::notifyObserversOnLogLevelChanged() {
    std::vector<LogLevelObserverInterface*> observersCopy;

    // copy the vector first with the lock
    {
        std::lock_guard<std::mutex> lock(m_observersMutex);
        observersCopy = m_observers;
    }

    // call the callbacks
    for (auto observer : observersCopy) {
        observer->onLogLevelChanged(m_level);
    }
}

void Logger::logAtExit(Level level, const LogEntry& entry) {
    if (shouldLog(level)) {
        emit(level, std::chrono::system_clock::now(), AT_EXIT_THREAD_ID, entry.c_str());
    }
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
