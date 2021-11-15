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
#include <chrono>
#include <thread>

#include "AVSCommon/Utils/Timing/MultiTimer.h"
#include "AVSCommon/Utils/WaitEvent.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {
namespace test {

/// Test that an enqueued task gets called.
TEST(MultiTimerTest, test_taskGetsCalled) {
    WaitEvent calledEvent;
    MultiTimer timer;
    timer.submitTask(std::chrono::milliseconds(10), [&calledEvent] { calledEvent.wakeUp(); });

    EXPECT_TRUE(calledEvent.wait(std::chrono::seconds(5)));
}

/// Test that an enqueued task that is cancelled before its expiration date does not run.
TEST(MultiTimerTest, testTimer_cancelledTaskShouldNotRun) {
    WaitEvent calledEvent;
    MultiTimer timer;
    auto token = timer.submitTask(std::chrono::seconds(1), [&calledEvent] {
        // Task should not be called.
        EXPECT_TRUE(false);
        calledEvent.wakeUp();
    });

    timer.cancelTask(token);

    EXPECT_FALSE(calledEvent.wait(std::chrono::seconds(2)));
}

/// Insert two tasks to the timer where the first task has a longer wait period and the second is much shorter.
/// Expect that the second one is run first. The first task should not run since we will exit the test before its
/// expiration time.
TEST(MultiTimerTest, test_executionOrderFollowExpirationTime) {
    WaitEvent calledEvent;
    MultiTimer timer;
    size_t counter = 0;
    timer.submitTask(std::chrono::seconds(10), [&counter] {
        // This function should never be called.
        counter++;
        EXPECT_TRUE(false);
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    timer.submitTask(std::chrono::milliseconds(10), [&calledEvent, &counter] {
        // This function is due first and should be called first.
        counter++;
        calledEvent.wakeUp();
    });

    EXPECT_TRUE(calledEvent.wait(std::chrono::seconds(5)));
    EXPECT_EQ(counter, 1u);
}

/// Insert two tasks to the timer with the same delay. Expect that the tasks are run in the order they were inserted.
TEST(MultiTimerTest, test_multipleTasksGetCalledInOrder) {
    WaitEvent calledEvent;
    MultiTimer timer;
    size_t counter = 0;
    timer.submitTask(std::chrono::milliseconds(100), [&counter] {
        // This task should be executed first.
        counter++;
        EXPECT_EQ(counter, 1u);
    });

    timer.submitTask(std::chrono::milliseconds(100), [&calledEvent, &counter] {
        // This should be the second task to be executed.
        counter++;
        EXPECT_EQ(counter, 2u);
        calledEvent.wakeUp();
    });

    EXPECT_TRUE(calledEvent.wait(std::chrono::seconds(5)));
    EXPECT_EQ(counter, 2u);
}

}  // namespace test
}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
