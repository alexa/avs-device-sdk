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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_LOGGER_TESTTRACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_LOGGER_TESTTRACE_H_

#include <string>

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/**
 * Utility class that can be used to print logs in testcases.
 *
 * This class will print debug logs and it will include the test suite and test name as part of the logs.
 */
class TestTrace {
public:
    /**
     * Initialize object and logger as well.
     */
    TestTrace();

    /**
     * Log a message.
     *
     * @param message The message to be logged.
     */
    void log(const std::string& message);

    /**
     * Log a value.
     *
     * @tparam ValueType The type of the value (must be supported by LogEntry).
     * @param name A string used to identify the value.
     * @param value The value to be logged.
     */
    template <typename ValueType>
    void log(const std::string& name, const ValueType& value);

private:
    // The test name extracted from googletest framework.
    std::string m_testName;

    // The test case extracted from googletest framework.
    std::string m_testCase;
};

template <typename ValueType>
void TestTrace::log(const std::string& name, const ValueType& value) {
    ACSDK_DEBUG(LogEntry(m_testCase, m_testName).d(name, value));
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_LOGGER_TESTTRACE_H_
