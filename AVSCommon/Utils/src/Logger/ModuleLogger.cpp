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

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

void ModuleLogger::emit(
        Level level,
        std::chrono::system_clock::time_point time,
        const char *threadId,
        const char *text) {
    if (shouldLog(level)) {
        m_sink.emit(level, time, threadId, text);
    }
}

ModuleLogger::ModuleLogger(const std::string& configKey, Logger& sink) :
#ifdef DEBUG
        Logger(Level::DEBUG0),
#else
        Logger(Level::INFO),
#endif // DEBUG
        m_sink(sink) {
    init(configuration::ConfigurationNode::getRoot()[configKey]);
}

} // namespace logger
} // namespace avsCommon
} // namespace utils
} // namespace alexaClientSDK

