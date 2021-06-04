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

#ifdef ENABLE_LPM

#include <atomic>
#include <condition_variable>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockPowerResourceManager.h>
#include <AVSCommon/Utils/Power/PowerMonitor.h>
#include <AVSCommon/Utils/Power/PowerResource.h>
#include <AVSCommon/Utils/Threading/ConditionVariableWrapper.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <AVSCommon/Utils/Timing/StopTaskTimer.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::sdkInterfaces::timing;
using namespace avsCommon::utils;
using namespace avsCommon::utils::error;
using namespace avsCommon::utils::power;
using namespace avsCommon::utils::timing;

/// PowerResource Component name for thread.
static std::string TEST_THREAD_ID = "test-thread";

/// PowerResource Component name.
static std::string TEST_ID = "test";

/// Long timeout used to ensure an expected event does happen.
static std::chrono::minutes LONG_TIMEOUT{2};

/// Tolerance in time difference used for testing with LONG_TIMEOUT.
static std::chrono::minutes TIMEOUT_TOLERANCE{2};

/// Short timeout used to verify an event does not happen.
static std::chrono::milliseconds SHORT_TIMEOUT{750};

/**
 * Due to the nature of the object under test, ConditionVariableWrapper, we must perform white-box testing
 * to verify the correct notification and power management logic.
 *
 * These tests require PowerMonitor to be enabled.
 */
class ConditionVariableWrapperPowerTest : public ::testing::Test {
public:
    virtual void SetUp() override;
    virtual void TearDown() override;

    /**
     * Checks notifyX methods against power expectations.
     *
     * Check that during wait thread PowerResources are released.
     * Check that PowerResource are acquired in notifyAll.
     * Check that there are non-zero PowerResources acquired in wait predicate.
     * Check that PowerResources are thawed after wait predicate.
     *
     * @param numberOfThreads Number of threads to spawn.
     * @param notify The notify methods to call.
     */
    void notifyPowerCheck(int numberOfThreads, std::function<void()> notify);

    /// PowerResourceManagerInterface Mock.
    std::shared_ptr<NiceMock<MockPowerResourceManager>> m_powerManagerMock;

    /// Mutex for synchronization.
    std::mutex m_mutex;

    /**
     * ConditionVariableWrapper.
     * Use a pointer because we need to ensure this is instantiated
     * only after PowerMonitor has been initialized with a @c PowerResourceMonitorInterface.
     */
    std::shared_ptr<ConditionVariableWrapper> m_cv;

    /// Track refcount for @c MockPowerResourceManager.
    std::atomic<int> m_totalRefCount;

    /// Variable to measure success.
    bool m_exitCondition;

    /// Return value from waitFor/waitUntil operations.
    bool m_waitReturn;

    /// Often tests require that m_cv be in the waiting state.
    WaitEvent m_enteredWaiting;
};

/// These are tests that do not require PowerMonitor to be enabled.
class ConditionVariableWrapperTest
        : public ConditionVariableWrapperPowerTest
        , public testing::WithParamInterface<bool> {
public:
    void SetUp() override;
};

void ConditionVariableWrapperPowerTest::SetUp() {
    m_powerManagerMock = std::make_shared<NiceMock<MockPowerResourceManager>>();
    PowerMonitor::getInstance()->activate(m_powerManagerMock);
    m_cv = std::make_shared<ConditionVariableWrapper>();
    m_exitCondition = false;
    m_totalRefCount = 0;
    m_waitReturn = false;

    ON_CALL(*m_powerManagerMock, acquire(_, _)).WillByDefault(InvokeWithoutArgs([this] {
        m_totalRefCount++;
        return true;
    }));

    ON_CALL(*m_powerManagerMock, release(_)).WillByDefault(InvokeWithoutArgs([this] {
        if (m_totalRefCount < 1) {
            return false;
        }
        m_totalRefCount--;
        return true;
    }));
}

void ConditionVariableWrapperTest::SetUp() {
    m_cv = std::make_shared<ConditionVariableWrapper>();
    m_exitCondition = false;
    m_totalRefCount = 0;
    m_waitReturn = false;

    if (GetParam()) {
        m_powerManagerMock = std::make_shared<NiceMock<MockPowerResourceManager>>();
        PowerMonitor::getInstance()->activate(m_powerManagerMock);

        ON_CALL(*m_powerManagerMock, acquire(_, _)).WillByDefault(InvokeWithoutArgs([this] {
            m_totalRefCount++;
            return true;
        }));
        ON_CALL(*m_powerManagerMock, release(_)).WillByDefault(InvokeWithoutArgs([this] {
            if (m_totalRefCount < 1) {
                return false;
            }
            m_totalRefCount--;
            return true;
        }));
    }
}

void ConditionVariableWrapperPowerTest::TearDown() {
    PowerMonitor::getInstance()->deactivate();
    m_cv.reset();
}

INSTANTIATE_TEST_CASE_P(TogglePowerMonitor, ConditionVariableWrapperTest, testing::Values(true, false));

void ConditionVariableWrapperPowerTest::notifyPowerCheck(int numberOfThreads, std::function<void()> notify) {
    std::vector<std::string> threadMonikers;
    std::vector<std::shared_ptr<std::thread>> threads;
    // Check each thread's PowerResource.
    std::map<std::string, std::shared_ptr<PowerResource>> waitingThreadPRs;
    std::vector<std::pair<std::string, std::thread::id>> acquireThreadIds;

    // Only proceed if all threads under test are waiting.
    std::condition_variable waitCV;
    std::mutex waitMutex;

    for (int i = 0; i < numberOfThreads; i++) {
        threadMonikers.push_back(TEST_THREAD_ID + std::to_string(i));
    }

    auto threadLogic = [this, &waitingThreadPRs, &waitMutex, &waitCV](std::string moniker) {
        auto threadPR = PowerMonitor::getInstance()->getThreadPowerResourceOrCreate(moniker);
        threadPR->acquire();

        std::unique_lock<std::mutex> lock(m_mutex);
        waitingThreadPRs.insert({moniker, threadPR});
        m_cv->wait(lock, [this, &waitMutex, &waitCV] {
            {
                std::lock_guard<std::mutex> lock(waitMutex);
                waitCV.notify_one();
            }

            EXPECT_GT(m_totalRefCount, 0);
            return m_exitCondition;
        });
    };

    for (unsigned int i = 0; i < threadMonikers.size(); i++) {
        auto threadMoniker = threadMonikers.at(i);
        auto thread = std::make_shared<std::thread>(threadLogic, threadMoniker);
        threads.push_back(thread);
    }

    {
        std::unique_lock<std::mutex> lock(waitMutex);
        ASSERT_TRUE(waitCV.wait_for(lock, LONG_TIMEOUT, [&waitingThreadPRs, numberOfThreads] {
            return waitingThreadPRs.size() == static_cast<unsigned int>(numberOfThreads);
        }));
    }

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        ASSERT_TRUE(waitingThreadPRs.size() == threads.size());
        ASSERT_TRUE(waitingThreadPRs.size() == threadMonikers.size());

        for (auto monikerToThreadPR : waitingThreadPRs) {
            EXPECT_TRUE(monikerToThreadPR.second->isFrozen());
        }
        EXPECT_EQ(m_totalRefCount, 0);
        m_exitCondition = true;
    }

    std::thread::id notifyingThreadId = std::this_thread::get_id();
    unsigned int acquiresOnNotifyingThread = 0;

    EXPECT_CALL(*m_powerManagerMock, acquire(_, _))
        .Times(AnyNumber())
        .WillRepeatedly(InvokeWithoutArgs([this, &acquiresOnNotifyingThread, notifyingThreadId] {
            m_totalRefCount++;
            if (std::this_thread::get_id() == notifyingThreadId) {
                acquiresOnNotifyingThread++;
            }

            return true;
        }));

    notify();

    for (auto thread : threads) {
        thread->join();
    }

    // Check that we have acquires on the notifying thread during NotifyOne/All call.
    EXPECT_GT(acquiresOnNotifyingThread, 0U);

    // Check that resources are thawed after wait.
    for (auto monikerToThreadPR : waitingThreadPRs) {
        auto threadPR = monikerToThreadPR.second;
        EXPECT_FALSE(threadPR->isFrozen());
    }

    // m_totalRefCount cannot be negative so this cast is safe.
    EXPECT_EQ(static_cast<unsigned int>(m_totalRefCount), waitingThreadPRs.size());
}

// ------------- NotifyOne

/// Test wait() succeeds.
TEST_P(ConditionVariableWrapperTest, test_wait_NoThreadPowerResource_NotifyOne_Succeeds) {
    auto testPredicate = [this] {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv->wait(lock, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_exitCondition = true;
    }

    m_cv->notifyOne();
    t1.join();
}

/// Test waitFor() succeeds and that no timeout happens. No thread PowerResource.
TEST_P(ConditionVariableWrapperTest, test_waitFor_NoThreadPowerResource_NotifyOne_Succeeds) {
    int elapsedMs = 0;

    auto testPredicate = [this, &elapsedMs] {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto start = std::chrono::steady_clock::now();
        m_waitReturn = m_cv->waitFor(lock, LONG_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
        auto stop = std::chrono::steady_clock::now();

        elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_exitCondition = true;
    }

    m_cv->notifyOne();
    t1.join();

    EXPECT_LE(elapsedMs, TIMEOUT_TOLERANCE.count());
    EXPECT_TRUE(m_waitReturn);
}

/// Test waitUntil() succeeds and that that timeout did not happen. No thread PowerResource.
TEST_P(ConditionVariableWrapperTest, test_waitUntil_NoThreadPowerResource_NotifyOne_Succeeds) {
    int elapsedMs = 0;

    auto testPredicate = [this, &elapsedMs] {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto start = std::chrono::steady_clock::now();
        m_waitReturn = m_cv->waitUntil(lock, start + LONG_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
        auto stop = std::chrono::steady_clock::now();

        elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_exitCondition = true;
    }

    m_cv->notifyOne();
    t1.join();

    EXPECT_LE(elapsedMs, TIMEOUT_TOLERANCE.count());
    EXPECT_TRUE(m_waitReturn);
}

/// Check that calling notifyOne with no threads waiting does not result in the resource being held.
TEST_P(ConditionVariableWrapperTest, test_notifyOne_NoWaitingThreads_PowerCheck) {
    m_cv->notifyOne();
    EXPECT_EQ(m_totalRefCount, 0);
}

/// Test wait() succeeds and that timeout did not happen. With thread PowerResource.
TEST_F(ConditionVariableWrapperPowerTest, test_wait_ThreadPowerResource_NotifyOne_Succeeds) {
    std::shared_ptr<PowerResource> waitingThreadPR;
    auto testPredicate = [this, &waitingThreadPR] {
        waitingThreadPR = PowerMonitor::getInstance()->getThreadPowerResourceOrCreate(TEST_THREAD_ID);
        waitingThreadPR->acquire();

        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv->wait(lock, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ASSERT_TRUE(waitingThreadPR);
        EXPECT_TRUE(waitingThreadPR->isFrozen());
        m_exitCondition = true;
    }

    m_cv->notifyOne();
    t1.join();

    EXPECT_EQ(m_totalRefCount, 1);
    EXPECT_FALSE(waitingThreadPR->isFrozen());
}

/// Test waitFor() succeeds and that that timeout did not happen. With thread PowerResource.
TEST_F(ConditionVariableWrapperPowerTest, test_waitFor_ThreadPowerResource_NotifyOne_Succeeds) {
    int elapsedMs = 0;
    std::shared_ptr<PowerResource> waitingThreadPR;

    auto testPredicate = [this, &waitingThreadPR, &elapsedMs] {
        waitingThreadPR = PowerMonitor::getInstance()->getThreadPowerResourceOrCreate(TEST_THREAD_ID);
        waitingThreadPR->acquire();

        std::unique_lock<std::mutex> lock(m_mutex);
        auto start = std::chrono::steady_clock::now();
        m_waitReturn = m_cv->waitFor(lock, LONG_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
        auto stop = std::chrono::steady_clock::now();

        elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ASSERT_TRUE(waitingThreadPR);
        EXPECT_TRUE(waitingThreadPR->isFrozen());
        m_exitCondition = true;
    }

    m_cv->notifyOne();
    t1.join();

    EXPECT_LE(elapsedMs, TIMEOUT_TOLERANCE.count());
    EXPECT_EQ(m_totalRefCount, 1);
    EXPECT_FALSE(waitingThreadPR->isFrozen());
    EXPECT_TRUE(m_waitReturn);
}

/// Test waitUntil() succeeds and that that timeout did not happen. With thread PowerResource.
TEST_F(ConditionVariableWrapperPowerTest, test_waitUntil_ThreadPowerResource_NotifyOne_Succeeds) {
    int elapsedMs = 0;
    std::shared_ptr<PowerResource> waitingThreadPR;

    auto testPredicate = [this, &waitingThreadPR, &elapsedMs] {
        waitingThreadPR = PowerMonitor::getInstance()->getThreadPowerResourceOrCreate(TEST_THREAD_ID);
        waitingThreadPR->acquire();

        std::unique_lock<std::mutex> lock(m_mutex);
        auto start = std::chrono::steady_clock::now();
        m_waitReturn = m_cv->waitUntil(lock, start + LONG_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
        auto stop = std::chrono::steady_clock::now();

        elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ASSERT_TRUE(waitingThreadPR);
        EXPECT_TRUE(waitingThreadPR->isFrozen());
        m_exitCondition = true;
    }

    m_cv->notifyOne();
    t1.join();

    EXPECT_LE(elapsedMs, TIMEOUT_TOLERANCE.count());
    EXPECT_EQ(m_totalRefCount, 1);
    EXPECT_FALSE(waitingThreadPR->isFrozen());
    EXPECT_TRUE(m_waitReturn);
}

/**
 * Test wait() succeeds with strict checks against PowerResourceManagerInterface to ensure we don't
 * accidentally enter a low power state.
 */
TEST_F(ConditionVariableWrapperPowerTest, test_wait_ThreadPowerResource_NotifyOne_PowerCheck_Succeeds) {
    const int NUM_THREADS = 1;
    notifyPowerCheck(NUM_THREADS, [this] {
        for (int i = 0; i < NUM_THREADS; i++) {
            m_cv->notifyOne();
        }
    });
}

/**
 * Test wait() succeeds with strict checks against PowerResourceManagerInterface to ensure we don't accidentally enter a
 * low power state.
 *
 * Call notifyOne() in succession on each thread.
 */
TEST_F(ConditionVariableWrapperPowerTest, test_wait_ThreadPowerResource_NotifyOne_PowerCheckMultiple_Succeeds) {
    const int NUM_THREADS = 3;
    notifyPowerCheck(NUM_THREADS, [this] {
        for (int i = 0; i < NUM_THREADS; i++) {
            m_cv->notifyOne();
        }
    });
}

/**
 * Test when notifyOne() is called and the predicate is false. Expect the resource to be frozen.
 * This test uses strict timing because we want to measure the state after notifyOne() is called,
 * the predicate has been evaluated to false, and the thread goes back to being blocked. In this state,
 * we verify if the resource has been frozen.
 */
TEST_F(
    ConditionVariableWrapperPowerTest,
    test_wait_ThreadPowerResource_NotifyOne_PowerCheckFalsePredicate_ReleasesPower) {
    WaitEvent waitForNotifyOne;
    bool notifyOneCalled = false;

    std::shared_ptr<PowerResource> waitingThreadPR;
    auto threadLogic = [this, &waitingThreadPR, &waitForNotifyOne, &notifyOneCalled](std::string moniker) {
        waitingThreadPR = PowerMonitor::getInstance()->getThreadPowerResourceOrCreate(moniker);
        waitingThreadPR->acquire();

        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv->wait(lock, [this, &waitForNotifyOne, &notifyOneCalled] {
            m_enteredWaiting.wakeUp();
            if (notifyOneCalled) {
                waitForNotifyOne.wakeUp();
            }
            EXPECT_GT(m_totalRefCount, 0);
            return m_exitCondition;
        });
    };

    std::thread t1(threadLogic, TEST_THREAD_ID);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        EXPECT_EQ(m_totalRefCount, 0);
        notifyOneCalled = true;
    }

    m_cv->notifyOne();
    waitForNotifyOne.wait(LONG_TIMEOUT);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ASSERT_TRUE(waitingThreadPR);
        EXPECT_EQ(waitingThreadPR->isFrozen(), true);
        m_exitCondition = true;
    }

    m_cv->notifyOne();
    t1.join();
}

// ------------- NotifyAll

/// Test wait() succeeds.
TEST_P(ConditionVariableWrapperTest, test_wait_NoThreadPowerResource_NotifyAll_Succeeds) {
    auto testPredicate = [this] {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv->wait(lock, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_exitCondition = true;
    }

    m_cv->notifyAll();
    t1.join();
}

/// Test waitFor() succeeds and that timeout did not happen. No thread PowerResource.
TEST_P(ConditionVariableWrapperTest, test_waitFor_NoThreadPowerResource_NotifyAll_Succeeds) {
    int elapsedMs = 0;

    auto testPredicate = [this, &elapsedMs] {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto start = std::chrono::steady_clock::now();
        m_waitReturn = m_cv->waitFor(lock, LONG_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
        auto stop = std::chrono::steady_clock::now();

        elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_exitCondition = true;
    }

    m_cv->notifyAll();
    t1.join();

    EXPECT_LE(elapsedMs, TIMEOUT_TOLERANCE.count());
    EXPECT_TRUE(m_waitReturn);
}

/// Test waitUntil() succeeds and that that timeout did not happen. No thread PowerResource.
TEST_P(ConditionVariableWrapperTest, test_waitUntil_NoThreadPowerResource_NotifyAll_Succeeds) {
    int elapsedMs = 0;

    auto testPredicate = [this, &elapsedMs] {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto start = std::chrono::steady_clock::now();
        m_waitReturn = m_cv->waitUntil(lock, start + LONG_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
        auto stop = std::chrono::steady_clock::now();

        elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_exitCondition = true;
    }

    m_cv->notifyAll();
    t1.join();

    EXPECT_LE(elapsedMs, TIMEOUT_TOLERANCE.count());
    EXPECT_TRUE(m_waitReturn);
}

/// Check that calling notifyOne with no threads waiting does not result in the resource being held.
TEST_P(ConditionVariableWrapperTest, test_notifyAll_noWaitingThreads_PowerCheck) {
    m_cv->notifyAll();
    EXPECT_EQ(m_totalRefCount, 0);
}

/// Test wait() succeeds and that that timeout did not happen. With thread PowerResource.
TEST_F(ConditionVariableWrapperPowerTest, test_wait_ThreadPowerResource_NotifyAll_Succeeds) {
    std::shared_ptr<PowerResource> waitingThreadPR;
    auto testPredicate = [this, &waitingThreadPR] {
        waitingThreadPR = PowerMonitor::getInstance()->getThreadPowerResourceOrCreate(TEST_THREAD_ID);
        waitingThreadPR->acquire();

        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv->wait(lock, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ASSERT_TRUE(waitingThreadPR);
        EXPECT_TRUE(waitingThreadPR->isFrozen());
        m_exitCondition = true;
    }

    m_cv->notifyAll();
    t1.join();

    EXPECT_EQ(m_totalRefCount, 1);
    EXPECT_FALSE(waitingThreadPR->isFrozen());
}

/// Test waitUntil() succeeds and that timeout did not happen. With thread PowerResource.
TEST_F(ConditionVariableWrapperPowerTest, test_waitFor_ThreadPowerResource_NotifyAll_Succeeds) {
    int elapsedMs = 0;
    std::shared_ptr<PowerResource> waitingThreadPR;

    auto testPredicate = [this, &waitingThreadPR, &elapsedMs] {
        waitingThreadPR = PowerMonitor::getInstance()->getThreadPowerResourceOrCreate(TEST_THREAD_ID);
        waitingThreadPR->acquire();

        std::unique_lock<std::mutex> lock(m_mutex);
        auto start = std::chrono::steady_clock::now();
        m_waitReturn = m_cv->waitFor(lock, LONG_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
        auto stop = std::chrono::steady_clock::now();

        elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ASSERT_TRUE(waitingThreadPR);
        EXPECT_TRUE(waitingThreadPR->isFrozen());
        m_exitCondition = true;
    }

    m_cv->notifyAll();
    t1.join();

    EXPECT_LE(elapsedMs, TIMEOUT_TOLERANCE.count());
    EXPECT_EQ(m_totalRefCount, 1);
    EXPECT_FALSE(waitingThreadPR->isFrozen());
    EXPECT_TRUE(m_waitReturn);
}

/// Test waitUntil() succeeds and that that timeout did not happen. With thread PowerResource.
TEST_F(ConditionVariableWrapperPowerTest, test_waitUntil_ThreadPowerResource_NotifyAll_Succeeds) {
    int elapsedMs = 0;
    std::shared_ptr<PowerResource> waitingThreadPR;

    auto testPredicate = [this, &waitingThreadPR, &elapsedMs] {
        waitingThreadPR = PowerMonitor::getInstance()->getThreadPowerResourceOrCreate(TEST_THREAD_ID);
        waitingThreadPR->acquire();

        std::unique_lock<std::mutex> lock(m_mutex);
        auto start = std::chrono::steady_clock::now();
        m_waitReturn = m_cv->waitUntil(lock, start + LONG_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
        auto stop = std::chrono::steady_clock::now();

        elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ASSERT_TRUE(waitingThreadPR);
        EXPECT_TRUE(waitingThreadPR->isFrozen());
        m_exitCondition = true;
    }

    m_cv->notifyAll();
    t1.join();

    EXPECT_LE(elapsedMs, TIMEOUT_TOLERANCE.count());
    EXPECT_EQ(m_totalRefCount, 1);
    EXPECT_FALSE(waitingThreadPR->isFrozen());
}

/**
 * Test wait() succeeds with strict checks against PowerResourceManagerInterface to ensure we don't accidentally
 * enter a low power state. Single blocked thread.
 */
TEST_F(ConditionVariableWrapperPowerTest, test_wait_ThreadPowerResource_NotifyAll_PowerCheck_Succeeds) {
    notifyPowerCheck(1, [this] { m_cv->notifyAll(); });
}

/**
 * Test wait() succeeds with strict checks against PowerResourceManagerInterface to ensure we don't accidentally
 * enter a low power state. Multiple blocked threads.
 */
TEST_F(ConditionVariableWrapperPowerTest, test_wait_ThreadPowerResource_NotifyAll_PowerCheckMultiple_Succeeds) {
    notifyPowerCheck(3, [this] { m_cv->notifyAll(); });
}

/**
 * Test when notifyAll() is called and the predicate is false. Expect the resource to be frozen.
 * This test uses strict timing because we want to measure the state after notifyAll() is called,
 * the predicate has been evaluated to false, and the thread goes back to being blocked. In this state,
 * we verify if the resource has been frozen.
 */
TEST_F(
    ConditionVariableWrapperPowerTest,
    test_wait_ThreadPowerResource_NotifyAll_PowerCheckFalsePredicate_ReleasesPower) {
    WaitEvent waitForNotifyAll;
    bool notifyAllCalled = false;

    std::shared_ptr<PowerResource> waitingThreadPR;
    auto threadLogic = [this, &waitingThreadPR, &waitForNotifyAll, &notifyAllCalled](std::string moniker) {
        waitingThreadPR = PowerMonitor::getInstance()->getThreadPowerResourceOrCreate(moniker);
        waitingThreadPR->acquire();

        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv->wait(lock, [this, &waitForNotifyAll, &notifyAllCalled] {
            m_enteredWaiting.wakeUp();
            if (notifyAllCalled) {
                waitForNotifyAll.wakeUp();
            }
            EXPECT_GT(m_totalRefCount, 0);
            return m_exitCondition;
        });
    };

    std::thread t1(threadLogic, TEST_THREAD_ID);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        EXPECT_EQ(m_totalRefCount, 0);
        notifyAllCalled = true;
    }

    m_cv->notifyAll();
    waitForNotifyAll.wait(LONG_TIMEOUT);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ASSERT_TRUE(waitingThreadPR);
        EXPECT_EQ(waitingThreadPR->isFrozen(), true);
        m_exitCondition = true;
    }

    m_cv->notifyAll();
    t1.join();
}

// ------------- NoNotify

/// Test waitFor() times out and returns false. Test no deadlocks. No thread PowerResource.
TEST_P(ConditionVariableWrapperTest, testTimer_waitFor_NoThreadPowerResource_NoNotify_ReturnsFalse) {
    auto testPredicate = [this] {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_waitReturn = m_cv->waitFor(lock, SHORT_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));
    t1.join();

    EXPECT_FALSE(m_waitReturn);
}

/// Test waitUntil() times out and returns false. Test no deadlocks. No thread PowerResource.
TEST_P(ConditionVariableWrapperTest, testTimer_waitUntil_NoThreadPowerResource_NoNotify_ReturnsFalse) {
    auto testPredicate = [this] {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_waitReturn = m_cv->waitUntil(lock, std::chrono::steady_clock::now() + SHORT_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    m_cv->notifyOne();
    t1.join();

    EXPECT_FALSE(m_waitReturn);
}

/// Test waitFor() times out and returns false. Test no deadlocks. No Power Monitor.
TEST_P(ConditionVariableWrapperTest, testTimer_waitFor_NoPowerMonitor_NoNotify_ReturnsFalse) {
    PowerMonitor::getInstance()->deactivate();
    m_cv = std::make_shared<ConditionVariableWrapper>();

    auto testPredicate = [this] {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_waitReturn = m_cv->waitFor(lock, SHORT_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));
    t1.join();

    EXPECT_FALSE(m_waitReturn);
}

/// Test waitUntil() times out and returns false. Test no deadlocks. No Power Monitor.
TEST_P(ConditionVariableWrapperTest, testTimer_waitUntil_NoPowerMonitor_NoNotify_ReturnsFalse) {
    PowerMonitor::getInstance()->deactivate();
    m_cv = std::make_shared<ConditionVariableWrapper>();

    auto testPredicate = [this] {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_waitReturn = m_cv->waitUntil(lock, std::chrono::steady_clock::now() + SHORT_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));
    t1.join();

    EXPECT_FALSE(m_waitReturn);
}

/// Test waitFor() times out and returns false. No deadlocks. With thread PowerResource.
TEST_F(ConditionVariableWrapperPowerTest, testTimer_waitFor_ThreadPowerResource_NoNotify_ReturnsFalse) {
    std::shared_ptr<PowerResource> waitingThreadPR;

    auto testPredicate = [this, &waitingThreadPR] {
        waitingThreadPR = PowerMonitor::getInstance()->getThreadPowerResourceOrCreate(TEST_THREAD_ID);
        waitingThreadPR->acquire();

        std::unique_lock<std::mutex> lock(m_mutex);
        m_waitReturn = m_cv->waitFor(lock, SHORT_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ASSERT_TRUE(waitingThreadPR);
        EXPECT_TRUE(waitingThreadPR->isFrozen());
    }

    t1.join();

    EXPECT_FALSE(waitingThreadPR->isFrozen());
    EXPECT_FALSE(m_waitReturn);
}

/// Test waitFor() times out returns true if the predicate is true. No deadlocks. With thread PowerResource.
TEST_F(ConditionVariableWrapperPowerTest, testTimer_waitFor_ThreadPowerResource_NoNotifySuccessfulPred_ReturnsTrue) {
    std::shared_ptr<PowerResource> waitingThreadPR;

    auto testPredicate = [this, &waitingThreadPR] {
        waitingThreadPR = PowerMonitor::getInstance()->getThreadPowerResourceOrCreate(TEST_THREAD_ID);
        waitingThreadPR->acquire();

        std::unique_lock<std::mutex> lock(m_mutex);
        m_waitReturn = m_cv->waitFor(lock, SHORT_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ASSERT_TRUE(waitingThreadPR);
        EXPECT_TRUE(waitingThreadPR->isFrozen());
        m_exitCondition = true;
    }

    t1.join();

    EXPECT_FALSE(waitingThreadPR->isFrozen());
    EXPECT_TRUE(m_waitReturn);
}

/// Test waitUntil() times out and returns false. No deadlocks. With thread PowerResource.
TEST_F(ConditionVariableWrapperPowerTest, testTimer_waitUntil_ThreadPowerResource_NoNotify_ReturnsFalse) {
    std::shared_ptr<PowerResource> waitingThreadPR;

    auto testPredicate = [this, &waitingThreadPR] {
        waitingThreadPR = PowerMonitor::getInstance()->getThreadPowerResourceOrCreate(TEST_THREAD_ID);
        waitingThreadPR->acquire();

        std::unique_lock<std::mutex> lock(m_mutex);
        m_waitReturn = m_cv->waitUntil(lock, std::chrono::steady_clock::now() + SHORT_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ASSERT_TRUE(waitingThreadPR);
        EXPECT_TRUE(waitingThreadPR->isFrozen());
    }

    t1.join();

    EXPECT_FALSE(waitingThreadPR->isFrozen());
    EXPECT_FALSE(m_waitReturn);
}

/// Test waitUntil() times out returns true if the predicate is true. No deadlocks. With thread PowerResource.
TEST_F(ConditionVariableWrapperPowerTest, testTimer_waitUntil_ThreadPowerResource_NoNotifySuccessfulPred_ReturnsTrue) {
    std::shared_ptr<PowerResource> waitingThreadPR;

    auto testPredicate = [this, &waitingThreadPR] {
        waitingThreadPR = PowerMonitor::getInstance()->getThreadPowerResourceOrCreate(TEST_THREAD_ID);
        waitingThreadPR->acquire();

        std::unique_lock<std::mutex> lock(m_mutex);
        m_waitReturn = m_cv->waitUntil(lock, std::chrono::steady_clock::now() + SHORT_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ASSERT_TRUE(waitingThreadPR);
        EXPECT_TRUE(waitingThreadPR->isFrozen());
        m_exitCondition = true;
    }

    t1.join();

    EXPECT_FALSE(waitingThreadPR->isFrozen());
    EXPECT_TRUE(m_waitReturn);
}

/// Test the race condition that notifyAll is called at the same time the Timer fires.
/// This is difficult to time correctly, so a modified TimerDelegate object is used.
TEST_F(ConditionVariableWrapperPowerTest, testTimer_timerTriggersWithPredicate_ReturnsTrue) {
    auto primitivesProvider = avs::initialization::SDKPrimitivesProvider::getInstance();
    primitivesProvider->withTimerDelegateFactory(std::make_shared<timing::test::StopTaskTimerDelegateFactory>());
    ASSERT_TRUE(primitivesProvider->initialize());

    std::shared_ptr<ConditionVariableWrapper> cv = std::make_shared<ConditionVariableWrapper>();
    int elapsedMs = 0;

    std::shared_ptr<PowerResource> waitingThreadPR;
    auto testPredicate = [this, &elapsedMs, cv, &waitingThreadPR] {
        waitingThreadPR = PowerMonitor::getInstance()->getThreadPowerResourceOrCreate(TEST_THREAD_ID);
        std::unique_lock<std::mutex> lock(m_mutex);
        auto start = std::chrono::steady_clock::now();
        m_waitReturn = cv->waitUntil(lock, start + LONG_TIMEOUT, [this] {
            m_enteredWaiting.wakeUp();
            return m_exitCondition;
        });
        auto stop = std::chrono::steady_clock::now();

        elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    };

    std::thread t1(testPredicate);
    EXPECT_TRUE(m_enteredWaiting.wait(LONG_TIMEOUT));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_exitCondition = true;
    }

    cv->notifyAll();
    t1.join();
    primitivesProvider->terminate();

    EXPECT_LE(elapsedMs, TIMEOUT_TOLERANCE.count());
    EXPECT_TRUE(m_waitReturn);
}

int main(int argc, char** argv) {
    return 1;
}

}  // namespace test
}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif
