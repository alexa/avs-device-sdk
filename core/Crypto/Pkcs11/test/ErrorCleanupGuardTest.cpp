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

#include <gtest/gtest.h>

#include <acsdk/Pkcs11/private/ErrorCleanupGuard.h>

namespace alexaClientSDK {
namespace pkcs11 {
namespace test {

using namespace ::testing;

TEST(ErrorCleanupGuardTest, test_executeOnFailure) {
    bool successFlag = false;
    bool executed = false;
    {
        ErrorCleanupGuard guard(successFlag, [&]() -> void { executed = true; });
        ASSERT_FALSE(executed);
    }
    ASSERT_TRUE(executed);
}

TEST(ErrorCleanupGuardTest, test_executeOnSuccess) {
    bool successFlag = false;
    bool executed = false;
    {
        ErrorCleanupGuard guard(successFlag, [&]() -> void { executed = true; });
        ASSERT_FALSE(executed);
        successFlag = true;
    }
    ASSERT_FALSE(executed);
}

}  // namespace test
}  // namespace pkcs11
}  // namespace alexaClientSDK
