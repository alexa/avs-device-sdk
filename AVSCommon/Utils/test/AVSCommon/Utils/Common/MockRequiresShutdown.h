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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_MOCKREQUIRESSHUTDOWN_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_MOCKREQUIRESSHUTDOWN_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace test {

/// Mock class that implements RequiresShutdown.
class MockRequiresShutdown : public RequiresShutdown {
public:
    MockRequiresShutdown(const std::string& name);
    MOCK_CONST_METHOD0(name, std::string());
    MOCK_METHOD0(shutdown, void());
    MOCK_METHOD0(isShutdown, bool());
    MOCK_METHOD0(doShutdown, void());
};

}  // namespace test
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_MOCKREQUIRESSHUTDOWN_H_
