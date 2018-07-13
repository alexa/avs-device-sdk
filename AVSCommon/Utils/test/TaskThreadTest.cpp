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

#include "AVSCommon/Utils/Threading/TaskThread.h"
#include "ExecutorTestUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {
namespace test {

class TaskThreadTest : public ::testing::Test {
public:
    std::shared_ptr<TaskQueue> createTaskQueue() {
        return std::make_shared<TaskQueue>();
    };
};

TEST_F(TaskThreadTest, theTaskThreadReadsFromAGivenEmptyQueue) {
    auto queue = createTaskQueue();
    TaskThread taskThread{queue};
    taskThread.start();

    auto future = queue->push(TASK, VALUE);

    auto status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(status, std::future_status::ready);
    ASSERT_EQ(future.get(), VALUE);

    // shutting down the queue will lead to the thread shutting down
    queue->shutdown();
}

TEST_F(TaskThreadTest, theTaskThreadReadsFromAGivenNonEmptyQueue) {
    auto queue = createTaskQueue();
    auto future = queue->push(TASK, VALUE);

    TaskThread taskThread{queue};
    taskThread.start();

    auto status = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(status, std::future_status::ready);
    ASSERT_EQ(future.get(), VALUE);

    // shutting down the queue will lead to the thread shutting down
    queue->shutdown();
}

TEST_F(TaskThreadTest, theTaskThreadShutsDownWhenTheQueueIsDestroyed) {
    // the TaskThread only has a weak pointer to the queue, once it goes out of scope the thread will shutdown
    TaskThread taskThread{createTaskQueue()};
    taskThread.start();

    // wait for the thread to shutdown
    std::this_thread::sleep_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(taskThread.isShutdown(), true);
}

TEST_F(TaskThreadTest, theTaskThreadShutsDownWhenTheQueueIsShutdown) {
    auto queue = createTaskQueue();
    TaskThread taskThread{queue};
    taskThread.start();

    queue->shutdown();

    // wait for the thread to shutdown
    std::this_thread::sleep_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(taskThread.isShutdown(), true);
}

TEST_F(TaskThreadTest, theTaskThreadIsNotStartedUntilStartIsCalled) {
    auto queue = createTaskQueue();
    auto future = queue->push(TASK, VALUE);
    TaskThread taskThread{queue};

    // wait for long enough that the task would normall be run
    auto failedStatus = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(failedStatus, std::future_status::timeout);

    // start the thread
    taskThread.start();

    // now it should succeed almost immediately
    auto successStatus = future.wait_for(SHORT_TIMEOUT_MS);
    ASSERT_EQ(successStatus, std::future_status::ready);
    ASSERT_EQ(future.get(), VALUE);

    // shutting down the queue will lead to the thread shutting down
    queue->shutdown();
}

}  // namespace test
}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
