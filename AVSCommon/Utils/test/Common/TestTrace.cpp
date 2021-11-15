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

#include "AVSCommon/Utils/Logger/TestTrace.h"
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <gtest/gtest.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

using namespace std;

void TestTrace::log(const string& message) {
    ACSDK_DEBUG(LogEntry(m_testCase, m_testName).m(message));
}

TestTrace::TestTrace() : m_testName{"UnknownTest"}, m_testCase{"unknownTestCase"} {
    auto gtestPtr = testing::UnitTest::GetInstance();
    if (gtestPtr) {
        auto testInfoPtr = gtestPtr->current_test_info();
        if (testInfoPtr) {
            m_testName = testInfoPtr->name();
            m_testCase = testInfoPtr->test_case_name();
        }
    }
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
