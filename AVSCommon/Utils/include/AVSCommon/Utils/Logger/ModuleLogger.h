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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_MODULELOGGER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_MODULELOGGER_H_

#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/**
 * @c Logger implementation providing per module configuration. Forwards logs to another @c Logger.
 */
class ModuleLogger
        : public Logger
        , protected LogLevelObserverInterface
        , protected SinkObserverInterface {
public:
    /**
     * Constructor.
     *
     * @param configKey The name of the root configuration key to inspect for a "logLevel" string value. That
     * string is used to specify the lowest log severity level that this @c ModuleLogger should emit.
     */
    ModuleLogger(const std::string& configKey);

    void setLevel(Level level) override;

    void emit(Level level, std::chrono::system_clock::time_point time, const char* threadId, const char* text) override;

private:
    void onLogLevelChanged(Level level) override;

    void onSinkChanged(const std::shared_ptr<Logger>& sink) override;

    /**
     * Combine @c m_moduleLogLevel and @c m_sinkLogLevel to determine the appropriate value for m_logLevel.
     */
    void updateLogLevel();

    /// Log level specified for this module logger.
    Level m_moduleLogLevel;

    /// Log level specified for the sink to forward logs to.
    Level m_sinkLogLevel;

protected:
    /// The @c Logger to forward logs to.
    std::shared_ptr<Logger> m_sink;
};

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_MODULELOGGER_H_
