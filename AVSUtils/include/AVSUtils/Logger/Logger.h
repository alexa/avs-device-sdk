/*
 * Logger.h
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

#ifndef ALEXA_CLIENT_SDK_AVSUTILS_INCLUDE_AVSUTILS_LOGGER_LOGGER_H_
#define ALEXA_CLIENT_SDK_AVSUTILS_INCLUDE_AVSUTILS_LOGGER_LOGGER_H_

#include <chrono>
#include <sstream>
#include "AVSUtils/Logger/Level.h"
#include "AVSUtils/Logger/LogEntry.h"

namespace alexaClientSDK {
namespace avsUtils {
namespace logger {

/**
 * An object to send LogEntries to.  Logger pairs the received LogEntries with date, time, thread, and level
 * properties and forwards the result to an emit() method which is to be implemented by the consumer of this class.
 */
class Logger {
public:
    /**
     * Logger constructor.
     *
     * @param level The lowest severity level of logs to be emitted by this Logger.
     */
    Logger(Level level);

    /**
     * Return true of logs of a specified severity should be emitted by this Logger.
     *
     * @param level The Level to check.
     * @return Returns true if logs of the specified Level should be emitted.
     */
    inline bool shouldLog(Level level) const;

    /**
     * Send a log entry to this Logger.
     *
     * @param level The severity Level to associate with this log entry.
     * @param entry Object used to build the text of this log entry.
     */
    void log(Level level, const LogEntry& entry);

protected:
    /**
     * Emit a log entry.
     * NOTE: This method must be thread-safe.
     * NOTE: Delays in returning from this method may hold up calls to Logger::log().
     *
     * @param level The severity Level of this log line.
     * @param time The time that the event to log occurred.
     * @param threadId Id of the thread that generated the event, as a C string.
     * @param text The text of the entry to log.
     */
    virtual void emit(
            Level level,
            std::chrono::system_clock::time_point time,
            const char *threadId,
            const char *text) = 0;

private:
    /// The lowest severity level of logs to be output by this Logger.
    const Level m_level;
};

bool Logger::shouldLog(Level level) const {
    return level >= m_level;
}

} // namespace logger
} // namespace avsUtils
} // namespace alexaClientSDK

/**
 * Common implementation for sending entries to the log.
 *
 * @param loggerArg The Logger to send the log to.
 * @param level The log level to associate with the log line.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_LOG(loggerArg, level, entry)              \
    do {                                                \
        if (loggerArg && loggerArg->shouldLog(level)) { \
            loggerArg->log(level, entry);               \
        }                                               \
    } while (false)

#ifdef ACSDK_DEBUG_LOG_ENABLED

/**
 * Send a DEBUG9 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG9(loggerArg, entry) ACSDK_LOG(loggerArg, Level::DEBUG9, entry)

/**
 * Send a DEBUG8 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG8(loggerArg, entry) ACSDK_LOG(loggerArg, Level::DEBUG8, entry)

/**
 * Send a DEBUG7 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG7(loggerArg, entry) ACSDK_LOG(loggerArg, Level::DEBUG7, entry)

/**
 * Send a DEBUG6 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG6(loggerArg, entry) ACSDK_LOG(loggerArg, Level::DEBUG6, entry)

/**
 * Send a DEBUG5 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG5(loggerArg, entry) ACSDK_LOG(loggerArg, Level::DEBUG5, entry)

/**
 * Send a DEBUG4 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG4(loggerArg, entry) ACSDK_LOG(loggerArg, Level::DEBUG4, entry)

/**
 * Send a DEBUG3 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG3(loggerArg, entry) ACSDK_LOG(loggerArg, Level::DEBUG3, entry)

/**
 * Send a DEBUG2 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG2(loggerArg, entry) ACSDK_LOG(loggerArg, Level::DEBUG2, entry)

/**
 * Send a DEBUG1 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG1(loggerArg, entry) ACSDK_LOG(loggerArg, Level::DEBUG1, entry)

/**
 * Send a DEBUG0 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG0(loggerArg, entry) ACSDK_LOG(loggerArg, Level::DEBUG0, entry)

#else // ACSDK_DEBUG_LOG_ENABLED

/**
 * Compile out a DEBUG9 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG9(loggerArg, entry)

/**
 * Compile out a DEBUG8 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG8(loggerArg, entry)

/**
 * Compile out a DEBUG7 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG7(loggerArg, entry)

/**
 * Compile out a DEBUG6 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG6(loggerArg, entry)

/**
 * Compile out a DEBUG5 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG5(loggerArg, entry)

/**
 * Compile out a DEBUG4 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG4(loggerArg, entry)

/**
 * Compile out a DEBUG3 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG3(loggerArg, entry)

/**
 * Compile out a DEBUG2 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG2(loggerArg, entry)

/**
 * Compile out a DEBUG1 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG1(loggerArg, entry)

/**
 * Compile out a DEBUG0 severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_DEBUG0(loggerArg, entry)

#endif // ACSDK_DEBUG_LOG_ENABLED

/**
 * Send a INFO severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_INFO(loggerArg, entry) ACSDK_LOG(loggerArg, Level::INFO, entry)

/**
 * Send a WARN severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_WARN(loggerArg, entry) ACSDK_LOG(loggerArg, Level::WARN, entry)

/**
 * Send a ERROR severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_ERROR(loggerArg, entry) ACSDK_LOG(loggerArg, Level::ERROR, entry)
/**
 * Send a CRITICAL severity log line.
 *
 * @param loggerArg The Logger to send the line to.
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_CRITICAL(loggerArg, entry) ACSDK_LOG(loggerArg, Level::CRITICAL, entry)

#endif // ALEXA_CLIENT_SDK_AVSUTILS_INCLUDE_AVSUTILS_LOGGER_LOGGER_H_
