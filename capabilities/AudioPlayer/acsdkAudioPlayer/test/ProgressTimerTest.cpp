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

#include <gmock/gmock.h>

#include <AVSCommon/Utils/PromiseFuturePair.h>
#include <AVSCommon/Utils/Timing/Stopwatch.h>

#include "acsdkAudioPlayer/ProgressTimer.h"

namespace alexaClientSDK {
namespace acsdkAudioPlayer {
namespace test {

using namespace avsCommon::utils;
using namespace avsCommon::utils::timing;
using namespace testing;

/// 10 millisecond delay or interval value.
static const std::chrono::milliseconds MILLIS_10{10};

/// 25 millisecond delay or interval value (something not an interval of @c MILLIS_10).
static const std::chrono::milliseconds MILLIS_25{25};

/// 100 millisecond duration to allow ProgressTimer to misbehave.
static const std::chrono::milliseconds MILLIS_100{100};

/// 5 second timer to allow plenty of time for expected behaviors to be detected.
static const std::chrono::seconds FAIL_TIMEOUT{5};

/// Delay value to use for test to verify offsets.
static const std::chrono::milliseconds OFFSET_TEST_DELAY{300};

/// Interval value to use for test to verify offsets.
static const std::chrono::milliseconds OFFSET_TEST_INTERVAL{500};

/// Amount an offset can be less than expected and still be acceptable.
static const std::chrono::milliseconds LOWER_ERROR{100};

/// Amount an offset can be greater than expected and still be acceptable.
static const std::chrono::milliseconds UPPER_ERROR{200};

/**
 * Helper function used to validate received offset values.
 *
 * @param expected The expected offset.
 * @param received The received offset.
 */
static void verifyOffset(std::chrono::milliseconds expected, std::chrono::milliseconds received) {
    ASSERT_GE(received, expected - LOWER_ERROR);
    ASSERT_LE(received, expected + UPPER_ERROR);
};

/**
 * Mock of ProgressTimer::ContextInterface with which to verify ProgressTimer behavior.
 */
class MockContext : public ProgressTimer::ContextInterface {
public:
    MOCK_METHOD0(requestProgress, void());
    MOCK_METHOD0(onProgressReportDelayElapsed, void());
    MOCK_METHOD0(onProgressReportIntervalElapsed, void());
    MOCK_METHOD0(onProgressReportIntervalUpdated, void());
};

/**
 * Test fixture for exercising ProgressTimer.
 *
 * The player methods (i.e. play(), pause(), resume(), stop()) control reporting mock progress in the
 * audio stream, and call ProgressTimer methods (rather than directly in the tests) to simplify the
 * tests and assure proper ordering of ersatz player state and ProgressTimer calls.
 */
class ProgressTimerTest : public ::testing::Test {
public:
    /// @{
    /// @name Test methods
    void SetUp() override;
    /// @}

    /**
     * Start the the audio offset generator and then the ProgressTimer.
     */
    void play();

    /**
     * Pause the ProgressTimer and then the audio offset generator.
     */
    void pause();

    /**
     * Resume the audio offset generator and then the ProgressTimer.
     */
    void resume();

    /**
     * Stop the ProgressTimer and then the offset generator.
     */
    void stop();

    /**
     * Get the current offset in to the mock audio stream and call m_timer.onProgress() with it.
     */
    void callOnProgress();

    /// The MockContext with which to monitor @c m_timer's behavior.
    std::shared_ptr<MockContext> m_mockContext;

    /// The ProgressTimer to test.
    ProgressTimer m_timer;

    /// A stopwatch with which to generate offsets.
    Stopwatch m_stopwatch;
};

void ProgressTimerTest::SetUp() {
    m_mockContext = std::make_shared<NiceMock<MockContext>>();
}

void ProgressTimerTest::play() {
    ASSERT_TRUE(m_stopwatch.start());
    m_timer.start();
}

void ProgressTimerTest::pause() {
    ASSERT_TRUE(m_stopwatch.pause());
    m_timer.pause();
}

void ProgressTimerTest::resume() {
    ASSERT_TRUE(m_stopwatch.resume());
    m_timer.resume();
}

void ProgressTimerTest::stop() {
    m_stopwatch.stop();
    m_timer.stop();
}

void ProgressTimerTest::callOnProgress() {
    auto progress = m_stopwatch.getElapsed();
    m_timer.onProgress(progress);
}

// Verify that with invalid delay and interval, no progress is reported.
TEST_F(ProgressTimerTest, test_noDelayOrInterval) {
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportDelayElapsed()).Times(0);
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed()).Times(0);

    m_timer.init(m_mockContext, ProgressTimer::getNoDelay(), ProgressTimer::getNoInterval());

    play();
    std::this_thread::sleep_for(MILLIS_100);
    stop();
}

// Verify that an interval of zero does not trigger progress reports or a crash.
TEST_F(ProgressTimerTest, test_zeroInterval) {
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportDelayElapsed()).Times(0);
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed()).Times(0);

    m_timer.init(m_mockContext, ProgressTimer::getNoDelay(), std::chrono::milliseconds{0});

    play();
    std::this_thread::sleep_for(MILLIS_100);
    stop();
}

// Verify that with a valid delay and invalid interval, a delay notification is generated.
TEST_F(ProgressTimerTest, test_justDelay) {
    auto requestProgress = [this] { callOnProgress(); };

    EXPECT_CALL(*(m_mockContext.get()), requestProgress()).WillRepeatedly(Invoke(requestProgress));
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportDelayElapsed()).Times(1);
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed()).Times(0);

    m_timer.init(m_mockContext, MILLIS_10, ProgressTimer::getNoInterval());

    play();
    std::this_thread::sleep_for(MILLIS_100);
    stop();
}

// Verify that with a invalid delay and a valid interval, interval notifications are generated.
TEST_F(ProgressTimerTest, test_justInterval) {
    auto requestProgress = [this] { callOnProgress(); };

    int reportCounter = 0;
    PromiseFuturePair<void> gotTenReports;
    auto notifyOnTenReports = [&reportCounter, &gotTenReports]() {
        if (10 == ++reportCounter) {
            gotTenReports.setValue();
        }
    };

    EXPECT_CALL(*(m_mockContext.get()), requestProgress()).WillRepeatedly(Invoke(requestProgress));
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportDelayElapsed()).Times(0);
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed()).WillRepeatedly(Invoke(notifyOnTenReports));

    m_timer.init(m_mockContext, ProgressTimer::getNoDelay(), MILLIS_10);

    play();
    ASSERT_TRUE(gotTenReports.waitFor(FAIL_TIMEOUT));
    stop();
}

// Verify that with both a valid delay and interval, both types of notifications are generated.
TEST_F(ProgressTimerTest, test_delayAndInterval) {
    auto requestProgress = [this] { callOnProgress(); };

    int reportCounter = 0;
    PromiseFuturePair<void> gotTenReports;
    auto notifyOnTenReports = [&reportCounter, &gotTenReports]() {
        if (10 == ++reportCounter) {
            gotTenReports.setValue();
        }
    };

    EXPECT_CALL(*(m_mockContext.get()), requestProgress()).WillRepeatedly(Invoke(requestProgress));

    {
        InSequence sequence;

        EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed())
            .Times(2)
            .WillRepeatedly(Invoke(notifyOnTenReports))
            .RetiresOnSaturation();
        EXPECT_CALL(*(m_mockContext.get()), onProgressReportDelayElapsed()).Times(1);
        EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed())
            .WillRepeatedly(Invoke(notifyOnTenReports));
    }

    m_timer.init(m_mockContext, MILLIS_25, MILLIS_10);

    play();
    ASSERT_TRUE(gotTenReports.waitFor(FAIL_TIMEOUT));

    stop();

    // Also verify that notifications are not generated after stop.
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportDelayElapsed()).Times(0);
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed()).Times(0);
    std::this_thread::sleep_for(MILLIS_100);
}

// Verify that when paused, a ProgressTimer will not generate notifications.
TEST_F(ProgressTimerTest, test_pause) {
    auto requestProgress = [this] { callOnProgress(); };

    std::mutex counterMutex;
    std::condition_variable wake;
    int reportCounter = 0;

    auto notifyOnTenReports = [&counterMutex, &wake, &reportCounter]() {
        std::lock_guard<std::mutex> lock(counterMutex);
        if (10 == ++reportCounter) {
            wake.notify_all();
        }
    };

    auto wakeOnTenReports = [&counterMutex, &wake, &reportCounter] {
        std::unique_lock<std::mutex> lock(counterMutex);
        ASSERT_TRUE(wake.wait_for(lock, FAIL_TIMEOUT, [&reportCounter] { return reportCounter >= 10; }));
    };

    EXPECT_CALL(*(m_mockContext.get()), requestProgress()).WillRepeatedly(Invoke(requestProgress));
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportDelayElapsed()).Times(1).RetiresOnSaturation();
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed()).WillRepeatedly(Invoke(notifyOnTenReports));

    m_timer.init(m_mockContext, MILLIS_10, MILLIS_10);

    play();
    wakeOnTenReports();

    // Loop to verify that ProgressTimer can handle multiple pauses.
    for (int ix = 0; ix < 2; ix++) {
        pause();
        EXPECT_CALL(*(m_mockContext.get()), onProgressReportDelayElapsed()).Times(0);
        EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed()).Times(0);

        std::this_thread::sleep_for(MILLIS_100);

        reportCounter = 0;
        EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed())
            .WillRepeatedly(Invoke(notifyOnTenReports));
        resume();
        wakeOnTenReports();
    }

    stop();
}

// Verify that when resumed, a ProgressTimer will not repeat delay progress reports.
TEST_F(ProgressTimerTest, test_resumeDoesNotRepeat) {
    auto requestProgress = [this] { callOnProgress(); };

    EXPECT_CALL(*(m_mockContext.get()), requestProgress()).WillRepeatedly(Invoke(requestProgress));
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportDelayElapsed()).Times(1);
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed()).Times(0);

    m_timer.init(m_mockContext, MILLIS_10, ProgressTimer::getNoInterval());

    play();
    std::this_thread::sleep_for(MILLIS_100);
    pause();
    std::this_thread::sleep_for(MILLIS_100);
    resume();
    std::this_thread::sleep_for(MILLIS_100);
    stop();
}

// Verify that the generated offsets are approximately correct.
TEST_F(ProgressTimerTest, testTimer_offsets) {
    auto requestProgress = [this] { callOnProgress(); };

    auto verifyDelayOffset = [this]() { verifyOffset(OFFSET_TEST_DELAY, m_stopwatch.getElapsed()); };

    int reportCounter = 0;
    PromiseFuturePair<void> gotTenReports;
    auto notifyOnTenReports = [this, &reportCounter, &gotTenReports]() {
        ++reportCounter;
        verifyOffset(reportCounter * OFFSET_TEST_INTERVAL, m_stopwatch.getElapsed());
        if (3 == reportCounter) {
            gotTenReports.setValue();
        }
    };

    EXPECT_CALL(*(m_mockContext.get()), requestProgress()).WillRepeatedly(Invoke(requestProgress));
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportDelayElapsed()).Times(1).WillOnce(Invoke(verifyDelayOffset));
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed()).WillRepeatedly(Invoke(notifyOnTenReports));

    m_timer.init(m_mockContext, OFFSET_TEST_DELAY, OFFSET_TEST_INTERVAL);

    play();
    ASSERT_TRUE(gotTenReports.waitFor(FAIL_TIMEOUT));

    stop();
}

// Verify that when delay and interval coincide, both types of notifications are generated.
TEST_F(ProgressTimerTest, test_delayAndIntervalCoincide) {
    auto requestProgress = [this] { callOnProgress(); };

    PromiseFuturePair<void> gotReport;
    auto notifyGotReport = [&gotReport]() { gotReport.setValue(); };

    EXPECT_CALL(*(m_mockContext.get()), requestProgress()).WillRepeatedly(Invoke(requestProgress));
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportDelayElapsed()).Times(1);
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed()).Times(1).WillOnce(Invoke(notifyGotReport));

    m_timer.init(m_mockContext, MILLIS_100, MILLIS_100);

    play();
    ASSERT_TRUE(gotReport.waitFor(FAIL_TIMEOUT));

    stop();
}

TEST_F(ProgressTimerTest, test_updateInterval) {
    auto requestProgress = [this] { callOnProgress(); };

    PromiseFuturePair<void> oldIntervalReport;
    auto notifyOldReport = [this, &oldIntervalReport]() {
        verifyOffset(OFFSET_TEST_INTERVAL, m_stopwatch.getElapsed());
        oldIntervalReport.setValue();
    };

    EXPECT_CALL(*(m_mockContext.get()), requestProgress()).WillRepeatedly(Invoke(requestProgress));
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed()).Times(1).WillOnce(Invoke(notifyOldReport));

    m_timer.init(m_mockContext, ProgressTimer::getNoDelay(), OFFSET_TEST_INTERVAL);

    play();
    ASSERT_TRUE(oldIntervalReport.waitFor(FAIL_TIMEOUT));

    PromiseFuturePair<void> newIntervalReport;
    auto notifyNewReport = [this, &newIntervalReport]() {
        verifyOffset(OFFSET_TEST_INTERVAL + MILLIS_100, m_stopwatch.getElapsed());
        newIntervalReport.setValue();
    };

    EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalElapsed()).WillRepeatedly(Invoke(notifyNewReport));
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalUpdated()).Times(1);

    m_timer.updateInterval(MILLIS_100);
    ASSERT_TRUE(newIntervalReport.waitFor(FAIL_TIMEOUT));

    stop();
}

// Verify that when the UpdateInterval changes after stop() is called, that no update report is made
TEST_F(ProgressTimerTest, test_updateIntervalAfterStop) {
    m_timer.init(m_mockContext, ProgressTimer::getNoDelay(), OFFSET_TEST_INTERVAL);

    play();
    stop();
    EXPECT_CALL(*(m_mockContext.get()), onProgressReportIntervalUpdated()).Times(0);
    m_timer.updateInterval(MILLIS_100);
}

}  // namespace test
}  // namespace acsdkAudioPlayer
}  // namespace alexaClientSDK
