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

#include "ExecutorTestUtils.h"
#include "AVSCommon/Utils/Threading/Executor.h"
#include "AVSCommon/Utils/Threading/TaskQueue.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {
namespace test {

class TaskQueueTest : public ::testing::Test {
public:
    /**
     * Asserts that a call to pop on an empty queue is blocking, and will be awoken by a task being pushed onto
     * the queue
     */
    void testQueueBlocksWhenEmpty() {
        // Have another thread blocked on the queue
        auto future = std::async(std::launch::async, [=]() {
            auto t = queue.pop();
            return t->operator()();
        });

        // This is expected to timeout
        auto failedStatus = future.wait_for(SHORT_TIMEOUT_MS);

        ASSERT_EQ(failedStatus, std::future_status::timeout);

        // Push a task to unblock the queue
        auto pushFuture = queue.push(TASK, VALUE);

        // This is expected to succeed
        auto successStatus = future.wait_for(SHORT_TIMEOUT_MS);
        ASSERT_EQ(successStatus, std::future_status::ready);

        // Verify the pushed future behaved correctly
        auto pushStatus = pushFuture.wait_for(SHORT_TIMEOUT_MS);
        ASSERT_EQ(pushStatus, std::future_status::ready);
        ASSERT_EQ(pushFuture.get(), VALUE);
    }

    TaskQueue queue;
};

TEST_F(TaskQueueTest, pushStdFunctionAndVerifyPopReturnsIt) {
    std::function<void()> function([]() {});
    auto future = queue.push(function);
    auto task = queue.pop();
    task->operator()();
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(TaskQueueTest, pushStdBindAndVerifyPopReturnsIt) {
    auto future = queue.push(std::bind(exampleFunctionParams, 0));
    auto task = queue.pop();
    task->operator()();
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(TaskQueueTest, pushLambdaAndVerifyPopReturnsIt) {
    auto future = queue.push([]() {});
    auto task = queue.pop();
    task->operator()();
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(TaskQueueTest, pushFunctionPointerAndVerifyPopReturnsIt) {
    auto future = queue.push(&exampleFunction);
    auto task = queue.pop();
    task->operator()();
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(TaskQueueTest, pushFunctorAndVerifyPopReturnsIt) {
    ExampleFunctor exampleFunctor;
    auto future = queue.push(exampleFunctor);
    auto task = queue.pop();
    task->operator()();
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(TaskQueueTest, pushFunctionWithPrimitiveReturnTypeNoArgsAndVerifyPopReturnsIt) {
    int value = VALUE;
    auto future = queue.push([=]() { return value; });
    auto task = queue.pop();
    task->operator()();
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get(), value);
}

TEST_F(TaskQueueTest, pushFunctionWithObjectReturnTypeNoArgsAndVerifyPopReturnsIt) {
    SimpleObject value(VALUE);
    auto future = queue.push([=]() { return value; });
    auto task = queue.pop();
    task->operator()();
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get().getValue(), value.getValue());
}

TEST_F(TaskQueueTest, pushFunctionWithNoReturnTypePrimitiveArgsAndVerifyPopReturnsIt) {
    int value = VALUE;
    auto future = queue.push([](int number) {}, value);
    auto task = queue.pop();
    task->operator()();
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(TaskQueueTest, pushFunctionWithNoReturnTypeObjectArgsAndVerifyPopReturnsIt) {
    SimpleObject arg(0);
    auto future = queue.push([](SimpleObject object) {}, arg);
    auto task = queue.pop();
    task->operator()();
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
}

TEST_F(TaskQueueTest, pushFunctionWithPrimitiveReturnTypeObjectArgsAndVerifyPopReturnsIt) {
    int value = VALUE;
    SimpleObject arg(0);
    auto future = queue.push([=](SimpleObject object) { return value; }, arg);
    auto task = queue.pop();
    task->operator()();
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get(), value);
}

TEST_F(TaskQueueTest, pushFunctionWithObjectReturnTypePrimitiveArgsAndVerifyPopReturnsIt) {
    int arg = 0;
    SimpleObject value(VALUE);
    auto future = queue.push([=](int primitive) { return value; }, arg);
    auto task = queue.pop();
    task->operator()();
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get().getValue(), value.getValue());
}

TEST_F(TaskQueueTest, pushFunctionWithPrimitiveReturnTypePrimitiveArgsAndVerifyPopReturnsIt) {
    int arg = 0;
    int value = VALUE;
    auto future = queue.push([=](int number) { return value; }, arg);
    auto task = queue.pop();
    task->operator()();
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get(), value);
}

TEST_F(TaskQueueTest, pushFunctionWithObjectReturnTypeObjectArgsAndVerifyPopReturnsIt) {
    SimpleObject value(VALUE);
    SimpleObject arg(0);
    auto future = queue.push([=](SimpleObject object) { return value; }, arg);
    auto task = queue.pop();
    task->operator()();
    auto future_status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(future_status, std::future_status::ready);
    ASSERT_EQ(future.get().getValue(), value.getValue());
}

TEST_F(TaskQueueTest, verifyFirstInFirstOutOrderIsMaintained) {
    int argOne = 1;
    int argTwo = 2;
    int argThree = 3;
    int argFour = 4;

    auto futureOne = queue.push(TASK, argOne);
    auto futureTwo = queue.push(TASK, argTwo);
    auto futureThree = queue.push(TASK, argThree);
    auto futureFour = queue.push(TASK, argFour);

    auto taskOne = queue.pop();
    auto taskTwo = queue.pop();
    auto taskThree = queue.pop();
    auto taskFour = queue.pop();

    taskOne->operator()();
    taskTwo->operator()();
    taskThree->operator()();
    taskFour->operator()();

    auto futureStatusOne = futureOne.wait_for(SHORT_TIMEOUT_MS);
    auto futureStatusTwo = futureTwo.wait_for(SHORT_TIMEOUT_MS);
    auto futureStatusThree = futureThree.wait_for(SHORT_TIMEOUT_MS);
    auto futureStatusFour = futureFour.wait_for(SHORT_TIMEOUT_MS);

    ASSERT_EQ(futureStatusOne, std::future_status::ready);
    ASSERT_EQ(futureStatusTwo, std::future_status::ready);
    ASSERT_EQ(futureStatusThree, std::future_status::ready);
    ASSERT_EQ(futureStatusFour, std::future_status::ready);

    ASSERT_EQ(futureOne.get(), argOne);
    ASSERT_EQ(futureTwo.get(), argTwo);
    ASSERT_EQ(futureThree.get(), argThree);
    ASSERT_EQ(futureFour.get(), argFour);
}

TEST_F(TaskQueueTest, popBlocksOnInitiallyEmptyQueue) {
    testQueueBlocksWhenEmpty();
}

TEST_F(TaskQueueTest, popBlocksOnEmptyQueueAfterAllTasksArePopped) {
    // Put a task on the queue, and take it off to get back to empty
    auto futureOne = queue.push(TASK, VALUE);
    auto taskOne = queue.pop();
    taskOne->operator()();
    auto futureOneStatus = futureOne.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(futureOneStatus, std::future_status::ready);
    ASSERT_EQ(futureOne.get(), VALUE);

    testQueueBlocksWhenEmpty();
}

TEST_F(TaskQueueTest, isShutdownReturnsFalseWhenRunning) {
    ASSERT_EQ(queue.isShutdown(), false);
}

TEST_F(TaskQueueTest, isShutdownReturnsTrueAfterShutdown) {
    queue.shutdown();
    ASSERT_EQ(queue.isShutdown(), true);
}

TEST_F(TaskQueueTest, shutdownUnblocksAnEmptyQueue) {
    // Have another thread blocked on the queue
    auto future = std::async(std::launch::async, [=]() {
        auto t = queue.pop();
        if (t) t->operator()();
    });

    // This is expected to timeout
    auto failedStatus = future.wait_for(SHORT_TIMEOUT_MS);

    ASSERT_EQ(failedStatus, std::future_status::timeout);

    // Shutdown to unblock the queue
    queue.shutdown();

    // This is expected to succeed
    auto successStatus = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(successStatus, std::future_status::ready);
}

TEST_F(TaskQueueTest, pushFailsToEnqueueANewTaskOnAShutdownQueue) {
    // No tasks should be enqueued
    queue.shutdown();

    auto future = queue.push(TASK, VALUE);
    ASSERT_EQ(future.valid(), false);

    auto retrievedTask = queue.pop();

    ASSERT_EQ(retrievedTask, nullptr);
}

}  // namespace test
}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
