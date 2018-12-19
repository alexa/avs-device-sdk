/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <gmock/gmock.h>

#include "AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h"
#include "AVSCommon/AVS/HandlerAndPolicy.h"

using namespace ::testing;

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

/// Minimal DirectiveHandlerInterface implementation so we can generate instance pointers.
class TestDirectiveHandler : public sdkInterfaces::DirectiveHandlerInterface {
public:
    void handleDirectiveImmediately(std::shared_ptr<AVSDirective>) override {
    }
    void preHandleDirective(
        std::shared_ptr<AVSDirective>,
        std::unique_ptr<sdkInterfaces::DirectiveHandlerResultInterface>) override {
    }
    bool handleDirective(const std::string&) override {
        return false;
    }
    void cancelDirective(const std::string&) override {
    }
    void onDeregistered() override {
    }
    avs::DirectiveHandlerConfiguration getConfiguration() const override {
        // Not using an empty initializer list here to account for a GCC 4.9.2 regression
        return avs::DirectiveHandlerConfiguration();
    }
};

/**
 * HandlerAndPolicyTest
 */
class HandlerAndPolicyTest : public ::testing::Test {};

/**
 * Invoke default constructor.  Expect @c nameSpace and @c name properties are both empty strings.
 */
TEST_F(HandlerAndPolicyTest, testDefaultConstructor) {
    HandlerAndPolicy handlerAndPolicy;
    ASSERT_EQ(handlerAndPolicy.handler, nullptr);
    ASSERT_FALSE(handlerAndPolicy.policy.isValid());
}

/**
 * Invoke constructor with member values.  Expect properties match those provided to the constructor.
 */
TEST_F(HandlerAndPolicyTest, testConstructorWithValues) {
    auto handler = std::make_shared<TestDirectiveHandler>();
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    HandlerAndPolicy handlerAndPolicy(handler, neitherNonBlockingPolicy);
    ASSERT_EQ(handlerAndPolicy.handler, handler);
    ASSERT_EQ(handlerAndPolicy.policy, neitherNonBlockingPolicy);
}

/**
 * Create empty and non-empty HandlerAndPolicy instances. Expect that empty instances are interpreted as false and
 * non-empty values are interpreted as true.
 */
TEST_F(HandlerAndPolicyTest, testOperatorBool) {
    auto handler = std::make_shared<TestDirectiveHandler>();
    HandlerAndPolicy defaultHandlerAndPolicy;
    auto audioBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true);

    HandlerAndPolicy firstHalfEmpty(nullptr, audioBlockingPolicy);
    HandlerAndPolicy secondHalfEmpty(handler, BlockingPolicy());
    HandlerAndPolicy nonEmpty(handler, audioBlockingPolicy);
    ASSERT_FALSE(defaultHandlerAndPolicy);
    ASSERT_FALSE(firstHalfEmpty);
    ASSERT_FALSE(secondHalfEmpty);
    ASSERT_TRUE(nonEmpty);
}

/**
 * Create instances with different and identical values. Expect that instances with different values test as
 * not equal and that instances with identical values test as equal.
 */
TEST_F(HandlerAndPolicyTest, testOperatorEqualandNotEqual) {
    auto handler1 = std::make_shared<TestDirectiveHandler>();
    auto handler2 = std::make_shared<TestDirectiveHandler>();
    HandlerAndPolicy defaultHandlerAndPolicy;
    auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    HandlerAndPolicy handlerAndPolicy1(handler1, audioNonBlockingPolicy);
    HandlerAndPolicy handlerAndPolicy1Clone(handler1, audioNonBlockingPolicy);
    HandlerAndPolicy handlerAndPolicy2(handler1, BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, true));
    HandlerAndPolicy handlerAndPolicy3(handler2, audioNonBlockingPolicy);
    HandlerAndPolicy handlerAndPolicy4(nullptr, audioNonBlockingPolicy);
    ASSERT_EQ(defaultHandlerAndPolicy, defaultHandlerAndPolicy);
    ASSERT_NE(defaultHandlerAndPolicy, handlerAndPolicy1);
    ASSERT_EQ(handlerAndPolicy1, handlerAndPolicy1Clone);
    ASSERT_NE(handlerAndPolicy1, handlerAndPolicy2);
    ASSERT_NE(handlerAndPolicy1, handlerAndPolicy3);
    ASSERT_NE(handlerAndPolicy2, handlerAndPolicy3);
    ASSERT_NE(handlerAndPolicy3, handlerAndPolicy4);
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
