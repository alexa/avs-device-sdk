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

#include <list>
#include <gtest/gtest.h>

#include "ExecutorTestUtils.h"
#include "AVSCommon/Utils/Threading/Executor.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {
namespace test {

class ExecutorTest : public ::testing::Test {
public:
    Executor executor;
};

TEST_F(ExecutorTest, submitStdFunctionAndVerifyExecution) {
    std::function<void()> function = []() {};
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
    auto future = executor.submit([]() {});
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
    auto future = executor.submit([](int number) {}, value);
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, submitFunctionWithNoReturnTypeObjectArgsAndVerifyExecution) {
    SimpleObject arg(0);
    auto future = executor.submit([](SimpleObject object) {}, arg);
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

TEST_F(ExecutorTest, submitToFront) {
    std::atomic<bool> ready(false);
    std::atomic<bool> blocked(false);
    std::list<int> order;

    // submit a task which will block the executor
    executor.submit([&] {
        blocked = true;
        while (!ready) {
            std::this_thread::yield();
        }
    });

    // wait for it to block
    while (!blocked) {
        std::this_thread::yield();
    }

    // submit a task to the empty queue
    executor.submit([&] { order.push_back(1); });

    // submit a task to the back of the queue
    executor.submit([&] { order.push_back(2); });

    // submit a task to the front of the queue
    executor.submitToFront([&] { order.push_back(3); });

    // unblock the executor
    ready = true;

    // wait for all tasks to complete
    executor.waitForSubmittedTasks();

    // verify execution order
    ASSERT_EQ(order.size(), 3U);
    ASSERT_EQ(order.front(), 3);
    ASSERT_EQ(order.back(), 2);
}

/// Used by @c futureWaitsForTaskCleanup delay and timestamp the time of lambda parameter destruction.
struct SlowDestructor {
    /// Constructor.
    SlowDestructor() : cleanedUp{nullptr} {
    }

    /// Destructor which delays destruction, timestamps, and notifies a condition variable when it is done
    ~SlowDestructor() {
        if (cleanedUp) {
            /* Delay briefly so that there is a measurable delay between the completion of the lambda's content and the
               cleanup of the lambda's parameters */
            std::this_thread::sleep_for(SHORT_TIMEOUT_MS / 10);

            // Note the time when the destructor has (nominally) completed.
            *cleanedUp = true;
        }
    }

    /**
     * Boolean indicating destruction is completed (if != nullptr).  Mutable so that a lambda can write to it in a
     * SlowDestructor parameter that is captured by value.
     */
    mutable std::atomic<bool>* cleanedUp;
};

/// This test verifies that the executor waits to fulfill its promise until after the task is cleaned up.
TEST_F(ExecutorTest, futureWaitsForTaskCleanup) {
    std::atomic<bool> cleanedUp(false);
    SlowDestructor slowDestructor;

    // Submit a lambda to execute which captures a parameter by value that is slow to destruct.
    executor
        .submit([slowDestructor, &cleanedUp] {
            // Update the captured copy of slowDestructor so that it will delay destruction and update the cleanedUp
            // flag.
            slowDestructor.cleanedUp = &cleanedUp;
        }
                // wait for the promise to be fulfilled.
                )
        .wait();

    ASSERT_TRUE(cleanedUp);
}

/// This test verifies that the shutdown function completes the current task and does not accept new tasks.
TEST_F(ExecutorTest, shutdown) {
    std::atomic<bool> ready(false);
    std::atomic<bool> blocked(false);

    // submit a task which will block the executor and then sleep briefly
    auto done = executor.submit([&] {
        blocked = true;
        while (!ready) {
            std::this_thread::yield();
        }
        std::this_thread::sleep_for(SHORT_TIMEOUT_MS);
    });

    // wait for it to block
    while (!blocked) {
        std::this_thread::yield();
    }

    // release the task to start sleeping
    ready = true;

    // shut down the executor
    EXPECT_FALSE(executor.isShutdown());
    executor.shutdown();
    EXPECT_TRUE(executor.isShutdown());

    // verify that the task has now completed
    EXPECT_TRUE(done.valid());
    done.get();

    // try to submit a new task and verify that it is rejected
    auto rejected = executor.submit([] {});
    ASSERT_FALSE(rejected.valid());
}

}  // namespace test
}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
