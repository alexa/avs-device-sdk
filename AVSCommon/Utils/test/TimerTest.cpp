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

/// @file TimerTest.cpp

#include <deque>
#include <gtest/gtest.h>

#include "AVSCommon/Utils/Timing/Timer.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {
namespace test {

/// Specifies the expected timing accuracy (timestamps must be within +/- ACCURACY of expected values).
#ifdef _WIN32
static const auto ACCURACY = std::chrono::milliseconds(30);
#else
static const auto ACCURACY = std::chrono::milliseconds(15);
#endif

/// Used for cases where the task should return immediately, without delay.
static const auto NO_DELAY = std::chrono::milliseconds(0);

/// Used for cases where the task duration or timer period should be shorter than MEDIUM_DELAY and LONG_DELAY.
static const auto SHORT_DELAY = ACCURACY * 2;

/// Used for cases where the task duration or timer period should be greater than SHORT_DELAY and less than LONG_DELAY.
static const auto MEDIUM_DELAY = SHORT_DELAY + ACCURACY;

/**
 * Used for cases where the task duration or timer period should be greater than MEDIUM_DELAY, and greater than two
 * SHORT_DELAYs.
 */
static const auto LONG_DELAY = SHORT_DELAY * 2 + ACCURACY;

/**
 * Used to limit the amount of time tests will wait for an operation to finish.  This timeout will only be hit if a
 * test is failing.
 */
static const auto TIMEOUT = std::chrono::seconds(1);

/// Number of task iterations to run for multi-shot tests
static const size_t ITERATIONS = 5;

/// Test harness for Timer class.
class TimerTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

    /**
     * A simple task to test Timer instances with.  This task records its start time in @c m_timestamps and then waits
     * for the specified duration before returning.
     *
     * @param duration The number of milliseconds to sleep before returning.
     */
    void simpleTask(std::chrono::milliseconds duration);

protected:
    /**
     * Utility function which verifies the size and values of @c m_timestamps after runnning a timer.
     *
     * @param startTime The start time for a timer, against which @c m_timestamps should be verified.
     * @param period The period of the timer.
     * @param periodType The type of period used for the timer.
     * @param duration The runtime duration of the timer task.
     * @param iterations The expected number of task calls made by the timer.
     */
    void verifyTimestamps(
        std::chrono::time_point<std::chrono::steady_clock> startTime,
        std::chrono::milliseconds delay,
        std::chrono::milliseconds period,
        Timer::PeriodType periodType,
        std::chrono::milliseconds duration,
        size_t iterations = 1);

    /**
     * Utility function which polls for a timer to go inactive.  This function will time out and fail if the timer
     * does not go inactive within SHORT_DELAY.
     *
     * @return @c true if timer is inactive, else @c false.
     */
    bool waitForInactive();

    /// Mutex which protects access to @c m_timestamps.
    std::mutex m_mutex;

    /// Condition variable to notify about changes to @c m_timestamps.
    std::condition_variable m_conditionVariable;

    /// This deque accumulates timestamps from timer calls to simpleTask().
    std::deque<std::chrono::time_point<std::chrono::steady_clock>> m_timestamps;

    /// The timer to use in the tests.
    std::unique_ptr<Timer> m_timer;
};

void TimerTest::SetUp() {
    m_timer = std::unique_ptr<Timer>(new Timer);
}

void TimerTest::simpleTask(std::chrono::milliseconds duration) {
    auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_timestamps.push_back(now);
        m_conditionVariable.notify_all();
    }
    std::this_thread::sleep_for(duration);
}

void TimerTest::verifyTimestamps(
    std::chrono::time_point<std::chrono::steady_clock> startTime,
    std::chrono::milliseconds delay,
    std::chrono::milliseconds period,
    Timer::PeriodType periodType,
    std::chrono::milliseconds duration,
    size_t iterations) {
    // For ABSOLUTE periods, the actual number of task calls may be less than iterations if duration exceeds
    // period.
    size_t expectedTaskCalls = iterations;
    if (periodType == Timer::PeriodType::ABSOLUTE) {
        size_t callsSkippedPerIteration = duration / period;
        expectedTaskCalls = (iterations - 1) / (callsSkippedPerIteration + 1) + 1;
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    ASSERT_TRUE(m_conditionVariable.wait_for(
        lock, TIMEOUT, [this, expectedTaskCalls] { return m_timestamps.size() == expectedTaskCalls; }));

    // First timestamp should always occur after an elapsed time of delay.
    auto timestamp = m_timestamps.begin();
    auto elapsed = delay;
    auto taskRuntimeRemaining = std::chrono::milliseconds::zero();

    for (size_t i = 0; i < iterations; ++i) {
        // Only check timestamps if the task would have finished by this iteration.
        if (taskRuntimeRemaining <= std::chrono::milliseconds::zero()) {
            // Make sure this task call started on schedule.
            auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(*timestamp - startTime);
            EXPECT_GE(delta, elapsed - ACCURACY);
            EXPECT_LE(delta, elapsed + ACCURACY);

            // For ABSOLUTE periods, note how much task runtime remains after the first period.
            if (periodType == Timer::PeriodType::ABSOLUTE) {
                taskRuntimeRemaining = duration - period;
            }

            // For RELATIVE periods, the cadence can vary, and advances relative to the previous task
            // end (start + duration) by one period.
            if (periodType == Timer::PeriodType::RELATIVE) {
                elapsed = delta + duration + period;
            }

            // We're done with this timestamp.
            ++timestamp;
        } else {
            // Update task runtime after another period has elapsed.
            taskRuntimeRemaining -= period;
        }

        // For ABSOLUTE periods, the cadence is fixed, and consistently advances by one period.
        if (periodType == Timer::PeriodType::ABSOLUTE) {
            elapsed += period;
        }
    }

    // note: can't ASSERT_EQ on iterators with older GCC versions - see https://github.com/google/googletest/issues/742
    bool timestampEqualsEnd = (timestamp == m_timestamps.end());
    ASSERT_TRUE(timestampEqualsEnd);
}

bool TimerTest::waitForInactive() {
    auto t0 = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - t0 < TIMEOUT) {
        if (!m_timer->isActive()) {
            return true;
        }
        std::this_thread::sleep_for(SHORT_DELAY);
    }
    return false;
}

/// This test runs a single-shot timer and verifies that the task is called once, at the expected time.
TEST_F(TimerTest, singleShot) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_EQ(
        m_timer->start(SHORT_DELAY, std::bind(&TimerTest::simpleTask, this, NO_DELAY)).wait_for(TIMEOUT),
        std::future_status::ready);
    ASSERT_TRUE(waitForInactive());
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, NO_DELAY);
}

/**
 * This test runs a multi-shot ABSOLUTE timer and verifies that the task is called the expected number of times,
 * and that each call occurred at the expected time.
 */
TEST_F(TimerTest, multiShot) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_timer->start(
        SHORT_DELAY, Timer::PeriodType::ABSOLUTE, ITERATIONS, std::bind(&TimerTest::simpleTask, this, NO_DELAY)));
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, NO_DELAY, ITERATIONS);
    ASSERT_TRUE(waitForInactive());
}

/**
 * This test runs a multi-shot ABSOLUTE timer with different initial delay and verifies that the task is called
 * the expected number of times, and that each call occurred at the expected time.
 */
TEST_F(TimerTest, multiShotWithDelay) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_timer->start(
        MEDIUM_DELAY,
        SHORT_DELAY,
        Timer::PeriodType::ABSOLUTE,
        ITERATIONS,
        std::bind(&TimerTest::simpleTask, this, NO_DELAY)));
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, MEDIUM_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, NO_DELAY, ITERATIONS);
    ASSERT_TRUE(waitForInactive());
}

/**
 * This test runs a continuous ABSOLUTE timer, verifies that it remains active through several iterations,
 * verifies that it stops when requested, and verifies that the expected number of calls occurred at their expected
 * times.
 */
TEST_F(TimerTest, forever) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_timer->start(
        SHORT_DELAY, Timer::PeriodType::ABSOLUTE, Timer::FOREVER, std::bind(&TimerTest::simpleTask, this, NO_DELAY)));
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, NO_DELAY, ITERATIONS);
    ASSERT_TRUE(m_timer->isActive());
    m_timer->stop();
    ASSERT_TRUE(waitForInactive());
}

/**
 * This test runs a slow task with an ABSOLUTE timer, but one which completes in less than a period, and verifies
 * that the slow task does not change the number of calls or their period.
 */
TEST_F(TimerTest, slowTaskLessThanPeriod) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_timer->start(
        MEDIUM_DELAY, Timer::PeriodType::ABSOLUTE, ITERATIONS, std::bind(&TimerTest::simpleTask, this, SHORT_DELAY)));
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, MEDIUM_DELAY, MEDIUM_DELAY, Timer::PeriodType::ABSOLUTE, SHORT_DELAY, ITERATIONS);
}

/**
 * This test runs a slow task with an ABSOLUTE timer which does not complete within a period, and verifies that
 * the slow task results in skipped calls, but on a consistent period.
 */
TEST_F(TimerTest, slowTaskGreaterThanPeriod) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_timer->start(
        SHORT_DELAY, Timer::PeriodType::ABSOLUTE, ITERATIONS, std::bind(&TimerTest::simpleTask, this, MEDIUM_DELAY)));
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, MEDIUM_DELAY, ITERATIONS);
}

/**
 * This test runs a slow task with an ABSOLUTE timer which does not complete within multiple periods, and verifies
 * that the slow task results in skipped calls, but on a consistent period.
 */
TEST_F(TimerTest, slowTaskGreaterThanTwoPeriods) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_timer->start(
        SHORT_DELAY, Timer::PeriodType::ABSOLUTE, ITERATIONS, std::bind(&TimerTest::simpleTask, this, LONG_DELAY)));
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, LONG_DELAY, ITERATIONS);
}

/**
 * This test runs a slow task with a RELATIVE timer which does not complete within a period, and verifies that
 * the slow task does not result in skipped calls, but calls have a consistent between-task delay.
 */
TEST_F(TimerTest, endToStartPeriod) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_timer->start(
        SHORT_DELAY, Timer::PeriodType::RELATIVE, ITERATIONS, std::bind(&TimerTest::simpleTask, this, MEDIUM_DELAY)));
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::RELATIVE, MEDIUM_DELAY, ITERATIONS);
}

/**
 * This test verifies that a call to stop() before the task is called results in an inactive timer which does not
 * execute the task.
 */
TEST_F(TimerTest, stopSingleShotBeforeTask) {
    ASSERT_TRUE(m_timer->start(MEDIUM_DELAY, std::bind(&TimerTest::simpleTask, this, NO_DELAY)).valid());
    ASSERT_TRUE(m_timer->isActive());
    std::this_thread::sleep_for(SHORT_DELAY);
    ASSERT_TRUE(m_timer->isActive());
    m_timer->stop();
    ASSERT_TRUE(waitForInactive());
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        ASSERT_TRUE(m_timestamps.empty());
    }
}

/**
 * This test verifies that a call to stop() while a task is executing results in an inactive timer after that task
 * finishes executing.
 */
TEST_F(TimerTest, stopSingleShotDuringTask) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_timer->start(SHORT_DELAY, std::bind(&TimerTest::simpleTask, this, SHORT_DELAY)).valid());
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, SHORT_DELAY);
    ASSERT_TRUE(m_timer->isActive());
    m_timer->stop();
    ASSERT_TRUE(waitForInactive());
}

/**
 * This test verifies that a call to stop() after a task has finished executing leaves the timer inactive and
 * unchanged.
 */
TEST_F(TimerTest, stopSingleShotAfterTask) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_timer->start(SHORT_DELAY, std::bind(&TimerTest::simpleTask, this, SHORT_DELAY)).valid());
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, SHORT_DELAY);
    std::this_thread::sleep_for(SHORT_DELAY + SHORT_DELAY / 2);
    ASSERT_TRUE(waitForInactive());
    m_timer->stop();
    ASSERT_TRUE(waitForInactive());
}

/**
 * This test verifies that a call to stop() on a multi-shot timer results in an inactive timer with the expected
 * number of calls.
 */
TEST_F(TimerTest, stopMultiShot) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_timer->start(
        SHORT_DELAY, Timer::PeriodType::ABSOLUTE, ITERATIONS, std::bind(&TimerTest::simpleTask, this, NO_DELAY)));
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, NO_DELAY, ITERATIONS - 1);
    ASSERT_TRUE(m_timer->isActive());
    m_timer->stop();
    ASSERT_TRUE(waitForInactive());
}

/**
 * This test verifies that a call to start() fails on a timer which is active but has not called a task yet, but does
 * not interfere with the previously scheduled task.
 */
TEST_F(TimerTest, startRunningBeforeTask) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_timer->start(SHORT_DELAY, std::bind(&TimerTest::simpleTask, this, NO_DELAY)).valid());
    ASSERT_TRUE(m_timer->isActive());
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        ASSERT_EQ(m_timestamps.size(), 0U);
    }
    ASSERT_FALSE(m_timer->start(LONG_DELAY, std::bind(&TimerTest::simpleTask, this, NO_DELAY)).valid());
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, NO_DELAY);
    ASSERT_TRUE(waitForInactive());
}

/**
 * This test verifies that a call to start() fails on a timer which is currently executing a task, but does
 * not interfere with that task.
 */
TEST_F(TimerTest, startRunningDuringTask) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_timer->start(SHORT_DELAY, std::bind(&TimerTest::simpleTask, this, SHORT_DELAY)).valid());
    ASSERT_TRUE(m_timer->isActive());
    std::this_thread::sleep_for(SHORT_DELAY + SHORT_DELAY / 2);
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, NO_DELAY);
    ASSERT_FALSE(m_timer->start(LONG_DELAY, std::bind(&TimerTest::simpleTask, this, NO_DELAY)).valid());
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, NO_DELAY);
}

/**
 * This test verifies that a call to start() succeeds on a timer which was previously used to run a task, but which is
 * inactive at the time the new start() is called.
 */
TEST_F(TimerTest, startRunningAfterTask) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_timer->start(SHORT_DELAY, std::bind(&TimerTest::simpleTask, this, NO_DELAY)).valid());
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, NO_DELAY);
    ASSERT_TRUE(waitForInactive());
    m_timestamps.clear();
    auto t1 = std::chrono::steady_clock::now();
    ASSERT_EQ(
        m_timer->start(MEDIUM_DELAY, std::bind(&TimerTest::simpleTask, this, NO_DELAY)).wait_for(TIMEOUT),
        std::future_status::ready);
    ASSERT_TRUE(waitForInactive());
    verifyTimestamps(t1, MEDIUM_DELAY, MEDIUM_DELAY, Timer::PeriodType::ABSOLUTE, NO_DELAY);
}

/// This test verifies that a timer which is deleted while active, but before running its task, does not run the task.
TEST_F(TimerTest, deleteBeforeTask) {
    ASSERT_TRUE(m_timer->start(SHORT_DELAY, std::bind(&TimerTest::simpleTask, this, SHORT_DELAY)).valid());
    ASSERT_TRUE(m_timer->isActive());
    m_timer.reset();
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        ASSERT_EQ(m_timestamps.size(), 0U);
    }
}

/// This test verifies that a timer which is deleted while running a task completes the task.
TEST_F(TimerTest, deleteDuringTask) {
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(m_timer->start(
        SHORT_DELAY,
        Timer::PeriodType::ABSOLUTE,
        Timer::FOREVER,
        std::bind(&TimerTest::simpleTask, this, SHORT_DELAY)));
    ASSERT_TRUE(m_timer->isActive());
    std::this_thread::sleep_for(SHORT_DELAY + SHORT_DELAY / 2);
    ASSERT_TRUE(m_timer->isActive());
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, SHORT_DELAY);
    m_timer.reset();
    verifyTimestamps(t0, SHORT_DELAY, SHORT_DELAY, Timer::PeriodType::ABSOLUTE, SHORT_DELAY);
}

/**
 * This test verifies that a call to start() succeeds on a timer which was previously stopped while running a task, but
 * which is inactive at the time the new start() is called.
 */
TEST_F(TimerTest, startRunningAfterStopDuringTask) {
    ASSERT_TRUE(m_timer->start(NO_DELAY, std::bind(&TimerTest::simpleTask, this, MEDIUM_DELAY)).valid());
    ASSERT_TRUE(m_timer->isActive());
    std::this_thread::sleep_for(SHORT_DELAY);
    m_timer->stop();
    ASSERT_TRUE(waitForInactive());
    m_timestamps.clear();
    auto t0 = std::chrono::steady_clock::now();
    ASSERT_EQ(
        m_timer->start(MEDIUM_DELAY, std::bind(&TimerTest::simpleTask, this, NO_DELAY)).wait_for(TIMEOUT),
        std::future_status::ready);
    ASSERT_TRUE(waitForInactive());
    verifyTimestamps(t0, MEDIUM_DELAY, MEDIUM_DELAY, Timer::PeriodType::ABSOLUTE, NO_DELAY);
}

}  // namespace test
}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
