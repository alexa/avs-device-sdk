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
#include <thread>

#include <gtest/gtest.h>

#include <AVSCommon/Utils/Timing/TimerDelegate.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {
namespace test {

using namespace std;
using namespace std::chrono;
using namespace alexaClientSDK::avsCommon::sdkInterfaces::timing;
using namespace ::testing;

class TimerDelegateTest : public Test {
public:
    /// SetUp for the test.
    void SetUp() override;

    /// TearDown for the test.
    void TearDown() override;

    /// Simple timer task that increments the task counter.
    void testTask();

    /**
     * Get the current value of the task counter.
     * @return Value of the task counter.
     */
    size_t getTaskCounter();

    /**
     * Task with fixed duration emulated as a sleep.
     * @param sleepDuration duration emulating the task in milliseconds
     */
    void testTaskWithSleep(milliseconds sleepDuration);

    /**
     * Task that calls an internal stop to the timer delegate.
     * @param timer @c TimerDelegateInterface used to call stop internally.
     */
    void taskWithStop(TimerDelegateInterface* timer);

    /**
     * Task with customizable task durations across task iterations.
     * @param taskTimes vector containing task durations in milliseconds across
     *        task iterations.
     */
    void customVariableDurationTask(vector<milliseconds> taskTimes);

protected:
    /// @c TimerDelegate under test.
    TimerDelegate m_timerDelegate;

private:
    /// Counter to track number of invocations of the timer task.
    atomic<size_t> m_taskCounter;
};

void TimerDelegateTest::SetUp() {
    m_taskCounter = 0;
}

void TimerDelegateTest::TearDown() {
    m_timerDelegate.stop();
}

void TimerDelegateTest::testTask() {
    m_taskCounter++;
}

size_t TimerDelegateTest::getTaskCounter() {
    return m_taskCounter;
}

void TimerDelegateTest::testTaskWithSleep(milliseconds sleepDuration) {
    this_thread::sleep_for(sleepDuration);
    m_taskCounter++;
}

void TimerDelegateTest::taskWithStop(TimerDelegateInterface* timer) {
    m_taskCounter++;
    timer->stop();
}

void TimerDelegateTest::customVariableDurationTask(vector<chrono::milliseconds> taskTimes) {
    if (!taskTimes.empty() && (m_taskCounter < taskTimes.size())) {
        this_thread::sleep_for(taskTimes[m_taskCounter]);
    }
    m_taskCounter++;
}

/**
 * Test to verify basic APIs to activate, start, and ensure that the TimerDelegate triggered the expected number of
 * times.
 */
TEST_F(TimerDelegateTest, test_basicTimerDelegateAPI) {
    auto delay = milliseconds(100);
    auto period = milliseconds(500);
    auto maxCount = 2u;
    auto graceTime = milliseconds(50);

    m_timerDelegate.start(
        delay, period, TimerDelegateInterface::PeriodType::ABSOLUTE, maxCount, [this] { testTask(); });

    /// Check after first task call.
    this_thread::sleep_for(delay + graceTime);
    ASSERT_EQ(getTaskCounter(), 1u);
    ASSERT_TRUE(m_timerDelegate.isActive());

    /// Sleep until the timer completes all iterations.
    this_thread::sleep_for((maxCount - 1) * period + graceTime);
    ASSERT_EQ(getTaskCounter(), maxCount);

    /// Ensure timer is inactive post completion of all iterations.
    ASSERT_FALSE(m_timerDelegate.isActive());
}

/// Test to verify the stop and start API.
TEST_F(TimerDelegateTest, test_stopAndStartTimeDelegate) {
    auto delay = milliseconds(500);
    auto period = milliseconds(500);
    auto maxCount = 2u;
    auto graceTime = milliseconds(50);

    m_timerDelegate.start(
        delay, period, TimerDelegateInterface::PeriodType::ABSOLUTE, maxCount, [this] { testTask(); });

    /// Confirm that the timer stops immediately since the timer is not active yet (due to the delay).
    m_timerDelegate.stop();
    ASSERT_EQ(getTaskCounter(), 0u);
    ASSERT_FALSE(m_timerDelegate.isActive());

    /// Ensure timer is active once start is called.
    m_timerDelegate.start(
        delay, period, TimerDelegateInterface::PeriodType::ABSOLUTE, maxCount, [this] { testTask(); });
    ASSERT_TRUE(m_timerDelegate.isActive());

    /// Sleep until all iterations complete.
    this_thread::sleep_for(delay + period * maxCount + graceTime);
    ASSERT_EQ(getTaskCounter(), maxCount);
    ASSERT_FALSE(m_timerDelegate.isActive());
}

/// Test to verify that stopping an already stopped timer is a no-op.
TEST_F(TimerDelegateTest, test_doubleStopTestAVS) {
    auto delay = milliseconds(100);
    auto period = milliseconds(100);
    auto maxCount = 2u;
    auto graceTime = milliseconds(50);

    m_timerDelegate.start(
        delay, period, TimerDelegateInterface::PeriodType::ABSOLUTE, maxCount, [this] { testTask(); });

    /// Wait until all iterations complete.
    this_thread::sleep_for(delay + period * maxCount + graceTime);

    /// Stop the timer and confirm that the timer becomes inactive.
    m_timerDelegate.stop();
    ASSERT_EQ(getTaskCounter(), maxCount);
    ASSERT_FALSE(m_timerDelegate.isActive());

    /// Verify that subsequent stop calls changes nothing.
    m_timerDelegate.stop();
    ASSERT_EQ(getTaskCounter(), maxCount);
    ASSERT_FALSE(m_timerDelegate.isActive());
}

/**
 * Test to verify timer operations with a task of fixed duration.
 * Delay : 100ms.
 * Period : 100ms.
 * Task Time : 40ms.
 * Expectation : 4 task iterations should complete after 440 ms.
+----------------+------------------------+-----------------+
| Iteration Time | Intermediate timepoint | Action          |
+================+========================+=================+
| 100            |                        | Start Task#1    |
+----------------+------------------------+-----------------+
|                | 140                    | End of Task#1   |
+----------------+------------------------+-----------------+
| 200            |                        | Start of Task#2 |
+----------------+------------------------+-----------------+
|                | 240                    | End of Task#2   |
+----------------+------------------------+-----------------+
| 300            |                        | Start of Task#3 |
+----------------+------------------------+-----------------+
|                | 340                    | End of Task#3   |
+----------------+------------------------+-----------------+
| 400            |                        | Start of Task#4 |
+----------------+------------------------+-----------------+
|                | 440                    | End of Task#4   |
+----------------+------------------------+-----------------+
*/
TEST_F(TimerDelegateTest, test_verifyTaskWithFixedDuration) {
    auto delay = milliseconds(100);
    auto period = milliseconds(100);
    auto maxCount = 4u;
    auto taskDuration = milliseconds(40);
    auto graceTime = milliseconds(50);

    m_timerDelegate.start(
        delay, period, TimerDelegateInterface::PeriodType::ABSOLUTE, maxCount, [this, taskDuration]() {
            testTaskWithSleep(taskDuration);
        });

    /// Sleep until timer completes.
    this_thread::sleep_for(delay + maxCount * period + graceTime);

    /// Confirm the task counter is as expected.
    ASSERT_EQ(getTaskCounter(), maxCount);
    ASSERT_FALSE(m_timerDelegate.isActive());
}

/**
 * Test to verify timer operations with a task of variable duration.
 * Delay: 100ms.
 * Period: 100ms.
 * Task Times:
 ** Iteration 1: 220ms.
 ** Iteration 2: 120ms.
 ** Iteration 3 (and beyond): 80ms.
 * MaxCount: 9.
 * Expectation of 1000 ms only 6 iterations are run and deactivated.
+----------------+------------------------+------------------------+
| Iteration Time | Intermediate timepoint | Action                 |
+================+========================+========================+
| 100            |                        | Start Task#1           |
+----------------+------------------------+------------------------+
| 200            |                        | Skip                   |
+----------------+------------------------+------------------------+
| 300            |                        | Skip                   |
+----------------+------------------------+------------------------+
|                | 320                    | End of Task#1          |
+----------------+------------------------+------------------------+
| 400            |                        | Start of Task#2        |
+----------------+------------------------+------------------------+
| 500            |                        | Skip                   |
+----------------+------------------------+------------------------+
|                | 520                    | End of Task#2          |
+----------------+------------------------+------------------------+
| 600            |                        | Start of Task#3        |
+----------------+------------------------+------------------------+
|                | 680                    | End of Task#3          |
+----------------+------------------------+------------------------+
| 700            | 780                    | Start and End of Task#4|
+----------------+------------------------+------------------------+
| 800            | 880                    | Start and End of Task#5|
+----------------+------------------------+------------------------+
| 900            | 980                    | Start and End of Task#6|
+----------------+------------------------+------------------------+
*/
TEST_F(TimerDelegateTest, test_verifyTaskWithVariableDuration) {
    auto delay = milliseconds(100);
    auto period = milliseconds(100);
    auto maxCount = 9;
    auto expectedNumTaskCalls = 6u;
    auto gracePeriod = milliseconds(50);
    auto taskDurations = {
        milliseconds(220), milliseconds(120), milliseconds(80), milliseconds(80), milliseconds(80), milliseconds(80)};

    m_timerDelegate.start(
        delay, period, TimerDelegateInterface::PeriodType::ABSOLUTE, maxCount, [this, taskDurations]() {
            TimerDelegateTest::customVariableDurationTask(taskDurations);
        });

    /// Wait for all iterations to complete and verify the task counter.
    this_thread::sleep_for(delay + maxCount * period + gracePeriod);
    ASSERT_EQ(getTaskCounter(), expectedNumTaskCalls);
    ASSERT_FALSE(m_timerDelegate.isActive());
}

/**
 * Test to verify that subsequent start API call must wait for the previous iterations
 * to complete (when using Relative PeriodType).
 */
TEST_F(TimerDelegateTest, test_doubleStartMustWaitForPreviousIterations) {
    auto delay = milliseconds(500);
    auto period = milliseconds(500);
    auto maxCount = 2;
    auto expectedTaskCounter = 4u;
    auto gracePeriod = milliseconds(100);

    m_timerDelegate.start(
        delay, period, TimerDelegateInterface::PeriodType::RELATIVE, maxCount, [this] { testTask(); });

    /// Sleep for more time than the initial delay, verify the task counter.
    this_thread::sleep_for(delay + gracePeriod);
    ASSERT_EQ(getTaskCounter(), 1u);
    ASSERT_TRUE(m_timerDelegate.isActive());

    /// Start again after 1 iteration.
    /// This start will wait for all iterations from previous start call to complete.
    m_timerDelegate.start(
        delay, period, TimerDelegateInterface::PeriodType::RELATIVE, maxCount, [this] { testTask(); });

    /// Sleep until all iterations complete.
    this_thread::sleep_for(delay + period * maxCount + gracePeriod);

    /// Expected task count is 4 : 2 iterations in prev start call + 2 in later start call.
    ASSERT_EQ(getTaskCounter(), expectedTaskCounter);
    ASSERT_FALSE(m_timerDelegate.isActive());
}

/**
 * Test to verify that if the task internally stops the timer, the timer will not run
 * the remaining iterations.
 */
TEST_F(TimerDelegateTest, test_taskWithStop) {
    auto delay = milliseconds(100);
    auto period = milliseconds(500);
    auto maxCount = 2u;
    auto gracePeriod = milliseconds(100);

    m_timerDelegate.start(delay, period, TimerDelegateInterface::PeriodType::ABSOLUTE, maxCount, [this]() {
        taskWithStop(&m_timerDelegate);
    });

    /// Wait for 1 iteration to complete.
    this_thread::sleep_for(delay + gracePeriod);

    /// Since the task stopped the timer after the first iteration,
    /// expected task counter shall be 1.
    ASSERT_EQ(getTaskCounter(), 1u);
    ASSERT_FALSE(m_timerDelegate.isActive());
}

}  // namespace test
}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK