/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AndroidUtilities/AndroidLogger.h>

#include <android/log.h>

static const char* TAG{"AlexaSampleApp"};

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {

using namespace avsCommon::utils::logger;

/**
 * Convert AlexaClientSDK log level to Android log level.
 *
 * @param level AlexaClientSDK log level to be converted.
 * @return Android log level equivalent to the SDK one.
 */
static android_LogPriority convertToAndroidLevel(Level level) {
    switch (level) {
        case Level::ERROR:
            return ANDROID_LOG_ERROR;
        case Level::CRITICAL:
            return ANDROID_LOG_FATAL;
        case Level::INFO:
            return ANDROID_LOG_INFO;
        case Level::WARN:
            return ANDROID_LOG_WARN;
        case Level::NONE:
            return ANDROID_LOG_SILENT;
        case Level::DEBUG0:
        case Level::DEBUG1:
        case Level::DEBUG2:
        case Level::DEBUG3:
        case Level::DEBUG4:
            return ANDROID_LOG_DEBUG;
        case Level::DEBUG5:
        case Level::DEBUG6:
        case Level::DEBUG7:
        case Level::DEBUG8:
        case Level::DEBUG9:
            return ANDROID_LOG_VERBOSE;
        case Level::UNKNOWN:
            return ANDROID_LOG_UNKNOWN;
    }
}

AndroidLogger::AndroidLogger(Level level) : Logger{level} {
}

void AndroidLogger::emit(
    Level level,
    std::chrono::system_clock::time_point time,
    const char* threadMoniker,
    const char* text) {
    __android_log_print(
        convertToAndroidLevel(level), TAG, "[%s] %c %s", threadMoniker, convertLevelToChar(level), text);
}

}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
