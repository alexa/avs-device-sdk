/*
 * ExampleLoggerClient.cpp
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

#include <iostream>
#include "ExampleLogger/ExampleLogger.h"

using namespace alexaClientSDK::avsUtils::logger;

/// TAG to associate with log entries from this file.
static const std::string TAG = "main";

/**
 * Shorthand for the start of creating a LogEntry in this file.
 * @param event The name of the event generating this log entry.
 */
#define LX(event) LogEntry(TAG, event)

/**
 * Set the log level and then generate log lines for each log level.
 * @param level The log level setting to exercise the log levels against.
 */
void exerciseLogLevels(Level level) {
    auto log = std::make_shared<examples::ExampleLogger>("ExampleLoggerClient", level);
    ACSDK_DEBUG9(log, LX("Exercise DEBUG9   level"));
    ACSDK_DEBUG8(log, LX("Exercise DEBUG8   level").d("key1", 8));
    ACSDK_DEBUG7(log, LX("Exercise DEBUG7   level").d("key1", 7).d("key2", "\\\\hello\\\\ \\\\world\\\\!"));
    ACSDK_DEBUG6(log, LX("Exercise DEBUG6   level").d("key1", 6).d("key2", 6.0001).m("message"));
    ACSDK_DEBUG5(log, LX("Exercise DEBUG5   level").d("key1", 5).d("key2", "['\\' ',' ':' '=']"));
    ACSDK_DEBUG4(log, LX("Exercise DEBUG4   level").d("key1", "four"));
    ACSDK_DEBUG3(log, LX("Exercise DEBUG3   level").m("message after no metadata"));
    ACSDK_DEBUG3(log, LX("Exercise DEBUG2   level").d("key1", 1 + 1));
    ACSDK_DEBUG3(log, LX("Exercise DEBUG1   level").d("key1", 1.0));
    ACSDK_DEBUG0(log, LX("Exercise DEBUG0   level").d("key1", true));
    ACSDK_INFO(log, LX("Exercise INFO     level").d("key1", false));
    ACSDK_WARN(log, LX("Exercise WARN     level"));
    ACSDK_ERROR(log, LX("Exercise ERROR    level"));
    ACSDK_CRITICAL(log, LX("Exercise CRITICAL level").d("key1", "\"The time has come,\" the Walrus said..."));
}

/// Exercise setting the log level to each of the supported values.
void exerciseSetLogLevel() {
    exerciseLogLevels(Level::DEBUG9);
    exerciseLogLevels(Level::DEBUG8);
    exerciseLogLevels(Level::DEBUG7);
    exerciseLogLevels(Level::DEBUG6);
    exerciseLogLevels(Level::DEBUG5);
    exerciseLogLevels(Level::DEBUG4);
    exerciseLogLevels(Level::DEBUG3);
    exerciseLogLevels(Level::DEBUG2);
    exerciseLogLevels(Level::DEBUG1);
    exerciseLogLevels(Level::DEBUG0);
    exerciseLogLevels(Level::INFO);
    exerciseLogLevels(Level::WARN);
    exerciseLogLevels(Level::ERROR);
    exerciseLogLevels(Level::CRITICAL);
}

/**
 * ExampleLoggerClient's main().  Exercise all log levels and many logging fearures.
 * @param argc The number of command line arguments.
 * @param argv The command line arguments.
 * @return EXIT_SUCCESS if the example completes sucessfully, otherwise EXIT_FAILURE.
 */
int main(int argc, const char* argv[]) {
    exerciseSetLogLevel();
    std::cout << "ExampleLoggerClient() ran to completion." << std::endl;
    return EXIT_SUCCESS;
}