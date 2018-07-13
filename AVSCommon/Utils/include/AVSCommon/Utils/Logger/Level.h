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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LEVEL_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LEVEL_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/**
 * Enum used to specify the severity assigned to a log message.
 */
enum class Level {
    /// Most verbose debug log level. Compiled out when ACSDK_DEBUG_LOG_ENABLED is not defined.
    DEBUG9,

    /// Intermediate debug log level. Compiled out when ACSDK_DEBUG_LOG_ENABLED is not defined.
    DEBUG8,

    /// Intermediate debug log level. Compiled out when ACSDK_DEBUG_LOG_ENABLED is not defined.
    DEBUG7,

    /// Intermediate debug log level. Compiled out when ACSDK_DEBUG_LOG_ENABLED is not defined.
    DEBUG6,

    /// Intermediate debug log level. Compiled out when ACSDK_DEBUG_LOG_ENABLED is not defined.
    DEBUG5,

    /// Intermediate debug log level. Compiled out when ACSDK_DEBUG_LOG_ENABLED is not defined.
    DEBUG4,

    /// Intermediate debug log level. Compiled out when ACSDK_DEBUG_LOG_ENABLED is not defined.
    DEBUG3,

    /// Intermediate debug log level. Compiled out when ACSDK_DEBUG_LOG_ENABLED is not defined.
    DEBUG2,

    /// Intermediate debug log level. Compiled out when ACSDK_DEBUG_LOG_ENABLED is not defined.
    DEBUG1,

    /// Least verbose debug log level. Compiled out when ACSDK_DEBUG_LOG_ENABLED is not defined.
    DEBUG0,

    /// Logs of normal operations, to be used in release builds.
    INFO,

    /// Log of an event that may indicate a problem.
    WARN,

    /// Log of an event that indicates an error.
    ERROR,

    /// Log of a event that indicates an unrecoverable error.
    CRITICAL,

    /// Level used to disable all logging.
    NONE,

    /// An unknown severity level.
    UNKNOWN
};

/**
 * Get the name of a Level value.
 * @param level The Level to get the name of.
 * @return Returns the name of the Level. If the level is not recognized, returns "UNKNOWN".
 */
std::string convertLevelToName(Level level);

/**
 * Get a character corresponding to a Level value. The characters returned are unique per log level and are
 * intended to be used to minimize the space taken up by the level specifier in log lines.
 * @param level The Level for which to get the corresponding character.
 * @return The character corresponding to the level. If the @c level is not recognized, returns 'U'.
 */
char convertLevelToChar(Level level);

/**
 * Get the Level corresponding to a Level name.
 * @param name The name corresponding to the desired Level value.
 * @return The @c Level corresponding to the specified name. If the @c name is not recognized,
 * returns @c Level::UNKNOWN.
 */
Level convertNameToLevel(const std::string& name);

/**
 * Write a log severity @c Level value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param level The @c Level value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, Level level) {
    stream << convertLevelToName(level);
    return stream;
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LEVEL_H_
