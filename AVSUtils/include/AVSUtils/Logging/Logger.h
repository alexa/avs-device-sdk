/*
* Logger.h
*
* Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSUTILS_INCLUDE_AVSUTILS_LOGGING_LOGGER_H_
#define ALEXA_CLIENT_SDK_AVSUTILS_INCLUDE_AVSUTILS_LOGGING_LOGGER_H_

#include <string>
#include <mutex>

// TODO:  ACSDK-179 Remove this file and migrate to new Logger

#ifdef DEBUG

/**
 * Emit a debug level log line.
 *
 * @param expression An expression that results in some object with a c_str() method that returns the string to log.
 */
#define ACSDK_DEBUG(expression)                                         \
    do {                                                                \
        ::alexaClientSDK::avsUtils::Logger::log(expression.c_str());    \
    } while (false)

#else

/**
 * Stifle a debug level log line.
 *
 * @param expression An expression that results in some object with a c_str() method that returns the string to log.
 */
#define ACSDK_DEBUG(expression)                                         \
    do {                                                                \
    } while (false)

#endif

/**
 * Emit an info level log line.
 *
 * @param expression An expression that results in some object with a c_str() method that returns the string to log.
 */
#define ACSDK_INFO(expression)                                          \
    do {                                                                \
        ::alexaClientSDK::avsUtils::Logger::log(expression.c_str());    \
    } while (false)

/**
 * Emit an warning level log line.
 *
 * @param expression An expression that results in some object with a c_str() method that returns the string to log.
 */
#define ACSDK_WARN(expression) ACSDK_INFO(expression)

/**
 * Emit an error level log line.
 *
 * @param expression An expression that results in some object with a c_str() method that returns the string to log.
 */
#define ACSDK_ERROR(expression) ACSDK_INFO(expression)


namespace alexaClientSDK {
namespace avsUtils {

class Logger {

public:

    /**
     * A starting point for a log function.
     * @param The message to be logged.
     */
    static void log(const std::string &msg);

private:

    /// our mutex to ensure logging behaves in a multithreaded context.
    static std::mutex m_mutex;
};

} // namespace avsUtils
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_AVSUTILS_INCLUDE_AVSUTILS_LOGGING_LOGGER_H_