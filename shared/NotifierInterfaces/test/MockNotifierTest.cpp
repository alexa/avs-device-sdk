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

/// @file NotifierTest.cpp

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <acsdk/NotifierInterfaces/test/MockNotifier.h>

namespace alexaClientSDK {
namespace notifierInterfaces {
namespace test {

/**
 * @brief Test fixture for MockNotifier tests.
 *
 * @ingroup lib_acsdkNotifierTestLib
 */
class MockNotifierTest : public ::testing::Test {};

/**
 * Verify that mock is correct.
 */
TEST_F(MockNotifierTest, test_construction) {
    auto mock = std::make_shared<MockNotifier<void>>();
    ASSERT_TRUE(mock);
}

}  // namespace test
}  // namespace notifierInterfaces
}  // namespace alexaClientSDK
