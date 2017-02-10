/**
 * ExecutorTest.cpp
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

#include <gtest/gtest.h>

#include "ExecutorTestUtils.h"
#include "AVSUtils/Threading/Executor.h"

namespace alexaClientSDK {
namespace avsUtils {
namespace threading {

class ExecutorTest: public ::testing::Test {
public:
    Executor executor;
};

TEST_F(ExecutorTest, submitStdFunctionAndVerifyExecution) {
    std::function<void()> function = []() { };
    auto future = executor.submit(function);
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, submitStdBindAndVerifyExecution) {
    auto future = executor.submit(std::bind(exampleFunctionParams, 0));
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, submitLambdaAndVerifyExecution) {
    auto future = executor.submit([]() { });
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, submitFunctionPointerAndVerifyExecution) {
    auto future = executor.submit(&exampleFunction);
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, submitFunctorAndVerifyExecution) {
    ExampleFunctor exampleFunctor;
    auto future = executor.submit(exampleFunctor);
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, submitFunctionWithPrimitiveReturnTypeNoArgsAndVerifyExecution) {
    int value = VALUE;
    auto future = executor.submit([=]() { return value; });
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get(), value);
}

TEST_F(ExecutorTest, submitFunctionWithObjectReturnTypeNoArgsAndVerifyExecution) {
    SimpleObject value(VALUE);
    auto future = executor.submit([=]() { return value; });
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get().getValue(), value.getValue());
}

TEST_F(ExecutorTest, submitFunctionWithNoReturnTypePrimitiveArgsAndVerifyExecution) {
    int value = VALUE;
    auto future = executor.submit([](int number) { }, value);
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, submitFunctionWithNoReturnTypeObjectArgsAndVerifyExecution) {
    SimpleObject arg(0);
    auto future = executor.submit([](SimpleObject object) { }, arg);
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, submitFunctionWithPrimitiveReturnTypeObjectArgsAndVerifyExecution) {
    int value = VALUE;
    SimpleObject arg(0);
    auto future = executor.submit([=](SimpleObject object) { return value; }, arg);
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get(), value);
}

TEST_F(ExecutorTest, submitFunctionWithObjectReturnTypePrimitiveArgsAndVerifyExecution) {
    int arg = 0;
    SimpleObject value(VALUE);
    auto future = executor.submit([=](int primitive) { return value; }, arg);
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get().getValue(), value.getValue());
}

TEST_F(ExecutorTest, submitFunctionWithPrimitiveReturnTypePrimitiveArgsAndVerifyExecution) {
    int arg = 0;
    int value = VALUE;
    auto future = executor.submit([=](int number) { return value; }, arg);
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get(), value);
}

TEST_F(ExecutorTest, submitFunctionWithObjectReturnTypeObjectArgsAndVerifyExecution) {
    SimpleObject value(VALUE);
    SimpleObject arg(0);
    auto future = executor.submit([=](SimpleObject object) { return value; }, arg);
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get().getValue(), value.getValue());
}

} // namespace threading
} // namespace avsUtils
} // namespace alexaClientSDK
