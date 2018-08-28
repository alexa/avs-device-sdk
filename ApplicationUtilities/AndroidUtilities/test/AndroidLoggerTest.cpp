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

#include <gtest/gtest.h>

#include <AndroidUtilities/AndroidLogger.h>
#include <AVSCommon/Utils/Logger/Level.h>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {
namespace test {

using namespace avsCommon::utils::logger;

/// Test a valid log entry.
TEST(AndroidLoggerTest, sanityTestOk) {
    AndroidLogger logger{Level::DEBUG9};
    logger.emit(Level::ERROR, std::chrono::system_clock::now(), "A", "Hello");
}

/// Test a log entry with null threadMoniker.
TEST(AndroidLoggerTest, sanityTestNullThreadMoniker) {
    AndroidLogger logger{Level::DEBUG9};
    logger.emit(Level::INFO, std::chrono::system_clock::now(), nullptr, "log message");
}

/// Test a log entry with null text.
TEST(AndroidLoggerTest, sanityTestNullText) {
    AndroidLogger logger{Level::DEBUG9};
    logger.emit(Level::DEBUG0, std::chrono::system_clock::now(), "A", nullptr);
}

/// Test all levels.
TEST(AndroidLoggerTest, sanityTestLevels) {
    AndroidLogger logger{Level::DEBUG9};
    auto levels = {Level::UNKNOWN,
                   Level::NONE,
                   Level::CRITICAL,
                   Level::ERROR,
                   Level::WARN,
                   Level::INFO,
                   Level::DEBUG0,
                   Level::DEBUG1,
                   Level::DEBUG2,
                   Level::DEBUG3,
                   Level::DEBUG4,
                   Level::DEBUG5,
                   Level::DEBUG6,
                   Level::DEBUG7,
                   Level::DEBUG8,
                   Level::DEBUG9};
    for (auto level : levels) {
        logger.emit(level, std::chrono::system_clock::now(), "123", "message");
    }
}

}  // namespace test
}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
