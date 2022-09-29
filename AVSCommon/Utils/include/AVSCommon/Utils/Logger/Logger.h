/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGGER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGGER_H_

#include <atomic>
#include <chrono>
#include <mutex>
#include <sstream>
#include <vector>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

#include "AVSCommon/Utils/Logger/Level.h"
#include "AVSCommon/Utils/Logger/LogEntry.h"
#include "AVSCommon/Utils/Logger/LogLevelObserverInterface.h"
#include "AVSCommon/Utils/Logger/SinkObserverInterface.h"

/**
 * Inner part of ACSDK_STRINGIFY.  Turns an expression in to a string literal.
 *
 * @param expression The expression to turn in to a string literal.
 */
#define ACSDK_STRINGIFY_INNER(expression) #expression

/**
 * Turn a macro in to a string literal.
 *
 * @param macro The macro to expand and turn in to a string literal.
 */
#define ACSDK_STRINGIFY(macro) ACSDK_STRINGIFY_INNER(macro)

/**
 * Inner part of ACSDK_CONCATENATE.  Concatenate two expressions in to a token.
 *
 * @param lhs The expression to turn in to the left hand part of the token.
 * @param rhs The expression to turn in to the right hand part of the token.
 */
#define ACSDK_CONCATENATE_INNER(lhs, rhs) lhs##rhs

/**
 * Concatenate two macros in to a token.
 *
 * @param lhs The macro to turn in to the left hand part of the token.
 * @param rhs The macro to turn in to the right hand part of the token.
 */
#define ACSDK_CONCATENATE(lhs, rhs) ACSDK_CONCATENATE_INNER(lhs, rhs)

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/**
 * @c Logger provides an interface for capturing log entries as well as some core logging functionality.
 * This includes:
 * - Capturing the time, thread ID, and severity level to associate with each log entry.
 * - Accessors for a @c logLevel value that specifies the minimum severity level of a log entries that will be emitted.
 * - Initialization of logging parameters (so far just @c logLevel) from a @c ConfigurationNode.
 *
 * The @c Logger interface is not typically used directly.  Instead, calls to it are usually wrapped in
 * invocations of macros.  These macros provide a way to selectively compile out logging code.  They also
 * provide additional contextual information and direct logs to the appropriate @c Logger instance.
 *
 * Typically, each .cpp source file using @c Logger defines a constant string and a macro near the top of the file.
 * The constant string is typically named @c TAG.  It specifies the name of the @c source of log entries originating
 * from the file.  This is usually the name of the class that is implemented in the file.  The macro is typically
 * named @c LX.  It is used to create a @c LogEntry instance in-line with the expression that builds the text
 * that is to be logged.  The resulting @c LogEntry instance is passed to whatever instance of @c Logger is in use
 * in the file.  @c LX bakes in @c TAG to associate the log entry with its source.  This macro also takes an
 * argument that specifies the name of the @c event that is being logged.  Both the constant string and event name
 * are passed to the @c LogEntry constructor.  Here is an example of the definitions that typically appear at
 * the start of a .cpp file:
 *
 *     #define TAG "MyClass"
 *     #define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)
 *
 * When an event is to be logged, a wrapper macro named @c ACSDK_<LEVEL> is invoked with an expression that starts
 * with an invocation of the @c LX macro.  The value of <LEVEL> is the name of the @c LogLevel value to associate
 * with the @c LogEntry.  Here is an example of a very simple log line that logs a "somethingHappened" event from
 * the source @c TAG with a severity level of @c INFO.
 *
 *     ACSDK_INFO(LX("somethingHappened"));
 *
 * The @c LogEntry class has a builder style method that provides a convenient way to parameterize the event with
 * named properties.  This is done by adding zero or more @c LogEntry::d() method invocations after the
 * @c LX() expression.  The name of the property is always a string, and the value can be of any type for which
 * an @c operator<<(std::ostream&, <type>) has been defined. These pairs are appended to the accumulating log
 * string.  Here is an example log line with two parameters:
 *
 *     ACSDK_WARN(LX("weirdnessHappened").d("<param1Name>", "stringValue").d("<param2Name>", 1 + 1 + 1);
 *
 * The @c LogEntry class also has a rarely used @c m() method that can be used to add an optional free-form
 * string at the end of the @c LogEntry.  When present, this must be the last method appended to the expression.
 * Here is an example of a log line with one parameter and a free-form message:
 *
 *     ACSDK_WARN(LX("weirdnessHappened").d("<param1Name>", "stringValue").m("free form text at the end");
 *
 * The @c ACSDK_<LEVEL> macros allow logs to be selectively eliminated from the generated code.  By default
 * logs of severity @c DEBUG0 and above are included in @c DEBUG builds, and logs of severity
 * @c INFO and above are in included in non @c DEBUG builds.  These macros also perform an in-line @c logLevel
 * check before evaluating the @c LX() expression.  That allows much of the CPU overhead of compiled-in log
 * lines to be selectively bypassed at run-time if the @c Logger's log level is set to not emit them.
 *
 * Logging may also be configured on a per-module basis.  Modules are defined by defining
 * @c ACSDK_LOG_MODULE to a common name for all source files in a module.  This name specifies the name
 * of an object under @c configuration::ConfigurationNode::getRoot().  The named object contains
 * configuration parameters for the per-module logger.  So, for example, to set the @c logLevel of the
 * @c foo module to @c WARN, the JSON used to configure the SDK would look something like this:
 *
 *     {
 *         "foo" : {
 *             "logLevel" : "WARN"
 *         }
 *         [... other propeties]
 *     }
 *
 * The value of @c ACSDK_LOG_MODULE is also used to generate a global inline function that is used to access
 * the logger for that module.  The signature of this function takes the form:
 *
 *     Logger& get<module name>Logger()
 *
 * The value of @c ACSDK_LOG_MODULE is typically defined in the CMakeLists.txt for a module using the
 * @c add_definitions() function.  Here is an example defining the @c foo log module:
 *
 *     add_definitions("-DACSDK_LOG_MODULE=foo")
 *
 * For modules that does not have the @c ACSDK_LOG_MODULE definition, logs will be defaulted to use the
 * ConsoleLogger module Logger.
 *
 * All logs (module specific or not) are output to a @c Sink @c Logger.  By default, the @c Sink @c Logger
 * is @c ConsoleLogger.  The sink logger can be changed by calling initialize in @c LoggerSinkManager.
 */
class Logger {
public:
    /**
     * Logger constructor.
     *
     * @param level The lowest severity level of logs to be emitted by this Logger.
     */
    Logger(Level level);

    /// Destructor.
    virtual ~Logger() = default;

    /**
     * Set the lowest severity level to be output by this logger.
     *
     * @param level The lowest severity level to be output by this logger.
     */
    virtual void setLevel(Level level);

    /**
     * Return true of logs of a specified severity should be emitted by this Logger.
     *
     * @param[in] level The Level to check.
     * @return Returns true if logs of the specified Level should be emitted.
     */
    inline bool shouldLog(Level level) const;

    /**
     * Send a log entry to this Logger.
     *
     * @param[in] level The severity Level to associate with this log entry.
     * @param[in] entry Object used to build the text of this log entry.
     */
    void log(Level level, const LogEntry& entry);

    /**
     * Send a log entry to this Logger while program is exiting.
     *
     * Use this method if the code may be run while destroying a static object. This method should not rely in any
     * other static object.
     *
     * @note The user code should still ensure that the Logger object itself is valid.
     *
     * @param[in] level The severity Level to associate with this log entry.
     * @param[in] entry Object used to build the text of this log entry.
     */
    void logAtExit(Level level, const LogEntry& entry);

    /**
     * Emit a log entry. Default implementation is no-op.
     * NOTE: This method must be thread-safe.
     * NOTE: Delays in returning from this method may hold up calls to Logger::log().
     *
     * @param[in] level The severity Level of this log line.
     * @param[in] time The time that the event to log occurred.
     * @param[in] threadMoniker Moniker of the thread that generated the event.
     * @param[in] text The text of the entry to log.
     */
    virtual void emit(
        Level level,
        std::chrono::system_clock::time_point time,
        const char* threadMoniker,
        const char* text);

    /**
     * Add an observer to this object.
     *
     * @param An observer to this class, which will be notified when the logLevel changes.
     */
    void addLogLevelObserver(LogLevelObserverInterface* observer);

    /**
     * Remove an observer to this object.
     *
     * @param An observer to this class that will be removed from the notification of logLevel changes.
     */
    void removeLogLevelObserver(LogLevelObserverInterface* observer);

protected:
    /**
     * Initialize @c Logger parameters from the specified @c ConfigurationNode.
     *
     * @param configuration The @c ConfigurationNode to read @c Logger parameters from.
     */
    void init(const configuration::ConfigurationNode configuration);

    /// The lowest severity level of logs to be output by this Logger.
    std::atomic<Level> m_level;

private:
    /**
     * Initialize the log level from the specified @c ConfigurationNode.
     *
     * @param configuration The @c ConfigurationNode to read the log level from.
     * @return Whether the logLevel was applied.
     */
    bool initLogLevel(const configuration::ConfigurationNode configuration);

    /**
     * Notify the observers of a logLevel change.
     */
    void notifyObserversOnLogLevelChanged();

    /// Vector of observers that want to be notified of logLevel changes
    std::vector<LogLevelObserverInterface*> m_observers;

    /// This mutex guards access to m_observers
    std::mutex m_observersMutex;
};

bool Logger::shouldLog(Level level) const {
    return level >= m_level;
}

/**
 * Macro for building function name of the form get<type>Logger().
 *
 * @param type The type of @c Logger for which to build a get<type>Logger() function name.
 */
#define ACSDK_GET_LOGGER_FUNCTION_NAME(type) ACSDK_CONCATENATE(ACSDK_CONCATENATE(get, type), Logger)

/**
 * Get the @c Logger that logs should be sent to.
 *
 * @return The @c Logger that logs should be sent to.
 */
std::shared_ptr<Logger> getConsoleLogger();

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#ifndef ACSDK_LOG_MODULE
#define ACSDK_LOG_MODULE ConsoleLogger
#endif  // ACSDK_LOG_MODULE

#include "AVSCommon/Utils/Logger/ModuleLogger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/**
 * Macro to define the function that ACSDK_<LEVEL> macros will send logs to.
 * In this case @c ACSDK_LOG_MODULE was defined and logs will be sent to the @c Logger returned by
 * @c get<ACSDK_LOG_MODULE>Logger().
 */
#define ACSDK_GET_LOGGER_FUNCTION ACSDK_GET_LOGGER_FUNCTION_NAME(ACSDK_LOG_MODULE)

/**
 * Inline method to get the logger for the module specified by @c ACSDK_LOG_MODULE.
 */
inline std::shared_ptr<Logger> ACSDK_GET_LOGGER_FUNCTION() {
    static std::shared_ptr<ModuleLogger> moduleLogger =
        std::make_shared<ModuleLogger>(ACSDK_STRINGIFY(ACSDK_LOG_MODULE));
    return moduleLogger;
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

/**
 * @def ACSDK_LOG
 * @brief Common implementation for sending entries to the log.
 *
 * @note If @c ACSDK_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] level The log level to associate with the log line.
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#ifdef ACSDK_LOG_ENABLED
#define ACSDK_LOG(level, entry)                                                                      \
    do {                                                                                             \
        auto loggerInstance = alexaClientSDK::avsCommon::utils::logger::ACSDK_GET_LOGGER_FUNCTION(); \
        if (loggerInstance->shouldLog(level)) {                                                      \
            loggerInstance->log(level, entry);                                                       \
        }                                                                                            \
    } while (false)
#else  // ACSDK_LOG_ENABLED
#define ACSDK_LOG(level, entry) \
    do {                        \
        (void)sizeof(level);    \
        (void)sizeof(entry);    \
    } while (false)
#endif  // ACSDK_LOG_ENABLED

/**
 * @def ACSDK_DEBUG_LOG
 * @brief Common implementation for sending debug entries to the log.
 *
 * @note If @c ACSDK_DEBUG_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] level The log level to associate with the log line.
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */

#ifdef ACSDK_DEBUG_LOG_ENABLED
#define ACSDK_DEBUG_LOG(level, entry) ACSDK_LOG((level), (entry))
#else  // ACSDK_DEBUG_LOG_ENABLED
#define ACSDK_DEBUG_LOG(level, entry) \
    do {                              \
        (void)sizeof(level);          \
        (void)sizeof(entry);          \
    } while (false)
#endif  // ACSDK_DEBUG_LOG_ENABLED

/**
 * Send a DEBUG9 severity log line.
 *
 * @note If @c ACSDK_DEBUG_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_DEBUG9(entry) ACSDK_DEBUG_LOG(alexaClientSDK::avsCommon::utils::logger::Level::DEBUG9, entry)

/**
 * Send a DEBUG8 severity log line.
 *
 * @note If @c ACSDK_DEBUG_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_DEBUG8(entry) ACSDK_DEBUG_LOG(alexaClientSDK::avsCommon::utils::logger::Level::DEBUG8, entry)

/**
 * Send a DEBUG7 severity log line.
 *
 * @note If @c ACSDK_DEBUG_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_DEBUG7(entry) ACSDK_DEBUG_LOG(alexaClientSDK::avsCommon::utils::logger::Level::DEBUG7, entry)

/**
 * Send a DEBUG6 severity log line.
 *
 * @note If @c ACSDK_DEBUG_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_DEBUG6(entry) ACSDK_DEBUG_LOG(alexaClientSDK::avsCommon::utils::logger::Level::DEBUG6, entry)

/**
 * Send a DEBUG5 severity log line.
 *
 * @note If @c ACSDK_DEBUG_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_DEBUG5(entry) ACSDK_DEBUG_LOG(alexaClientSDK::avsCommon::utils::logger::Level::DEBUG5, entry)

/**
 * Send a DEBUG4 severity log line.
 *
 * @note If @c ACSDK_DEBUG_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_DEBUG4(entry) ACSDK_DEBUG_LOG(alexaClientSDK::avsCommon::utils::logger::Level::DEBUG4, entry)

/**
 * Send a DEBUG3 severity log line.
 *
 * @note If @c ACSDK_DEBUG_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_DEBUG3(entry) ACSDK_DEBUG_LOG(alexaClientSDK::avsCommon::utils::logger::Level::DEBUG3, entry)

/**
 * Send a DEBUG2 severity log line.
 *
 * @note If @c ACSDK_DEBUG_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_DEBUG2(entry) ACSDK_DEBUG_LOG(alexaClientSDK::avsCommon::utils::logger::Level::DEBUG2, entry)

/**
 * Send a DEBUG1 severity log line.
 *
 * @note If @c ACSDK_DEBUG_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_DEBUG1(entry) ACSDK_DEBUG_LOG(alexaClientSDK::avsCommon::utils::logger::Level::DEBUG1, entry)

/**
 * Send a DEBUG0 severity log line.
 *
 * @note If @c ACSDK_DEBUG_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_DEBUG0(entry) ACSDK_DEBUG_LOG(alexaClientSDK::avsCommon::utils::logger::Level::DEBUG0, entry)

/**
 * Send a log line at the default debug level (DEBUG0).
 *
 * @note If @c ACSDK_DEBUG_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_DEBUG(entry) ACSDK_DEBUG_LOG(alexaClientSDK::avsCommon::utils::logger::Level::DEBUG0, entry)

/**
 * Send a INFO severity log line.
 *
 * @note If @c ACSDK_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_INFO(entry) ACSDK_LOG(alexaClientSDK::avsCommon::utils::logger::Level::INFO, entry)

/**
 * Send a WARN severity log line.
 *
 * @note If @c ACSDK_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_WARN(entry) ACSDK_LOG(alexaClientSDK::avsCommon::utils::logger::Level::WARN, entry)

/**
 * Send a ERROR severity log line.
 *
 * @note If @c ACSDK_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_ERROR(entry) ACSDK_LOG(alexaClientSDK::avsCommon::utils::logger::Level::ERROR, entry)
/**
 * Send a CRITICAL severity log line.
 *
 * @note If @c ACSDK_LOG_ENABLED is set to OFF, then logging is disabled.
 *
 * @param[in] entry A constructed @a LogEntry object with the log message text.
 */
#define ACSDK_CRITICAL(entry) ACSDK_LOG(alexaClientSDK::avsCommon::utils::logger::Level::CRITICAL, entry)

#ifndef ACSDK_LOGS_KEEP_FUNC_MACRO
// In older Android releases __func__ is redefined as __PRETTY_FUNCTION__ making log messages difficult to read and
// analyse. We undefine this macro if it is defined. See Logger.cmake for details on how to keep the macro intact.
#undef __func__
#endif  // ACSDK_LOGS_KEEP_FUNC_MACRO

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGGER_H_
