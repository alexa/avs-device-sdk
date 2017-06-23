/*
 * DeprecatedLogger.h
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

#ifndef ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_LOGGING_DEPRECATED_LOGGER_H_
#define ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_LOGGING_DEPRECATED_LOGGER_H_

#include <string>

#include "AVSCommon/Utils/Logger/Logger.h"

// TODO:  ACSDK-179 Remove this file and migrate to new Logger

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {
namespace deprecated {

class Logger {
public:
    /**
     * A starting point for a log function.
     * @param The message to be logged.
     */
    static void log(const std::string &msg);
};

} // namespace deprecated
} // namespace logger
} // namespace utils
} // namespace avsCommon
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_LOGGING_DEPRECATED_LOGGER_H_
