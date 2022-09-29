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

#include "AVSCommon/Utils/Logger/ThreadMoniker.h"
#include "AVSCommon/Utils/Threading/TaskThread.h"
#include "AVSCommon/Utils/WaitEvent.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {
namespace test {

/// Timeout used while waiting for synchronization events.
const std::chrono::milliseconds MY_WAIT_TIMEOUT{100};
/// Default thread moniker to use in tests.
const auto THREAD_MONIKER = "1a1";
/// Another thread moniker to use in tests.
const auto THREAD_MONIKER2 = "1a2";

using namespace logger;

/// Test that wait will return if no job has ever started.
TEST(TaskThreadTest, test_waitForNothing) {
    TaskThread taskThread;
}

/// Test that start will fail if function is empty.
TEST(TaskThreadTest, test_startFailsDueToEmptyFunction) {
    TaskThread taskThread;
    std::function<bool()> emptyFunction;
    EXPECT_FALSE(taskThread.start(emptyFunction, THREAD_MONIKER));
}

/// Test that start will trigger the provided job and thread will exit once the job is done and return @c false.
TEST(TaskThreadTest, test_simpleJob) {
    bool finished = false;
    WaitEvent waitEvent;
    auto simpleJob = [&finished, &waitEvent] {
        finished = true;
        waitEvent.wakeUp();
        return false;
    };

    {
        TaskThread taskThread;
        EXPECT_TRUE(taskThread.start(simpleJob, THREAD_MONIKER));
        EXPECT_TRUE(waitEvent.wait(MY_WAIT_TIMEOUT));
    }

    EXPECT_TRUE(finished);
}

/// Test that start will trigger the provided job and it will execute the job multiple times until the job returns
/// @c false.
TEST(TaskThreadTest, test_sequenceJobs) {
    int taskCounter = 0;
    const int runUntil = 10;
    WaitEvent waitEvent;
    auto jobSequence = [&] {
        taskCounter++;
        if (taskCounter < runUntil) {
            return true;
        }
        waitEvent.wakeUp();
        return false;
    };

    {
        TaskThread taskThread;
        EXPECT_TRUE(taskThread.start(jobSequence, THREAD_MONIKER));
        EXPECT_TRUE(waitEvent.wait(MY_WAIT_TIMEOUT));
    }

    EXPECT_EQ(taskCounter, runUntil);
}

/// Test that start will replace the existing next function.
/// - First function increments the counter, while the second will decrement until it reaches 0.
TEST(TaskThreadTest, test_startNewJob) {
    WaitEvent waitEvent;
    int taskCounter = 0;
    auto increment = [&taskCounter, &waitEvent] {
        taskCounter++;
        waitEvent.wakeUp();
        return true;
    };

    WaitEvent waitEvent2;
    auto decrement = [&taskCounter, &waitEvent2] {
        taskCounter--;
        if (taskCounter > 0) {
            return true;
        } else {
            waitEvent2.wakeUp();
            return false;
        }
    };

    TaskThread taskThread;
    EXPECT_TRUE(taskThread.start(increment, THREAD_MONIKER));

    EXPECT_TRUE(waitEvent.wait(MY_WAIT_TIMEOUT));
    EXPECT_TRUE(taskThread.start(decrement, THREAD_MONIKER));
    EXPECT_TRUE(waitEvent2.wait(MY_WAIT_TIMEOUT));
    EXPECT_TRUE(taskCounter == 0);
}

/// Test that start will fail if called multiple times while waiting for a job to start.
TEST(TaskThreadTest, testTimer_startFailDueTooManyThreads) {
    WaitEvent waitEnqueue, waitStart;
    auto simpleJob = [&waitEnqueue, &waitStart] {
        waitStart.wakeUp();                 // Job has started.
        waitEnqueue.wait(MY_WAIT_TIMEOUT);  // Wait till job should finish.
        return false;
    };

    TaskThread taskThread;
    EXPECT_TRUE(taskThread.start(simpleJob, THREAD_MONIKER));

    // Wait until first job has started.
    EXPECT_TRUE(waitStart.wait(MY_WAIT_TIMEOUT));
    EXPECT_TRUE(taskThread.start([] { return false; }, THREAD_MONIKER));

    // Starting a thread again immediately should fail, unless the system is so fast in starting
    // the thread on the other core that it starts and runs a few instructions before this can
    // call start again. We can account for such a very fast system by running in a loop 100 times.
    int threadStartCount;
    for (threadStartCount = 0; threadStartCount < 100; threadStartCount++) {
        // This should fail since the task thread is starting.
        if (!taskThread.start([] { return false; }, THREAD_MONIKER)) {
            break;
        }
    }
    EXPECT_TRUE(threadStartCount < 100);

    waitEnqueue.wakeUp();
}

/// Test that threads related to this task thread will always have specified moniker.
TEST(TaskThreadTest, DISABLED_test_moniker) {
    WaitEvent waitGetMoniker, waitValidateMoniker;
    std::string moniker;
    auto getMoniker = [&moniker, &waitGetMoniker] {
        moniker = ThreadMoniker::getThisThreadMoniker();
        waitGetMoniker.wakeUp();
        return false;
    };

    std::string moniker2;
    auto validateMoniker = [&moniker2, &waitValidateMoniker] {
        moniker2 = ThreadMoniker::getThisThreadMoniker();
        waitValidateMoniker.wakeUp();
        return false;
    };

    TaskThread taskThread;
    EXPECT_TRUE(taskThread.start(getMoniker, THREAD_MONIKER));
    EXPECT_TRUE(waitGetMoniker.wait(MY_WAIT_TIMEOUT));

    EXPECT_TRUE(taskThread.start(validateMoniker, THREAD_MONIKER2));
    EXPECT_TRUE(waitValidateMoniker.wait(MY_WAIT_TIMEOUT));

    EXPECT_EQ(THREAD_MONIKER, moniker);
    EXPECT_EQ(THREAD_MONIKER2, moniker2);
}

/// Test that threads from different @c TaskThreads will have different monikers.
TEST(TaskThreadTest, test_monikerDifferentObjects) {
    WaitEvent waitGetMoniker, waitThread2Start, waitValidateMoniker;
    std::string moniker;
    auto getMoniker = [&moniker, &waitGetMoniker, &waitThread2Start] {
        moniker = ThreadMoniker::getThisThreadMoniker();
        waitGetMoniker.wakeUp();
        // execute until thread2 has started, to ensure it cannot re-use the same thread.
        waitThread2Start.wait(MY_WAIT_TIMEOUT);
        return false;
    };

    std::string moniker2;
    auto validateMoniker = [&moniker2, &waitValidateMoniker] {
        moniker2 = ThreadMoniker::getThisThreadMoniker();
        waitValidateMoniker.wakeUp();
        return false;
    };

    TaskThread taskThread1;
    TaskThread taskThread2;
    EXPECT_TRUE(taskThread1.start(getMoniker, THREAD_MONIKER));
    EXPECT_TRUE(taskThread2.start(validateMoniker, THREAD_MONIKER2));
    waitThread2Start.wakeUp();
    EXPECT_TRUE(waitGetMoniker.wait(MY_WAIT_TIMEOUT));
    EXPECT_TRUE(waitValidateMoniker.wait(MY_WAIT_TIMEOUT));

    EXPECT_EQ(THREAD_MONIKER, moniker);
    EXPECT_EQ(THREAD_MONIKER2, moniker2);
}

}  // namespace test
}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
