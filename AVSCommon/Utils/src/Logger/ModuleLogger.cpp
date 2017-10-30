/*
 * ModuleLogger.cpp
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

// ModuleLogger.h will be indirectly pulled in through Logger.h when ACSDK_LOG_MODULE is defined.
#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Logger/LoggerSinkManager.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

void ModuleLogger::emit(
    Level level,
    std::chrono::system_clock::time_point time,
    const char* threadId,
    const char* text) {
    if (shouldLog(level)) {
        m_sink->emit(level, time, threadId, text);
    }
}

void ModuleLogger::setLevel(Level level) {
    Logger::setLevel(level);

    /*
     * Once the logLevel of the ModuleLogger has been changed, it should no
     * longer use the logLevel in the m_sink, hence the flag is cleared here.
     */
    m_useSinkLogLevel = false;
}

void ModuleLogger::onLogLevelChanged(Level level) {
    if (m_useSinkLogLevel) {
        Logger::m_level = level;
    }
}

void ModuleLogger::onSinkChanged(const std::shared_ptr<Logger>& logger) {
    if (m_sink) {
        m_sink->removeLogLevelObserver(this);
    }
    m_sink = logger;
    m_sink->addLogLevelObserver(this);
}

ModuleLogger::ModuleLogger(const std::string& configKey) :
        Logger(Level::UNKNOWN),
        m_useSinkLogLevel(true),
        m_sink(nullptr) {
    /*
     * Note that m_useSinkLogLevel is set to true by default.  The idea is for
     * the ModuleLogger to use the same logLevel as its sink unless it's been
     * set specifically.
     */

    /*
     * By adding itself to the LoggerSinkManager, the LoggerSinkManager will
     * notify the ModuleLogger of the current sink logger via the
     * SinkObserverInterface callback.  And in the onSinkChanged callback,
     * upon adding itself as a logLevel observer, the sink will notify the
     * ModuleLogger the current logLevel via the onLogLevelChanged callback.
     */
    LoggerSinkManager::instance().addSinkObserver(this);

    init(configuration::ConfigurationNode::getRoot()[configKey]);
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
