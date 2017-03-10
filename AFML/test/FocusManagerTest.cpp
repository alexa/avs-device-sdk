/**
 * FocusManagerTest.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <atomic>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AFML/FocusManager.h"
#include "AFML/FocusState.h"

namespace alexaClientSDK {
namespace afml {

/// Time out for the Channel observer to wait for the focus change callback.
static const auto TIME_OUT_IN_SECONDS = std::chrono::seconds(30);

/// The dialog Channel name used in intializing the FocusManager.
static const std::string DIALOG_CHANNEL_NAME = "DialogChannel";

/// The alerts Channel name used in intializing the FocusManager.
static const std::string ALERTS_CHANNEL_NAME = "AlertsChannel";

/// The content Channel name used in intializing the FocusManager.
static const std::string CONTENT_CHANNEL_NAME = "ContentChannel";

/// An incorrect Channel name that is never initialized as a Channel.
static const std::string INCORRECT_CHANNEL_NAME = "aksdjfl;aksdjfl;akdsjf";

/// The priority of the dialog Channel used in intializing the FocusManager.
static const unsigned int DIALOG_CHANNEL_PRIORITY = 10;

/// The priority of the alerts Channel used in intializing the FocusManager.
static const unsigned int ALERTS_CHANNEL_PRIORITY = 20;

/// The priority of the content Channel used in intializing the FocusManager.
static const unsigned int CONTENT_CHANNEL_PRIORITY = 30;

/// Sample dialog activity id.
static const std::string DIALOG_ACTIVITY_ID = "dialog";

/// Sample alerts activity id.
static const std::string ALERTS_ACTIVITY_ID = "alerts";

/// Sample content activity id.
static const std::string CONTENT_ACTIVITY_ID = "content";

/// Another sample dialog activity id.
static const std::string DIFFERENT_DIALOG_ACTIVITY_ID = "different dialog";

/// A test observer that mocks out the ChannelObserverInterface##onFocusChanged() call.
class TestClient : public ChannelObserverInterface {
public:
    MOCK_METHOD1(onFocusChanged, void(FocusState focusState));
};

/// Test fixture for testing FocusManager.
class FocusManagerTest : public ::testing::Test {
protected:
    /// The FocusManager.
    std::shared_ptr<FocusManager> m_focusManager;

    /// A client that acquires the dialog Channel.
    std::shared_ptr<TestClient> dialogClient;

    /// Another client that acquires the dialog Channel.
    std::shared_ptr<TestClient> anotherDialogClient;

    /// A client that acquires the alerts Channel.
    std::shared_ptr<TestClient> alertsClient;

    /// A client that acquires the content Channel.
    std::shared_ptr<TestClient> contentClient;

    /// A condition variable used to wait for all the onFocusChanged() calls.
    std::condition_variable m_cv;

    /// A lock used used to wait for all the onFocusChanged() calls.
    std::mutex m_mutex;

    virtual void SetUp() {
        FocusManager::ChannelConfiguration dialogChannelConfig{DIALOG_CHANNEL_NAME, DIALOG_CHANNEL_PRIORITY};

        FocusManager::ChannelConfiguration alertsChannelConfig{ALERTS_CHANNEL_NAME, ALERTS_CHANNEL_PRIORITY};

        FocusManager::ChannelConfiguration contentChannelConfig{CONTENT_CHANNEL_NAME, CONTENT_CHANNEL_PRIORITY};

        std::vector<FocusManager::ChannelConfiguration> channelConfigurations {
            dialogChannelConfig, alertsChannelConfig, contentChannelConfig
        };

        m_focusManager = std::make_shared<FocusManager>(channelConfigurations);

        dialogClient = std::make_shared<TestClient>();
        alertsClient = std::make_shared<TestClient>();
        contentClient = std::make_shared<TestClient>();
        anotherDialogClient = std::make_shared<TestClient>();
    }
};

/// Tests acquireChannel with an invalid Channel name, expecting no focus changes to be made.
TEST_F(FocusManagerTest, acquireInvalidChannelName) {
    ASSERT_FALSE(m_focusManager->acquireChannel(INCORRECT_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID));
}

/// Tests acquireChannel, expecting to get Foreground status since no other Channels are active.
TEST_F(FocusManagerTest, acquireChannelWithNoOtherChannelsActive) {
    std::atomic<int> numCalls(0);
    const int expectedNumCalls = 1;
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    std::unique_lock<std::mutex> lock(m_mutex);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID);
    m_cv.wait_for(lock, TIME_OUT_IN_SECONDS, [&numCalls, expectedNumCalls]() {
        return numCalls.load() == expectedNumCalls;
    });
}

/**
 * Tests acquireChannel with a two Channel. The lower priority Channel should get Background focus and the higher
 * priority Channel should get Foreground focus.
 */
TEST_F(FocusManagerTest, acquireLowerPriorityChannelWithOneHigherPriorityChannelTaken) {
    std::atomic<int> numCalls(0);
    const int expectedNumCalls = 2;
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*alertsClient, onFocusChanged(FocusState::BACKGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    std::unique_lock<std::mutex> lock(m_mutex);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID);
    m_focusManager->acquireChannel(ALERTS_CHANNEL_NAME, alertsClient, ALERTS_ACTIVITY_ID);
    m_cv.wait_for(lock, TIME_OUT_IN_SECONDS, [&numCalls, expectedNumCalls]() {
        return numCalls.load() == expectedNumCalls;
    });
}

/**
 * Tests acquireChannel with three Channels. The two lowest priority Channels should get Background focus while the 
 * highest priority Channel should be Foreground focused.
 */
TEST_F(FocusManagerTest, acquireLowerPriorityChannelWithTwoHigherPriorityChannelsTaken) {
    std::atomic<int> numCalls(0);
    const int expectedNumCalls = 3;
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*alertsClient, onFocusChanged(FocusState::BACKGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*contentClient, onFocusChanged(FocusState::BACKGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    std::unique_lock<std::mutex> lock(m_mutex);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID);
    m_focusManager->acquireChannel(ALERTS_CHANNEL_NAME, alertsClient, ALERTS_ACTIVITY_ID);
    m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_ACTIVITY_ID);
    m_cv.wait_for(lock, TIME_OUT_IN_SECONDS, [&numCalls, expectedNumCalls]() {
        return numCalls.load() == expectedNumCalls;
    });
}

/**
 * Tests acquireChannel with a high priority Channel while a low priority Channel is already taken. The lower priority
 * Channel should at first be Foreground focused and then get a change to Background focus while the higher priority
 * should be Foreground focused. 
 */
TEST_F(FocusManagerTest, acquireHigherPriorityChannelWithOneLowerPriorityChannelTaken) {
    std::atomic<int> numCalls(0);
    const int expectedNumCalls = 3;
    EXPECT_CALL(*contentClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*contentClient, onFocusChanged(FocusState::BACKGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    std::unique_lock<std::mutex> lock(m_mutex);
    m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_ACTIVITY_ID);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID);
    m_cv.wait_for(lock, TIME_OUT_IN_SECONDS, [&numCalls, expectedNumCalls]() {
        return numCalls.load() == expectedNumCalls;
    });
}

/**
 * Tests acquireChannel with a single Channel. The original observer should be notified to stop and the new observer
 * should obtain Foreground focus.
 */
TEST_F(FocusManagerTest, kickOutActivityOnSameChannel) {
    std::atomic<int> numCalls(0);
    const int expectedNumCalls = 3;
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::NONE))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*anotherDialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    std::unique_lock<std::mutex> lock(m_mutex);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, anotherDialogClient, DIFFERENT_DIALOG_ACTIVITY_ID);
    m_cv.wait_for(lock, TIME_OUT_IN_SECONDS, [&numCalls, expectedNumCalls]() {
        return numCalls.load() == expectedNumCalls;
    });
}

/**
 * Tests releaseChannel with a single Channel. The observer should be notified to stop.
 */
TEST_F(FocusManagerTest, simpleReleaseChannel) {
    std::atomic<int> numCalls(0);
    const int expectedNumCalls = 2;
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::NONE))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    ); 
    std::unique_lock<std::mutex> lock(m_mutex);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID);
    m_focusManager->releaseChannel(DIALOG_CHANNEL_NAME);
    m_cv.wait_for(lock, TIME_OUT_IN_SECONDS, [&numCalls, expectedNumCalls]() {
        return numCalls.load() == expectedNumCalls;
    });
}

/**
 * Tests releaseChannel of the Foreground focused Channel while another Channel is taken. The originally Background
 * focused Channel should be notified to come to the Foreground while the originally Foreground focused Channel should
 * be notified to stop.
 */
TEST_F(FocusManagerTest, releaseForegroundChannelWhileBackgroundChannelTaken) {
    std::atomic<int> numCalls(0);
    const int expectedNumCalls = 4;
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*contentClient, onFocusChanged(FocusState::BACKGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::NONE))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    ); 
    EXPECT_CALL(*contentClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    ); 
    std::unique_lock<std::mutex> lock(m_mutex);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID);
    m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_ACTIVITY_ID);
    m_focusManager->releaseChannel(DIALOG_CHANNEL_NAME);
    m_cv.wait_for(lock, TIME_OUT_IN_SECONDS, [&numCalls, expectedNumCalls]() {
        return numCalls.load() == expectedNumCalls;
    });
 }

/**
 * Tests stopForegroundActivity with a single Channel. The observer should be notified to stop.
 */
TEST_F(FocusManagerTest, simpleNonTargetedStop) {
    std::atomic<int> numCalls(0);
    const int expectedNumCalls = 2;
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
                lock.unlock();
                m_focusManager->stopForegroundActivity();
            }
        )
    ); 
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::NONE))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    ); 
    std::unique_lock<std::mutex> lock(m_mutex);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID);
    m_cv.wait_for(lock, TIME_OUT_IN_SECONDS, [&numCalls, expectedNumCalls]() {
        return numCalls.load() == expectedNumCalls;
    });
}

/**
 * Tests stopForegroundActivity with a three active Channels. The Foreground Channel observer should be notified to 
 * stop each time and the next highest priority background Channel should be brought to the foreground each time.
 */
TEST_F(FocusManagerTest, threeNonTargetedStopsWithThreeActivitiesHappening) {
    std::atomic<int> numCalls(0);
    const int expectedNumCalls = 8;
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*alertsClient, onFocusChanged(FocusState::BACKGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*contentClient, onFocusChanged(FocusState::BACKGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
                lock.unlock();
                m_focusManager->stopForegroundActivity();
            }
        )
    );
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::NONE))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*alertsClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
                lock.unlock();
                m_focusManager->stopForegroundActivity();
            }
        )
    );
    EXPECT_CALL(*alertsClient, onFocusChanged(FocusState::NONE))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*contentClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
                lock.unlock();
                m_focusManager->stopForegroundActivity();
            }
        )
    );
    EXPECT_CALL(*contentClient, onFocusChanged(FocusState::NONE))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    std::unique_lock<std::mutex> lock(m_mutex);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID);
    m_focusManager->acquireChannel(ALERTS_CHANNEL_NAME, alertsClient, ALERTS_ACTIVITY_ID);
    m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_ACTIVITY_ID);
    m_cv.wait_for(lock, TIME_OUT_IN_SECONDS, [&numCalls, expectedNumCalls]() {
        return numCalls.load() == expectedNumCalls;
    });
}

/**
 * Tests stopForegroundActivity with a single Channel. The next client to request a different Channel should be given 
 * foreground focus.
 */
TEST_F(FocusManagerTest, stopForegroundActivityAndAcquireDifferentChannel) {
    std::atomic<int> numCalls(0);
    const int expectedNumCalls = 3;
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
                lock.unlock();
                m_focusManager->stopForegroundActivity();
            }
        )
    ); 
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::NONE))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*contentClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    ); 
    std::unique_lock<std::mutex> lock(m_mutex);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID);
    m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_ACTIVITY_ID);
    m_cv.wait_for(lock, TIME_OUT_IN_SECONDS, [&numCalls, expectedNumCalls]() {
        return numCalls.load() == expectedNumCalls;
    });
}

/**
 * Tests stopForegroundActivity with a single Channel. The next client to request the same Channel should be given 
 * foreground focus.
 */
TEST_F(FocusManagerTest, stopForegroundActivityAndAcquireSameChannel) {
    std::atomic<int> numCalls(0);
    const int expectedNumCalls = 3;
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
                lock.unlock();
                m_focusManager->stopForegroundActivity();
            }
        )
    )
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    ); 
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::NONE))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    std::unique_lock<std::mutex> lock(m_mutex);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID);
    m_cv.wait_for(lock, TIME_OUT_IN_SECONDS, [&numCalls, expectedNumCalls]() {
        return numCalls.load() == expectedNumCalls;
    });
}

/**
 * Tests releaseChannel with the background Channel while there is a foreground Channel. The foreground Channel
 * should remain foregrounded while the background Channel's observer should be notified to stop.
 */
TEST_F(FocusManagerTest, releaseBackgroundChannelWhileTwoChannelsTaken) {
    std::atomic<int> numCalls(0);
    const int expectedNumCalls = 3;
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*contentClient, onFocusChanged(FocusState::BACKGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*contentClient, onFocusChanged(FocusState::NONE))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    std::unique_lock<std::mutex> lock(m_mutex);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID);
    m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_ACTIVITY_ID);
    m_focusManager->releaseChannel(CONTENT_CHANNEL_NAME);
    m_cv.wait_for(lock, TIME_OUT_IN_SECONDS, [&numCalls, expectedNumCalls]() {
        return numCalls.load() == expectedNumCalls;
    }); 
}

/**
 * Tests acquireChannel of an already active foreground Channel while another Channel is also active. The original 
 * observer of the foreground be notified to stop and the new observer of the Channel will be notified that it has
 * Foreground focus. The originally backgrounded Channel should not change focus.
 */
TEST_F(FocusManagerTest, kickOutActivityOnSameChannelWhileOtherChannelsActive) {
    std::atomic<int> numCalls(0);
    const int expectedNumCalls = 4;
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*contentClient, onFocusChanged(FocusState::BACKGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*dialogClient, onFocusChanged(FocusState::NONE))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    );
    EXPECT_CALL(*anotherDialogClient, onFocusChanged(FocusState::FOREGROUND))
        .WillOnce(testing::InvokeWithoutArgs(
            [this, &numCalls] () {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++numCalls;
                m_cv.notify_one();
            }
        )
    ); 
    std::unique_lock<std::mutex> lock(m_mutex);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_ACTIVITY_ID);
    m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_ACTIVITY_ID);
    m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, anotherDialogClient, DIFFERENT_DIALOG_ACTIVITY_ID);
    m_cv.wait_for(lock, TIME_OUT_IN_SECONDS, [&numCalls, expectedNumCalls]() {
        return numCalls.load() == expectedNumCalls;
    }); 
}

/// Test fixture for testing Channel.
class ChannelTest : public ::testing::Test {
protected:
    /// A test client that used to observe Channels.
    std::shared_ptr<TestClient> clientA;

    /// A test client that used to observe Channels.
    std::shared_ptr<TestClient> clientB;

    /// A test Channel.
    std::shared_ptr<Channel> testChannel;

    virtual void SetUp() {

        clientA = std::make_shared<TestClient>();
        clientB = std::make_shared<TestClient>();
        testChannel = std::make_shared<Channel>(DIALOG_CHANNEL_PRIORITY);
    }
};

/// Tests the that the getPriority method of Channel works properly.
TEST_F(ChannelTest, getPriority) {
    EXPECT_EQ(testChannel->getPriority(), DIALOG_CHANNEL_PRIORITY);
}

/// Tests that an old observer is kicked out on a Channel when a new observer is set.
TEST_F(ChannelTest, kickoutOldObserver) {
    EXPECT_CALL(*clientA, onFocusChanged(FocusState::FOREGROUND));
    EXPECT_CALL(*clientA, onFocusChanged(FocusState::NONE));
    testChannel->setObserver(clientA);
    testChannel->setFocus(FocusState::FOREGROUND);
    testChannel->setObserver(clientB);
}

/// Tests that the observer properly gets notified of focus changes.
TEST_F(ChannelTest, setObserverThenSetFocus) {
    EXPECT_CALL(*clientA, onFocusChanged(FocusState::FOREGROUND));
    EXPECT_CALL(*clientA, onFocusChanged(FocusState::BACKGROUND));
    EXPECT_CALL(*clientA, onFocusChanged(FocusState::NONE));
    testChannel->setObserver(clientA);
    testChannel->setFocus(FocusState::FOREGROUND);
    testChannel->setFocus(FocusState::BACKGROUND);
    testChannel->setFocus(FocusState::NONE);
}

/// Tests that Channels are compared properly
TEST_F(ChannelTest, priorityComparison) {
    std::shared_ptr<Channel> lowerPriorityChannel = std::make_shared<Channel>(CONTENT_CHANNEL_PRIORITY);
    EXPECT_TRUE(*testChannel > *lowerPriorityChannel);
    EXPECT_FALSE(*lowerPriorityChannel > *testChannel);
}

/**
 *Tests that the stopActivity method on Channel works properly and that observers are stopped if the activity id 
 * matches the the Channel's activity and doesn't get stopped if the ids don't match.
 */
TEST_F(ChannelTest, testStopActivityWithSameId) {
    EXPECT_CALL(*clientA, onFocusChanged(FocusState::FOREGROUND));
    EXPECT_CALL(*clientA, onFocusChanged(FocusState::NONE));
    testChannel->setActivityId(DIALOG_ACTIVITY_ID);
    testChannel->setObserver(clientA);
    testChannel->setFocus(FocusState::FOREGROUND);
    testChannel->stopActivity(DIALOG_ACTIVITY_ID);
}

TEST_F(ChannelTest, testStopActivityWithDifferentId) {
    EXPECT_CALL(*clientA, onFocusChanged(FocusState::FOREGROUND));
    EXPECT_CALL(*clientA, onFocusChanged(FocusState::NONE)).Times(0);
    testChannel->setActivityId(DIALOG_ACTIVITY_ID);
    testChannel->setObserver(clientA);
    testChannel->setFocus(FocusState::FOREGROUND);
    testChannel->stopActivity(CONTENT_ACTIVITY_ID);
}

} // namespace afml
} // namespace alexaClientSDK
