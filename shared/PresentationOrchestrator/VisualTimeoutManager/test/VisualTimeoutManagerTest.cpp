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

#include "acsdk/VisualTimeoutManager/private/VisualTimeoutManager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MockTimerFactory.h"

namespace alexaClientSDK {
namespace visualTimeoutManager {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace presentationOrchestratorInterfaces;
using namespace std::chrono;

static const milliseconds DELAY_1_MS = milliseconds(1);
static const milliseconds DELAY_2_MS = milliseconds(2);
static const milliseconds DELAY_5_MS = milliseconds(5);

class VisualTimeoutManagerTest : public ::testing::Test {
public:
    using MockTimeoutCallback = testing::MockFunction<void()>;

    /// Set up the test harness for running a test.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// Constructor
    VisualTimeoutManagerTest() {
    }

protected:
    /// A pointer to an instance of the @c VisualTimeoutManager that will be instantiated per test.
    std::shared_ptr<VisualTimeoutManager> m_visualTimeoutManager;

    /// A pointer to @c MockTimerFactory used by the test object.
    std::shared_ptr<MockTimerFactory> m_timerFactory;

    /// A pointer to @c Executor used by the tested object.
    std::shared_ptr<threading::Executor> m_executor;
};

void VisualTimeoutManagerTest::SetUp() {
    m_timerFactory = std::make_shared<MockTimerFactory>();

    m_visualTimeoutManager = VisualTimeoutManager::create(m_timerFactory);

    m_executor = std::make_shared<threading::Executor>();
    m_visualTimeoutManager->setExecutor(m_executor);
}

void VisualTimeoutManagerTest::TearDown() {
    m_timerFactory->getTimer()->stop();
    m_visualTimeoutManager->shutdown();
}

/**
 * Tests invocation of callback on RequestTimeout
 */
TEST_F(VisualTimeoutManagerTest, testRequestTimeout) {
    MockTimeoutCallback mockCallback;
    auto requestDelay = DELAY_1_MS;

    EXPECT_CALL(mockCallback, Call()).Times(Exactly(1));

    m_visualTimeoutManager->requestTimeout(requestDelay, mockCallback.AsStdFunction());
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(m_timerFactory->getTimer()->isActive());
    ASSERT_EQ(m_timerFactory->getTimer()->getDelay(), requestDelay);

    // Invoke callback via timer
    m_timerFactory->getTimer()->warpForward(requestDelay);
    m_executor->waitForSubmittedTasks();
}

/**
 * Tests StopTimeout call without RequestTimeout
 */
TEST_F(VisualTimeoutManagerTest, testStopTimeoutWithoutRequest) {
    VisualTimeoutManagerInterface::VisualTimeoutId timeoutId = 0;
    ASSERT_FALSE(m_visualTimeoutManager->stopTimeout(timeoutId));
}

/**
 * Tests behavior on RequestTimeout followed by StopTimeout.
 * Callback specified in request should not be invoked once StopTimeout is called.
 */
TEST_F(VisualTimeoutManagerTest, testRequestTimeoutFollowedByStopTimeout) {
    MockTimeoutCallback mockCallback;
    auto requestDelay = DELAY_2_MS;

    // Timeout callback shouldn't be invoked once stopTimeout is called
    EXPECT_CALL(mockCallback, Call()).Times(Exactly(0));

    auto timeoutId = m_visualTimeoutManager->requestTimeout(requestDelay, mockCallback.AsStdFunction());
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(m_timerFactory->getTimer()->isActive());
    ASSERT_EQ(m_timerFactory->getTimer()->getDelay(), requestDelay);
    ASSERT_TRUE(m_visualTimeoutManager->stopTimeout(timeoutId));

    m_executor->waitForSubmittedTasks();
    ASSERT_FALSE(m_timerFactory->getTimer()->isActive());
}

/**
 * Tests callback doesn't execute if timeout state changed to non-ACTIVE.
 * The timer in this case submits a task to execute callback to the executor.
 * A queued task in executor however changes the state and timeout is no longer active.
 */
TEST_F(VisualTimeoutManagerTest, testCallbackNotExecutedIfTimeoutNotActive) {
    MockTimeoutCallback mockCallback;
    auto requestDelay = DELAY_2_MS;

    EXPECT_CALL(mockCallback, Call()).Times(Exactly(0));

    m_visualTimeoutManager->requestTimeout(requestDelay, mockCallback.AsStdFunction());
    m_executor->waitForSubmittedTasks();
    ASSERT_EQ(m_timerFactory->getTimer()->getDelay(), requestDelay);
    ASSERT_TRUE(m_timerFactory->getTimer()->isActive());

    // submits task to executor to suspend timeout
    m_visualTimeoutManager->onDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState::SPEAKING);
    // submits task to executor to execute callback
    m_timerFactory->getTimer()->warpForward(requestDelay);

    m_executor->waitForSubmittedTasks();
    ASSERT_FALSE(m_timerFactory->getTimer()->isActive());
}

/**
 * Tests behavior on RequestTimeout followed by another RequestTimeout call.
 * Only the latest timeout request should be processed while replacing any previous requests.
 */
TEST_F(VisualTimeoutManagerTest, testRequestTimeoutFollowedByAnotherRequest) {
    MockTimeoutCallback mockCallback1;
    MockTimeoutCallback mockCallback2;
    auto requestDelay1 = DELAY_2_MS;
    auto requestDelay2 = DELAY_1_MS;

    EXPECT_CALL(mockCallback1, Call()).Times(Exactly(0));
    // Timeout callback specified by the latest request alone should be called
    EXPECT_CALL(mockCallback2, Call()).Times(Exactly(1));

    // Invoke two timeout requests
    m_visualTimeoutManager->requestTimeout(requestDelay1, mockCallback1.AsStdFunction());
    m_visualTimeoutManager->requestTimeout(requestDelay2, mockCallback2.AsStdFunction());
    m_executor->waitForSubmittedTasks();
    ASSERT_EQ(m_timerFactory->getTimer()->getDelay(), requestDelay2);
    ASSERT_TRUE(m_timerFactory->getTimer()->isActive());

    // Invoke callback via timer
    m_timerFactory->getTimer()->warpForward(requestDelay2);
    m_executor->waitForSubmittedTasks();
}

/**
 * Tests multiple timeout requests followed by stop timeouts.
 * StopTimeout with timeoutId of latest request should return true.
 * StopTimeout with timeoutId of requests other than latest should return false.
 * Only the latest timeout is considered active.
 */
TEST_F(VisualTimeoutManagerTest, testMultipleRequestTimeoutsFollowedByStopTimeouts) {
    MockTimeoutCallback mockCallback1;
    MockTimeoutCallback mockCallback2;
    auto requestDelay1 = DELAY_2_MS;
    auto requestDelay2 = DELAY_5_MS;

    EXPECT_CALL(mockCallback1, Call()).Times(Exactly(0));
    EXPECT_CALL(mockCallback2, Call()).Times(Exactly(0));

    auto timeoutId1 = m_visualTimeoutManager->requestTimeout(requestDelay1, mockCallback1.AsStdFunction());
    auto timeoutId2 = m_visualTimeoutManager->requestTimeout(requestDelay2, mockCallback2.AsStdFunction());
    m_executor->waitForSubmittedTasks();
    ASSERT_EQ(m_timerFactory->getTimer()->getDelay(), requestDelay2);
    ASSERT_TRUE(m_timerFactory->getTimer()->isActive());

    ASSERT_FALSE(m_visualTimeoutManager->stopTimeout(timeoutId1));
    ASSERT_TRUE(m_visualTimeoutManager->stopTimeout(timeoutId2));

    m_executor->waitForSubmittedTasks();
    ASSERT_FALSE(m_timerFactory->getTimer()->isActive());
}

/**
 * Tests suspending timeout on DialogUXState change to non-IDLE state.
 */
TEST_F(VisualTimeoutManagerTest, testTimeoutSuspendedByDialogUXState) {
    MockTimeoutCallback mockCallback;
    auto requestDelay = DELAY_2_MS;

    EXPECT_CALL(mockCallback, Call()).Times(Exactly(0));

    m_visualTimeoutManager->requestTimeout(requestDelay, mockCallback.AsStdFunction());
    ASSERT_TRUE(m_timerFactory->getTimer()->isActive());

    m_visualTimeoutManager->onDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState::SPEAKING);
    m_executor->waitForSubmittedTasks();
    ASSERT_FALSE(m_timerFactory->getTimer()->isActive());
}

/**
 * Tests whether timeout is restarted on DialogUXState change to IDLE state.
 */
TEST_F(VisualTimeoutManagerTest, testTimeoutSuspendedAndRestartedByDialogUXState) {
    MockTimeoutCallback mockCallback;
    auto requestDelay = DELAY_2_MS;

    EXPECT_CALL(mockCallback, Call()).Times(Exactly(0));
    m_visualTimeoutManager->requestTimeout(requestDelay, mockCallback.AsStdFunction());
    m_visualTimeoutManager->onDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState::SPEAKING);
    m_executor->waitForSubmittedTasks();
    ASSERT_FALSE(m_timerFactory->getTimer()->isActive());

    m_visualTimeoutManager->onDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState::IDLE);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(m_timerFactory->getTimer()->isActive());

    // Timeout callback should be invoked once timer is restarted
    EXPECT_CALL(mockCallback, Call()).Times(Exactly(1));
    // Invoke callback via timer
    m_timerFactory->getTimer()->warpForward(requestDelay);
    m_executor->waitForSubmittedTasks();
}

}  // namespace test
}  // namespace visualTimeoutManager
}  // namespace alexaClientSDK