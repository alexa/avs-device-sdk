/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSCommon/Utils/Logger/ThreadMoniker.h"
#include "AVSCommon/Utils/Threading/TaskThread.h"
#include "AVSCommon/Utils/WaitEvent.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {
namespace test {

/// Timeout used while waiting for synchronization events.
/// We picked 2s to avoid failure in slower systems (e.g.: valgrind and emulators).
const std::chrono::milliseconds WAIT_TIMEOUT{100};

using namespace utils::test;
using namespace logger;

/// Test that wait will return if no job has ever started.
TEST(TaskThreadTest, testWaitForNothing) {
    TaskThread taskThread;
}

/// Test that start will fail if function is empty.
TEST(TaskThreadTest, testStartFailsDueToEmptyFunction) {
    TaskThread taskThread;
    std::function<bool()> emptyFunction;
    EXPECT_FALSE(taskThread.start(emptyFunction));
}

/// Test that start will trigger the provided job and thread will exit once the job is done and return @c false.
TEST(TaskThreadTest, testSimpleJob) {
    bool finished = false;
    auto simpleJob = [&finished] {
        finished = true;
        return false;
    };

    {
        TaskThread taskThread;
        EXPECT_TRUE(taskThread.start(simpleJob));
    }

    EXPECT_TRUE(finished);
}

/// Test that start will trigger the provided job and it will execute the job multiple times until the job returns
/// @c false.
TEST(TaskThreadTest, testSequenceJobs) {
    int taskCounter = 0;
    const int runUntil = 10;
    auto jobSequence = [&taskCounter] {
        taskCounter++;
        return taskCounter < runUntil;
    };

    {
        TaskThread taskThread;
        EXPECT_TRUE(taskThread.start(jobSequence));
    }

    EXPECT_EQ(taskCounter, runUntil);
}

/// Test that start will replace the existing next function.
/// - First function increments the counter, while the second will decrement until it reaches 0.
TEST(TaskThreadTest, testStartNewJob) {
    WaitEvent waitEvent;
    int taskCounter = 0;
    auto increment = [&taskCounter, &waitEvent] {
        taskCounter++;
        waitEvent.wakeUp();
        return true;
    };

    auto decrement = [&taskCounter] {
        taskCounter--;
        return taskCounter > 0;
    };

    TaskThread taskThread;
    EXPECT_TRUE(taskThread.start(increment));

    ASSERT_TRUE(waitEvent.wait(WAIT_TIMEOUT));
    EXPECT_TRUE(taskThread.start(decrement));
}

/// Test that start will fail if called multiple times while waiting for a job.
TEST(TaskThreadTest, testStartFailDueTooManyThreads) {
    WaitEvent waitEnqueue, waitStart;
    auto simpleJob = [&waitEnqueue, &waitStart] {
        waitStart.wakeUp();              // Job has started.
        waitEnqueue.wait(WAIT_TIMEOUT);  // Wait till job should finish.
        return false;
    };

    TaskThread taskThread;
    EXPECT_TRUE(taskThread.start(simpleJob));

    // Wait until first job has started.
    waitStart.wait(WAIT_TIMEOUT);
    EXPECT_TRUE(taskThread.start([] { return false; }));

    // This should fail since the task thread is starting.
    EXPECT_FALSE(taskThread.start([] { return false; }));

    waitEnqueue.wakeUp();
}

/// Test that threads related to this task thread will always have the same moniker.
TEST(TaskThreadTest, testMoniker) {
    WaitEvent waitGetMoniker, waitValidateMoniker;
    std::string moniker;
    auto getMoniker = [&moniker, &waitGetMoniker] {
        moniker = ThreadMoniker::getThisThreadMoniker();
        waitGetMoniker.wakeUp();
        return false;
    };

    auto validateMoniker = [&moniker, &waitValidateMoniker] {
        EXPECT_EQ(moniker, ThreadMoniker::getThisThreadMoniker());
        waitValidateMoniker.wakeUp();
        return false;
    };

    TaskThread taskThread;
    EXPECT_TRUE(taskThread.start(getMoniker));
    waitGetMoniker.wait(WAIT_TIMEOUT);

    EXPECT_TRUE(taskThread.start(validateMoniker));
    waitValidateMoniker.wait(WAIT_TIMEOUT);
}

/// Test that threads from different @c TaskThreads will have different monikers.
TEST(TaskThreadTest, testMonikerDifferentObjects) {
    WaitEvent waitGetMoniker, waitValidateMoniker;
    std::string moniker;
    auto getMoniker = [&moniker, &waitGetMoniker] {
        moniker = ThreadMoniker::getThisThreadMoniker();
        waitGetMoniker.wakeUp();
        return false;
    };

    auto validateMoniker = [&moniker, &waitValidateMoniker] {
        EXPECT_NE(moniker, ThreadMoniker::getThisThreadMoniker());
        waitValidateMoniker.wakeUp();
        return false;
    };

    TaskThread taskThread1;
    EXPECT_TRUE(taskThread1.start(getMoniker));
    waitGetMoniker.wait(WAIT_TIMEOUT);

    TaskThread taskThread2;
    EXPECT_TRUE(taskThread2.start(validateMoniker));
    waitValidateMoniker.wait(WAIT_TIMEOUT);
}

}  // namespace test
}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
