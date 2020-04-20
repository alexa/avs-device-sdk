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
#include "AVSCommon/Utils/Threading/ThreadPool.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {
namespace test {

/// Test that wait will return if no job has ever started.
TEST(TaskThreadTest, test_obtainAndreleaseWorker) {
    ThreadPool testTheadPool{};
    uint64_t threadsCreated, threadsObtained, threadsReleasedToPool, threadsReleasedFromPool;

    // Stats should start out at all 0.
    testTheadPool.getStats(threadsCreated, threadsObtained, threadsReleasedToPool, threadsReleasedFromPool);
    EXPECT_TRUE(threadsCreated == 0 && threadsObtained == 0);
    EXPECT_TRUE(threadsReleasedToPool == 0 && threadsReleasedFromPool == 0);

    auto workerThread = testTheadPool.obtainWorker();
    EXPECT_TRUE(workerThread != nullptr);
    testTheadPool.getStats(threadsCreated, threadsObtained, threadsReleasedToPool, threadsReleasedFromPool);
    EXPECT_TRUE(threadsCreated == 1 && threadsObtained == 1);
    EXPECT_TRUE(threadsReleasedToPool == 0 && threadsReleasedFromPool == 0);

    testTheadPool.releaseWorker(std::move(workerThread));
    testTheadPool.getStats(threadsCreated, threadsObtained, threadsReleasedToPool, threadsReleasedFromPool);
    EXPECT_TRUE(threadsCreated == 1 && threadsObtained == 1);
    EXPECT_TRUE(threadsReleasedToPool == 1 && threadsReleasedFromPool == 0);

    // Verify thread is re-used from the pool
    workerThread = testTheadPool.obtainWorker();
    EXPECT_TRUE(workerThread != nullptr);
    testTheadPool.getStats(threadsCreated, threadsObtained, threadsReleasedToPool, threadsReleasedFromPool);
    EXPECT_TRUE(threadsCreated == 1 && threadsObtained == 2);
    EXPECT_TRUE(threadsReleasedToPool == 1 && threadsReleasedFromPool == 0);
}

/// Test that wait will return if no job has ever started.
TEST(TaskThreadTest, test_releaseMultipleHonorsMax) {
    // Default thread pool should have 20 threads.
    const auto expectedMaxThreads = 20;
    EXPECT_TRUE(ThreadPool::getDefaultThreadPool()->getMaxThreads() == expectedMaxThreads);
    ThreadPool testTheadPool{};
    EXPECT_TRUE(testTheadPool.getMaxThreads() == expectedMaxThreads);

    uint64_t threadsCreated, threadsObtained, threadsReleasedToPool, threadsReleasedFromPool;
    std::vector<std::unique_ptr<WorkerThread>> workerThreads;

    // create 2x worker threads than the pool holds.
    for (int i = 0; i < expectedMaxThreads * 2; i++) {
        workerThreads.push_back(testTheadPool.obtainWorker());
    }
    testTheadPool.getStats(threadsCreated, threadsObtained, threadsReleasedToPool, threadsReleasedFromPool);
    EXPECT_TRUE(threadsCreated == expectedMaxThreads * 2 && threadsObtained == expectedMaxThreads * 2);
    EXPECT_TRUE(threadsReleasedToPool == 0 && threadsReleasedFromPool == 0);

    // release 20 back to the pool
    for (int i = 0; i < expectedMaxThreads; i++) {
        testTheadPool.releaseWorker(std::move(workerThreads.back()));
        workerThreads.pop_back();

        testTheadPool.getStats(threadsCreated, threadsObtained, threadsReleasedToPool, threadsReleasedFromPool);
        EXPECT_TRUE(threadsCreated == expectedMaxThreads * 2 && threadsObtained == expectedMaxThreads * 2);
        EXPECT_TRUE(threadsReleasedToPool == static_cast<uint64_t>(i + 1) && threadsReleasedFromPool == 0);
    }

    // release remaining back to the pool
    uint64_t released = 0;
    while (!workerThreads.empty()) {
        testTheadPool.releaseWorker(std::move(workerThreads.back()));
        workerThreads.pop_back();

        testTheadPool.getStats(threadsCreated, threadsObtained, threadsReleasedToPool, threadsReleasedFromPool);
        EXPECT_TRUE(threadsCreated == (expectedMaxThreads * 2) && threadsObtained == (expectedMaxThreads * 2));
        EXPECT_TRUE(threadsReleasedToPool == 20 && threadsReleasedFromPool == ++released);
    }
}

}  // namespace test
}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
