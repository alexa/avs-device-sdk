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

#include <chrono>
#include <condition_variable>
#include <gtest/gtest.h>
#include <list>
#include <mutex>
#include <system_error>
#include <thread>

#include "ExecutorTestUtils.h"
#include "AVSCommon/Utils/Threading/Executor.h"
#include "AVSCommon/Utils/WaitEvent.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {
namespace test {

/// Maximum timeout for blocking wait when expecting a signal.
static const std::chrono::seconds EXECUTOR_SIGNAL_WAIT_TIMEOUT{30};

class ExecutorTest : public ::testing::Test {
public:
    Executor executor;
};

TEST_F(ExecutorTest, testTimer_submitStdFunctionAndVerifyExecution) {
    std::function<void()> function = []() {};
    auto future = executor.submit(function);
    ASSERT_TRUE(future.valid());
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, testTimer_submitStdBindAndVerifyExecution) {
    auto future = executor.submit(std::bind(exampleFunctionParams, 0));
    ASSERT_TRUE(future.valid());
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, testTimer_submitLambdaAndVerifyExecution) {
    auto future = executor.submit([]() {});
    ASSERT_TRUE(future.valid());
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, testTimer_submitFunctionPointerAndVerifyExecution) {
    auto future = executor.submit(&exampleFunction);
    ASSERT_TRUE(future.valid());
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, testTimer_submitFunctorAndVerifyExecution) {
    ExampleFunctor exampleFunctor;
    auto future = executor.submit(exampleFunctor);
    ASSERT_TRUE(future.valid());
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, testTimer_submitFunctionWithPrimitiveReturnTypeNoArgsAndVerifyExecution) {
    int value = VALUE;
    auto future = executor.submit([=]() { return value; });
    ASSERT_TRUE(future.valid());
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get(), value);
}

TEST_F(ExecutorTest, testTimer_submitFunctionWithObjectReturnTypeNoArgsAndVerifyExecution) {
    SimpleObject value(VALUE);
    auto future = executor.submit([=]() { return value; });
    ASSERT_TRUE(future.valid());
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get().getValue(), value.getValue());
}

TEST_F(ExecutorTest, testTimer_submitFunctionWithNoReturnTypePrimitiveArgsAndVerifyExecution) {
    int value = VALUE;
    auto future = executor.submit([](int number) {}, value);
    ASSERT_TRUE(future.valid());
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, testTimer_submitFunctionWithNoReturnTypeObjectArgsAndVerifyExecution) {
    SimpleObject arg(0);
    auto future = executor.submit([](SimpleObject object) {}, arg);
    ASSERT_TRUE(future.valid());
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(ExecutorTest, testTimer_submitFunctionWithPrimitiveReturnTypeObjectArgsAndVerifyExecution) {
    int value = VALUE;
    SimpleObject arg(0);
    auto future = executor.submit([=](SimpleObject object) { return value; }, arg);
    ASSERT_TRUE(future.valid());
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get(), value);
}

TEST_F(ExecutorTest, testTimer_submitFunctionWithObjectReturnTypePrimitiveArgsAndVerifyExecution) {
    int arg = 0;
    SimpleObject value(VALUE);
    auto future = executor.submit([=](int primitive) { return value; }, arg);
    ASSERT_TRUE(future.valid());
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get().getValue(), value.getValue());
}

TEST_F(ExecutorTest, testTimer_submitFunctionWithPrimitiveReturnTypePrimitiveArgsAndVerifyExecution) {
    int arg = 0;
    int value = VALUE;
    auto future = executor.submit([=](int number) { return value; }, arg);
    ASSERT_TRUE(future.valid());
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get(), value);
}

TEST_F(ExecutorTest, testTimer_submitFunctionWithObjectReturnTypeObjectArgsAndVerifyExecution) {
    SimpleObject value(VALUE);
    SimpleObject arg(0);
    auto future = executor.submit([=](SimpleObject object) { return value; }, arg);
    ASSERT_TRUE(future.valid());
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get().getValue(), value.getValue());
}

TEST_F(ExecutorTest, testTimer_submitToFront) {
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

TEST_F(ExecutorTest, testTimer_executionOrderEqualToSubmitOrder) {
    WaitEvent waitSetUp;
    executor.submit([&waitSetUp] { waitSetUp.wait(SHORT_TIMEOUT_MS); });

    // submit a task which will block the executor
    executor.submit([&waitSetUp] { waitSetUp.wait(SHORT_TIMEOUT_MS); });

    std::list<int> order;
    std::list<int> expectedOrder = {1, 2, 3};
    for (auto& value : expectedOrder) {
        // submit tasks in the expected order.
        executor.submit([&order, value] { order.push_back(value); });
    }

    // unblock the executor
    waitSetUp.wakeUp();

    // wait for all tasks to complete
    executor.waitForSubmittedTasks();

    // verify execution order
    ASSERT_EQ(order, expectedOrder);
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
TEST_F(ExecutorTest, testTimer_futureWaitsForTaskCleanup) {
    std::atomic<bool> cleanedUp(false);
    SlowDestructor slowDestructor;

    // Submit a lambda to execute which captures a parameter by value that is slow to destruct.
    auto future = executor.submit([slowDestructor, &cleanedUp] {
        // Update the captured copy of slowDestructor so that it will delay destruction and update the cleanedUp
        // flag.
        slowDestructor.cleanedUp = &cleanedUp;
    });
    ASSERT_TRUE(future.valid());
    // wait for the promise to be fulfilled.
    ASSERT_EQ(std::future_status::ready, future.wait_for(SHORT_TIMEOUT_MS * 2));

    ASSERT_TRUE(cleanedUp);
}

/// This test verifies that the shutdown function completes the current task and does not accept new tasks.
TEST_F(ExecutorTest, testTimer_shutdown) {
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
    ASSERT_TRUE(done.valid());
    done.get();

    // try to submit a new task and verify that it is rejected
    auto rejected = executor.submit([] {});
    ASSERT_FALSE(rejected.valid());
}

/// Test that calling submit after shutdown will fail the job.
TEST_F(ExecutorTest, testTimer_pushAfterExecutordownFail) {
    executor.shutdown();
    ASSERT_TRUE(executor.isShutdown());

    EXPECT_FALSE(executor.submit([] {}).valid());
    EXPECT_FALSE(executor.submitToFront([] {}).valid());
}

/// Test that shutdown cancel jobs in the queue.
TEST_F(ExecutorTest, testTimer_shutdownCancelJob) {
    bool executed = false;
    WaitEvent waitSetUp, waitJobStart;
    std::future<void> jobToDropResult;

    // Job that should be cancelled and never run.
    auto jobToDrop = [&executed] { executed = true; };

    // Job used to validate that jobToDrop return value becomes available (but invalid).
    auto jobToWaitDrop = [&jobToDropResult, &waitSetUp, &waitJobStart] {
        waitJobStart.wakeUp();
        waitSetUp.wait(SHORT_TIMEOUT_MS);
        // prevent unit test crash if jobToDropResult is not valid. It is checked below.
        if (jobToDropResult.valid()) {
            jobToDropResult.wait_for(SHORT_TIMEOUT_MS);
        }
    };

    // 1st job waits for setup to be done then wait for the second job to be cancelled.
    executor.submit(jobToWaitDrop);

    // 2nd job that should never run. When cancelled, its return will become available.
    jobToDropResult = executor.submit(jobToDrop);
    ASSERT_TRUE(jobToDropResult.valid());

    // Wake up first job and wait for it to start running.
    const std::chrono::seconds DEFAULT_TIMEOUT{5};
    waitSetUp.wakeUp();
    waitJobStart.wait(DEFAULT_TIMEOUT);

    // Shutdown should cancel enqueued jobs and wait for the ongoing job.
    executor.shutdown();

    // Executed should still be false.
    EXPECT_FALSE(executed);
}

TEST_F(ExecutorTest, test_forwardPromise) {
    // Should forward the value
    {
        std::promise<int> src;
        src.set_value(42);

        auto future = src.get_future();
        std::promise<int> dst;
        forwardPromise(dst, future);
        EXPECT_EQ(dst.get_future().get(), 42);
    }
    // Should forward the void value
    {
        std::promise<void> src;
        src.set_value();

        auto future = src.get_future();
        std::promise<void> dst;
        forwardPromise(dst, future);
        EXPECT_NO_THROW(dst.get_future().get());
    }
    // Should forward the exception
    {
        std::promise<int> src;
        src.set_exception(std::make_exception_ptr(std::exception()));

        auto future = src.get_future();
        std::promise<int> dst;
        forwardPromise(dst, future);
        EXPECT_THROW(dst.get_future().get(), std::exception);
    }
    // Should forward the exception
    {
        std::promise<void> src;
        src.set_exception(std::make_exception_ptr(std::exception()));

        auto future = src.get_future();
        std::promise<void> dst;
        forwardPromise(dst, future);
        EXPECT_THROW(dst.get_future().get(), std::exception);
    }
}

TEST_F(ExecutorTest, test_taskException) {
    {
        auto future = executor.submit([] { throw std::exception(); });
        ASSERT_TRUE(future.valid());
        EXPECT_THROW(future.get(), std::exception);
    }
    {
        auto future = executor.submit(
            [](int param) -> int {
                throw std::runtime_error("catch me");
                return param;
            },
            42);
        ASSERT_TRUE(future.valid());
        EXPECT_THROW(future.get(), std::runtime_error);
    }
}

/// Verify that empty function is not accepted by executor using movable function.
TEST_F(ExecutorTest, test_executeEmptyMove) {
    std::function<void()> fn;
    ASSERT_FALSE(executor.execute(std::move(fn)));
}

/// Verify that empty function is not accepted by executor using const reference function.
TEST_F(ExecutorTest, test_executeEmptyRef) {
    const std::function<void()> fn;
    ASSERT_FALSE(executor.execute(fn));
}

/// Verify that after task execution, the lambda is released if movable function is used.
TEST_F(ExecutorTest, test_executeLambdaMove) {
    std::mutex mutex;
    std::condition_variable cond;
    volatile bool executed = false;
    volatile bool canExecute = false;
    volatile bool started = false;
    std::error_condition error;

    auto shared = std::make_shared<std::string>();
    auto weak = std::weak_ptr<std::string>(shared);
    auto lambda = [&, shared] {
        std::unique_lock<std::mutex> lock{mutex};
        started = true;
        cond.notify_all();
        if (cond.wait_for(lock, EXECUTOR_SIGNAL_WAIT_TIMEOUT, [&] { return canExecute; })) {
            executed = true;
        } else {
            error = std::errc::timed_out;
        }
        (void)&shared;
    };

    // Release strong reference and verify weak one is still valid (hold by lambda).
    shared.reset();
    ASSERT_FALSE(shared);
    ASSERT_TRUE(weak.lock());

    // Initiate execution but block executor thread in lambda.
    ASSERT_TRUE(executor.execute(std::move(lambda)));
    // Ensure lambda execution has started and blocked. If there is a bug in lambda, the executed flag may be set only
    // if we give enough time for new thread start.
    {
        std::unique_lock<std::mutex> lock{mutex};
        ASSERT_TRUE(cond.wait_for(lock, EXECUTOR_SIGNAL_WAIT_TIMEOUT, [&] { return started; }));
    }
    ASSERT_FALSE(executed);

    // Check the reference is still valid.
    ASSERT_TRUE(weak.lock());

    // Allow lambda to complete
    {
        std::unique_lock<std::mutex> lock{mutex};
        canExecute = true;
    }
    cond.notify_all();
    executor.waitForSubmittedTasks();

    // Verify the task is completed and shared object is released.
    ASSERT_TRUE(executed);
    ASSERT_FALSE(weak.lock());
    ASSERT_FALSE(error);
}

/// Verify that after task execution, the lambda is not released if movable function is not used.
TEST_F(ExecutorTest, test_executeLambdaRef) {
    std::mutex mutex;
    std::condition_variable cond;
    volatile bool executed = false;
    volatile bool canExecute = false;
    volatile bool started = false;
    std::error_condition error;

    auto shared = std::make_shared<std::string>();
    auto weak = std::weak_ptr<std::string>(shared);
    const auto lambda = [&, shared] {
        std::unique_lock<std::mutex> lock{mutex};
        started = true;
        cond.notify_all();
        if (cond.wait_for(lock, EXECUTOR_SIGNAL_WAIT_TIMEOUT, [&] { return canExecute; })) {
            executed = true;
        } else {
            error = std::errc::timed_out;
        }
        (void)&shared;
    };

    // Release strong reference and verify weak one is still valid (hold by lambda).
    shared.reset();
    ASSERT_FALSE(shared);
    ASSERT_TRUE(weak.lock());

    // Initiate execution but block executor thread in lambda.
    ASSERT_TRUE(executor.execute(lambda));
    // Ensure lambda execution has started and blocked. If there is a bug in lambda, the executed flag may be set only
    // if we give enough time for new thread start.
    {
        std::unique_lock<std::mutex> lock{mutex};
        ASSERT_TRUE(cond.wait_for(lock, EXECUTOR_SIGNAL_WAIT_TIMEOUT, [&] { return started; }));
    }
    ASSERT_FALSE(executed);

    // Check the reference is still valid.
    ASSERT_TRUE(weak.lock());

    // Allow lambda to complete
    {
        std::unique_lock<std::mutex> lock{mutex};
        canExecute = true;
    }
    cond.notify_all();
    executor.waitForSubmittedTasks();

    // Verify task is completed and we still have shared object through lambda
    ASSERT_TRUE(executed);
    ASSERT_TRUE(weak.lock());
    ASSERT_FALSE(error);
}

}  // namespace test
}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
