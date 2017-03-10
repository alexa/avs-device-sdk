/*
 * ExampleLogger.h
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
#ifndef ALEXA_CLIENT_SDK_AVSUTILS_EXAMPLES_EXAMPLE_LOGGER_INCLUDE_EXAMPLE_LOGGER_EXAMPLE_LOGGER_H_
#define ALEXA_CLIENT_SDK_AVSUTILS_EXAMPLES_EXAMPLE_LOGGER_INCLUDE_EXAMPLE_LOGGER_EXAMPLE_LOGGER_H_

#include <iostream>
#include <mutex>
#include <string>

#include "AVSUtils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsUtils {
namespace logger {
namespace examples {

/**
 * ExampleLogger provides an example implementation of the Logger class.
 *
 * This class is intended to be used by in other Alexa Client SDK examples and integration tests.
 */
class ExampleLogger : public alexaClientSDK::avsUtils::logger::Logger {
public:
    /**
     * Construct an ExampleLogger instance.
     *
     * @param name A name to associate with log entries sent to this ExampleLogger.
     * @param level The lowest severity level of log entries to actually emit.
     * @param stream A stream to send logs to.  Defaults to @c std::cout.
     */
    ExampleLogger(const std::string &name, Level level, std::ostream& stream = std::cout);

protected:
    void emit(
            Level level,
            std::chrono::system_clock::time_point time,
            const char *threadId,
            const char *text) override;

private:
    /// The name to associate with log entries sent to this ExampleLogger.
    const std::string m_name;

    /// Mutex to serialize output of log entries.
    static std::mutex m_mutex;

    /// A stream to send log output to.
    std::ostream& m_stream;
};

} // examples
} // logger
} // avsUtils
} // alexaClientSDK

#endif // ALEXA_CLIENT_SDK_AVSUTILS_EXAMPLES_EXAMPLE_LOGGER_INCLUDE_EXAMPLE_LOGGER_EXAMPLE_LOGGER_H_
