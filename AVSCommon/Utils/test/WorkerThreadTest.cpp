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
#include <gtest/gtest.h>

#include "AVSCommon/Utils/Logger/ThreadMoniker.h"
#include "AVSCommon/Utils/Threading/WorkerThread.h"
#include "AVSCommon/Utils/WaitEvent.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {
namespace test {
using namespace std;
using namespace std::chrono;

/// Timeout used while waiting for synchronization events.
const std::chrono::milliseconds MY_WAIT_TIMEOUT{100};

TEST(TaskThreadTest, test_runWorkSeveralTimes) {
    std::atomic_int count{0};
    WaitEvent waitEvent;
    WorkerThread workerThread;

    for (int i = 1; i <= 10; i++) {
        waitEvent.reset();
        // return false, so only run once
        workerThread.run([&count, &waitEvent] {
            ++count;
            waitEvent.wakeUp();
            return false;
        });
        EXPECT_TRUE(waitEvent.wait(MY_WAIT_TIMEOUT));
        EXPECT_TRUE(count == i);
        // Ensure we are still not counting.
        for (int j = 0; j < 100; j++) {
            this_thread::yield();
        }
        EXPECT_TRUE(count == i);
    }
}

TEST(TaskThreadTest, test_runWorkRepeatedlyAndFinish) {
    std::atomic_int count{0};
    WaitEvent waitEvent;
    WorkerThread workerThread;
    workerThread.run([&count, &waitEvent] {
        if (++count < 100) {
            return true;
        }
        waitEvent.wakeUp();
        return false;
    });
    EXPECT_TRUE(waitEvent.wait(MY_WAIT_TIMEOUT));
    EXPECT_TRUE(count == 100);
}

TEST(TaskThreadTest, test_runWorkRepeatedlyAndCancelFromWorkerHappensImmediately) {
    std::atomic_int count{0};
    WaitEvent waitEvent;
    WorkerThread workerThread;

    workerThread.run([&count, &waitEvent, &workerThread] {
        if (++count == 10) {
            workerThread.cancel();
            waitEvent.wakeUp();
        }
        return true;
    });
    EXPECT_TRUE(waitEvent.wait(MY_WAIT_TIMEOUT));
    // Cancel will immediately have stopped the worker thread from continuing to run.
    EXPECT_TRUE(count == 10);
    for (int i = 0; i < 100; i++) this_thread::yield();
    EXPECT_TRUE(count == 10);
}

TEST(TaskThreadTest, test_runWorkRepeatedlyAndCancel) {
    std::atomic_int count{0};
    WaitEvent waitEvent;
    WorkerThread workerThread;

    workerThread.run([&count, &waitEvent] {
        if (++count == 10) {
            waitEvent.wakeUp();
        }
        return true;
    });
    EXPECT_TRUE(waitEvent.wait(MY_WAIT_TIMEOUT));

    // Calling cancel from the non-running thread will stop the worker thread within a reasonable amount of time.
    workerThread.cancel();
    this_thread::sleep_for(MY_WAIT_TIMEOUT);

    // cancel will stop the thread from continuing to count
    int snappedCount = count;
    for (int i = 0; i < 100; i++) this_thread::yield();
    EXPECT_TRUE(snappedCount == count);
}

TEST(TaskThreadTest, test_runWorkRepeatedlyWithCancelAndInterruptByDestruction) {
    std::atomic_int count{0};
    {
        WaitEvent waitEvent;
        WorkerThread workerThread;

        workerThread.run([&count, &waitEvent] {
            if (--count == -5) {
                waitEvent.wakeUp();
            }
            return true;
        });
        EXPECT_TRUE(waitEvent.wait(MY_WAIT_TIMEOUT));

        // Cancel running work.
        workerThread.cancel();
        waitEvent.reset();
        // Replace running work with different work, after canceling the previous work.
        workerThread.run([&count, &waitEvent] {
            if (++count == 5) {
                waitEvent.wakeUp();
            }
            return true;
        });

        EXPECT_TRUE(waitEvent.wait(MY_WAIT_TIMEOUT));
    }
    // destruction of workerThread will stop the thread from continuing to count.
    int snappedCount = count;
    for (int i = 0; i < 100; i++) this_thread::yield();
    EXPECT_TRUE(snappedCount == count);
}

}  // namespace test
}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
