/*
 * Logger.cpp
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

#include "AVSCommon/Utils/Logger/DeprecatedLogger.h"

// TODO:  ACSDK-179 Remove this file and migrate to new Logger

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/// String to identify log entries originating from this file.
static const std::string TAG("?");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

void logger::deprecated::Logger::log(const std::string &msg) {
    ACSDK_INFO(LX("?").m(msg));
}

} // namespace avsCommon
} // namespace utils
} // namespace alexaClientSDK
