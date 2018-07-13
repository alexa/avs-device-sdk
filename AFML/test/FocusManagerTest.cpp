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

#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/SDKInterfaces/MockFocusManagerObserver.h>

#include "AFML/FocusManager.h"

namespace alexaClientSDK {
namespace afml {
namespace test {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::avs;

/// Short time out for when callbacks are expected not to occur.
static const auto SHORT_TIMEOUT = std::chrono::milliseconds(50);

/// Long time out for the Channel observer to wait for the focus change callback (we should not reach this).
static const auto DEFAULT_TIMEOUT = std::chrono::seconds(15);

/// The dialog Channel name used in initializing the FocusManager.
static const std::string DIALOG_CHANNEL_NAME = "DialogChannel";

/// The alerts Channel name used in initializing the FocusManager.
static const std::string ALERTS_CHANNEL_NAME = "AlertsChannel";

/// The content Channel name used in initializing the FocusManager.
static const std::string CONTENT_CHANNEL_NAME = "ContentChannel";

/// An incorrect Channel name that is never initialized as a Channel.
static const std::string INCORRECT_CHANNEL_NAME = "aksdjfl;aksdjfl;akdsjf";

/// The priority of the dialog Channel used in initializing the FocusManager.
static const unsigned int DIALOG_CHANNEL_PRIORITY = 10;

/// The priority of the alerts Channel used in initializing the FocusManager.
static const unsigned int ALERTS_CHANNEL_PRIORITY = 20;

/// The priority of the content Channel used in initializing the FocusManager.
static const unsigned int CONTENT_CHANNEL_PRIORITY = 30;

/// Sample dialog interface name.
static const std::string DIALOG_INTERFACE_NAME = "dialog";

/// Sample alerts interface name.
static const std::string ALERTS_INTERFACE_NAME = "alerts";

/// Sample content interface name.
static const std::string CONTENT_INTERFACE_NAME = "content";

/// Another sample dialog interface name.
static const std::string DIFFERENT_DIALOG_INTERFACE_NAME = "different dialog";

/// A test observer that mocks out the ChannelObserverInterface##onFocusChanged() call.
class TestClient : public ChannelObserverInterface {
public:
    /**
     * Constructor.
     */
    TestClient() : m_focusState(FocusState::NONE), m_focusChangeOccurred(false) {
    }

    /**
     * Implementation of the ChannelObserverInterface##onFocusChanged() callback.
     *
     * @param focusState The new focus state of the Channel observer.
     */
    void onFocusChanged(FocusState focusState) override {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_focusState = focusState;
        m_focusChangeOccurred = true;
        m_focusChanged.notify_one();
    }

    /**
     * Waits for the ChannelObserverInterface##onFocusChanged() callback.
     *
     * @param timeout The amount of time to wait for the callback.
     * @param focusChanged An output parameter that notifies the caller whether a callback occurred.
     * @return Returns @c true if the callback occured within the timeout period and @c false otherwise.
     */
    FocusState waitForFocusChange(std::chrono::milliseconds timeout, bool* focusChanged) {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool success = m_focusChanged.wait_for(lock, timeout, [this]() { return m_focusChangeOccurred; });

        if (!success) {
            *focusChanged = false;
        } else {
            m_focusChangeOccurred = false;
            *focusChanged = true;
        }
        return m_focusState;
    }

private:
    /// The focus state of the observer.
    FocusState m_focusState;

    /// A lock to guard against focus state changes.
    std::mutex m_mutex;

    /// A condition variable to wait for focus changes.
    std::condition_variable m_focusChanged;

    /// A boolean flag so that we can re-use the observer even after a callback has occurred.
    bool m_focusChangeOccurred;
};

/// A test observer that mocks out the ActivityTrackerInterface##notifyOfActivityUpdates() call.
class MockActivityTrackerInterface : public ActivityTrackerInterface {
public:
    /**
     * Constructor.
     */
    MockActivityTrackerInterface() : m_activityUpdatesOccurred{false} {
    }

    /// Structure of expected Channel::State result from tests.
    struct ExpectedChannelStateResult {
        /// The expected channel name.
        const std::string name;

        /// The expected interface name.
        const std::string interfaceName;

        /// The expected focus state.
        const FocusState focusState;
    };

    /**
     * Implementation of the ActivityTrackerInterface##notifyOfActivityUpdates() callback.
     *
     * @param channelStates A vector of @c Channel::State that has been updated.
     */
    void notifyOfActivityUpdates(const std::vector<Channel::State>& channelStates) override {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_updatedChannels.clear();
        for (auto& channel : channelStates) {
            m_updatedChannels.push_back(channel);
        }
        m_activityUpdatesOccurred = true;
        m_activityChanged.notify_one();
    }

    /**
     * Waits for the ActivityTrackerInterface##notifyOfActivityUpdates() callback.
     *
     * @param timeout The amount of time to wait for the callback.
     * @param expected The expected channel state results
     */
    void waitForActivityUpdates(
        std::chrono::milliseconds timeout,
        const std::vector<ExpectedChannelStateResult>& expected) {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool success = m_activityChanged.wait_for(lock, timeout, [this, &expected]() {
            if (m_activityUpdatesOccurred) {
                EXPECT_EQ(m_updatedChannels.size(), expected.size());
                auto count = 0;
                for (auto& channel : m_updatedChannels) {
                    EXPECT_EQ(channel.name, expected[count].name);
                    EXPECT_EQ(channel.interfaceName, expected[count].interfaceName);
                    EXPECT_EQ(channel.focusState, expected[count].focusState);
                    count++;
                }
            }
            return m_activityUpdatesOccurred;
        });

        if (success) {
            m_activityUpdatesOccurred = false;
        }
        ASSERT_TRUE(success);
    }

private:
    /// The focus state of the observer.
    std::vector<Channel::State> m_updatedChannels;

    /// A lock to guard against activity changes.
    std::mutex m_mutex;

    /// A condition variable to wait for activity changes.
    std::condition_variable m_activityChanged;

    /// A boolean flag so that we can re-use the observer even after a callback has occurred.
    bool m_activityUpdatesOccurred;
};

/// Manages testing focus changes
class FocusChangeManager {
public:
    /**
     * Checks that a focus change occurred and that the focus state received is the same as the expected focus state.
     *
     * @param client The Channel observer.
     * @param expectedFocusState The expected focus state.
     */
    void assertFocusChange(std::shared_ptr<TestClient> client, FocusState expectedFocusState) {
        bool focusChanged = false;
        FocusState focusStateReceived = client->waitForFocusChange(DEFAULT_TIMEOUT, &focusChanged);
        ASSERT_TRUE(focusChanged);
        ASSERT_EQ(expectedFocusState, focusStateReceived);
    }

    /**
     * Checks that a focus change does not occur by waiting for the timeout duration.
     *
     * @param client The Channel observer.
     */
    void assertNoFocusChange(std::shared_ptr<TestClient> client) {
        bool focusChanged = false;
        // Will wait for the short timeout duration before succeeding
        client->waitForFocusChange(SHORT_TIMEOUT, &focusChanged);
        ASSERT_FALSE(focusChanged);
    }
};

/// Test fixture for testing FocusManager.
class FocusManagerTest
        : public ::testing::Test
        , public FocusChangeManager {
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

    std::shared_ptr<MockActivityTrackerInterface> m_activityTracker;

    virtual void SetUp() {
        m_activityTracker = std::make_shared<MockActivityTrackerInterface>();

        FocusManager::ChannelConfiguration dialogChannelConfig{DIALOG_CHANNEL_NAME, DIALOG_CHANNEL_PRIORITY};

        FocusManager::ChannelConfiguration alertsChannelConfig{ALERTS_CHANNEL_NAME, ALERTS_CHANNEL_PRIORITY};

        FocusManager::ChannelConfiguration contentChannelConfig{CONTENT_CHANNEL_NAME, CONTENT_CHANNEL_PRIORITY};

        std::vector<FocusManager::ChannelConfiguration> channelConfigurations{
            dialogChannelConfig, alertsChannelConfig, contentChannelConfig};

        m_focusManager = std::make_shared<FocusManager>(channelConfigurations, m_activityTracker);

        dialogClient = std::make_shared<TestClient>();
        alertsClient = std::make_shared<TestClient>();
        contentClient = std::make_shared<TestClient>();
        anotherDialogClient = std::make_shared<TestClient>();
    }
};

/// Tests acquireChannel with an invalid Channel name, expecting no focus changes to be made.
TEST_F(FocusManagerTest, acquireInvalidChannelName) {
    ASSERT_FALSE(m_focusManager->acquireChannel(INCORRECT_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
}

/// Tests acquireChannel, expecting to get Foreground status since no other Channels are active.
TEST_F(FocusManagerTest, acquireChannelWithNoOtherChannelsActive) {
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::FOREGROUND);
}

/**
 * Tests acquireChannel with two Channels. The lower priority Channel should get Background focus and the higher
 * priority Channel should get Foreground focus.
 */
TEST_F(FocusManagerTest, acquireLowerPriorityChannelWithOneHigherPriorityChannelTaken) {
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    ASSERT_TRUE(m_focusManager->acquireChannel(ALERTS_CHANNEL_NAME, alertsClient, ALERTS_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::FOREGROUND);
    assertFocusChange(alertsClient, FocusState::BACKGROUND);
}

/**
 * Tests acquireChannel with three Channels. The two lowest priority Channels should get Background focus while the
 * highest priority Channel should be Foreground focused.
 */
TEST_F(FocusManagerTest, aquireLowerPriorityChannelWithTwoHigherPriorityChannelsTaken) {
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    ASSERT_TRUE(m_focusManager->acquireChannel(ALERTS_CHANNEL_NAME, alertsClient, ALERTS_INTERFACE_NAME));
    ASSERT_TRUE(m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::FOREGROUND);
    assertFocusChange(alertsClient, FocusState::BACKGROUND);
    assertFocusChange(contentClient, FocusState::BACKGROUND);
}

/**
 * Tests acquireChannel with a high priority Channel while a low priority Channel is already taken. The lower priority
 * Channel should at first be Foreground focused and then get a change to Background focus while the higher priority
 * should be Foreground focused.
 */
TEST_F(FocusManagerTest, acquireHigherPriorityChannelWithOneLowerPriorityChannelTaken) {
    ASSERT_TRUE(m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_INTERFACE_NAME));
    assertFocusChange(contentClient, FocusState::FOREGROUND);

    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    assertFocusChange(contentClient, FocusState::BACKGROUND);
    assertFocusChange(dialogClient, FocusState::FOREGROUND);
}

/**
 * Tests acquireChannel with a single Channel. The original observer should be notified to stop and the new observer
 * should obtain Foreground focus.
 */
TEST_F(FocusManagerTest, kickOutActivityOnSameChannel) {
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::FOREGROUND);

    ASSERT_TRUE(
        m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, anotherDialogClient, DIFFERENT_DIALOG_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::NONE);
    assertFocusChange(anotherDialogClient, FocusState::FOREGROUND);
}

/**
 * Tests releaseChannel with a single Channel. The observer should be notified to stop.
 */
TEST_F(FocusManagerTest, simpleReleaseChannel) {
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::FOREGROUND);

    ASSERT_TRUE(m_focusManager->releaseChannel(DIALOG_CHANNEL_NAME, dialogClient).get());
    assertFocusChange(dialogClient, FocusState::NONE);
}

/**
 * Tests releaseChannel on a Channel with an incorrect observer. The client should not receive any callback.
 */
TEST_F(FocusManagerTest, simpleReleaseChannelWithIncorrectObserver) {
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::FOREGROUND);

    ASSERT_FALSE(m_focusManager->releaseChannel(CONTENT_CHANNEL_NAME, dialogClient).get());
    ASSERT_FALSE(m_focusManager->releaseChannel(DIALOG_CHANNEL_NAME, contentClient).get());

    assertNoFocusChange(dialogClient);
    assertNoFocusChange(contentClient);
}

/**
 * Tests releaseChannel of the Foreground focused Channel while another Channel is taken. The originally Background
 * focused Channel should be notified to come to the Foreground while the originally Foreground focused Channel should
 * be notified to stop.
 */
TEST_F(FocusManagerTest, releaseForegroundChannelWhileBackgroundChannelTaken) {
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::FOREGROUND);

    ASSERT_TRUE(m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_INTERFACE_NAME));
    assertFocusChange(contentClient, FocusState::BACKGROUND);

    ASSERT_TRUE(m_focusManager->releaseChannel(DIALOG_CHANNEL_NAME, dialogClient).get());
    assertFocusChange(dialogClient, FocusState::NONE);
    assertFocusChange(contentClient, FocusState::FOREGROUND);
}

/**
 * Tests stopForegroundActivity with a single Channel. The observer should be notified to stop.
 */
TEST_F(FocusManagerTest, simpleNonTargetedStop) {
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::FOREGROUND);

    m_focusManager->stopForegroundActivity();
    assertFocusChange(dialogClient, FocusState::NONE);
}

/**
 * Tests stopForegroundActivity with a three active Channels. The Foreground Channel observer should be notified to
 * stop each time and the next highest priority background Channel should be brought to the foreground each time.
 */
TEST_F(FocusManagerTest, threeNonTargetedStopsWithThreeActivitiesHappening) {
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::FOREGROUND);

    ASSERT_TRUE(m_focusManager->acquireChannel(ALERTS_CHANNEL_NAME, alertsClient, ALERTS_INTERFACE_NAME));
    assertFocusChange(alertsClient, FocusState::BACKGROUND);

    ASSERT_TRUE(m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_INTERFACE_NAME));
    assertFocusChange(contentClient, FocusState::BACKGROUND);

    m_focusManager->stopForegroundActivity();
    assertFocusChange(dialogClient, FocusState::NONE);
    assertFocusChange(alertsClient, FocusState::FOREGROUND);

    m_focusManager->stopForegroundActivity();
    assertFocusChange(alertsClient, FocusState::NONE);
    assertFocusChange(contentClient, FocusState::FOREGROUND);

    m_focusManager->stopForegroundActivity();
    assertFocusChange(contentClient, FocusState::NONE);
}

/**
 * Tests stopForegroundActivity with a single Channel. The next client to request a different Channel should be given
 * foreground focus.
 */
TEST_F(FocusManagerTest, stopForegroundActivityAndAcquireDifferentChannel) {
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::FOREGROUND);

    m_focusManager->stopForegroundActivity();
    assertFocusChange(dialogClient, FocusState::NONE);

    ASSERT_TRUE(m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_INTERFACE_NAME));
    assertFocusChange(contentClient, FocusState::FOREGROUND);
}

/**
 * Tests stopForegroundActivity with a single Channel. The next client to request the same Channel should be given
 * foreground focus.
 */
TEST_F(FocusManagerTest, stopForegroundActivityAndAcquireSameChannel) {
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::FOREGROUND);

    m_focusManager->stopForegroundActivity();
    assertFocusChange(dialogClient, FocusState::NONE);

    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::FOREGROUND);
}

/**
 * Tests releaseChannel with the background Channel while there is a foreground Channel. The foreground Channel
 * should remain foregrounded while the background Channel's observer should be notified to stop.
 */
TEST_F(FocusManagerTest, releaseBackgroundChannelWhileTwoChannelsTaken) {
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::FOREGROUND);

    ASSERT_TRUE(m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_INTERFACE_NAME));
    assertFocusChange(contentClient, FocusState::BACKGROUND);

    ASSERT_TRUE(m_focusManager->releaseChannel(CONTENT_CHANNEL_NAME, contentClient).get());
    assertFocusChange(contentClient, FocusState::NONE);

    assertNoFocusChange(dialogClient);
}

/**
 * Tests acquireChannel of an already active foreground Channel while another Channel is also active. The original
 * observer of the foreground be notified to stop and the new observer of the Channel will be notified that it has
 * Foreground focus. The originally backgrounded Channel should not change focus.
 */
TEST_F(FocusManagerTest, kickOutActivityOnSameChannelWhileOtherChannelsActive) {
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::FOREGROUND);

    ASSERT_TRUE(m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_INTERFACE_NAME));
    assertFocusChange(contentClient, FocusState::BACKGROUND);

    ASSERT_TRUE(
        m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, anotherDialogClient, DIFFERENT_DIALOG_INTERFACE_NAME));
    assertFocusChange(dialogClient, FocusState::NONE);
    assertFocusChange(anotherDialogClient, FocusState::FOREGROUND);

    assertNoFocusChange(contentClient);
}

/// Tests that multiple observers can be added, and that they are notified of all focus changes.
TEST_F(FocusManagerTest, addObserver) {
    // These are all the observers that will be added.
    std::vector<std::shared_ptr<MockFocusManagerObserver>> observers;
    observers.push_back(std::make_shared<MockFocusManagerObserver>());
    observers.push_back(std::make_shared<MockFocusManagerObserver>());

    for (auto& observer : observers) {
        m_focusManager->addObserver(observer);
    }

    // Focus change on DIALOG channel.
    for (auto& observer : observers) {
        observer->expectFocusChange(DIALOG_CHANNEL_NAME, FocusState::FOREGROUND);
    }
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));

    // Focus change on CONTENT channel.
    for (auto& observer : observers) {
        observer->expectFocusChange(CONTENT_CHANNEL_NAME, FocusState::BACKGROUND);
    }
    ASSERT_TRUE(m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_INTERFACE_NAME));

    // Wait for all pending changes to complete.
    for (auto& observer : observers) {
        ASSERT_TRUE(observer->waitForFocusChanges(DEFAULT_TIMEOUT));
    }

    // Drop foreground focus.
    for (auto& observer : observers) {
        observer->expectFocusChange(DIALOG_CHANNEL_NAME, FocusState::NONE);
        observer->expectFocusChange(CONTENT_CHANNEL_NAME, FocusState::FOREGROUND);
    }
    m_focusManager->stopForegroundActivity();

    for (auto& observer : observers) {
        ASSERT_TRUE(observer->waitForFocusChanges(DEFAULT_TIMEOUT));
    }
}

/// Tests that observers can be removed, and that they are no longer notified of focus changes after removal.
TEST_F(FocusManagerTest, removeObserver) {
    // These are all the observers that will ever be added.
    std::vector<std::shared_ptr<MockFocusManagerObserver>> allObservers;

    // Note: StrictMock here so that we fail on unexpected observer callbacks.
    allObservers.push_back(std::make_shared<testing::StrictMock<MockFocusManagerObserver>>());
    allObservers.push_back(std::make_shared<testing::StrictMock<MockFocusManagerObserver>>());

    for (auto& observer : allObservers) {
        m_focusManager->addObserver(observer);
    }

    // These are the observers which are currently added.
    auto activeObservers = allObservers;

    // One focus change with all observers added.
    for (auto& observer : activeObservers) {
        observer->expectFocusChange(DIALOG_CHANNEL_NAME, FocusState::FOREGROUND);
    }
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));

    // Wait for all pending changes to complete.
    for (auto& observer : allObservers) {
        ASSERT_TRUE(observer->waitForFocusChanges(DEFAULT_TIMEOUT));
    }

    // Remove an observer.
    m_focusManager->removeObserver(activeObservers.back());
    activeObservers.pop_back();

    // Now another focus change with the removed observer.
    for (auto& observer : activeObservers) {
        observer->expectFocusChange(CONTENT_CHANNEL_NAME, FocusState::BACKGROUND);
    }
    ASSERT_TRUE(m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_INTERFACE_NAME));

    // Wait for all pending changes to complete.
    for (auto& observer : allObservers) {
        ASSERT_TRUE(observer->waitForFocusChanges(DEFAULT_TIMEOUT));
    }

    // Remove all remaining observers.
    for (auto& observer : activeObservers) {
        m_focusManager->removeObserver(observer);
    }
    activeObservers.clear();

    // And a final focus change with no observers.
    m_focusManager->stopForegroundActivity();

    for (auto& observer : allObservers) {
        ASSERT_TRUE(observer->waitForFocusChanges(DEFAULT_TIMEOUT));
    }
}

/**
 * Tests activityTracker with three Channels and make sure notifyOfActivityUpdates() is called correctly.
 */
TEST_F(FocusManagerTest, activityTracker) {
    // Acquire Content channel and expect notifyOfActivityUpdates() to notify activities on the Content channel.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test1 = {
        {CONTENT_CHANNEL_NAME, CONTENT_INTERFACE_NAME, FocusState::FOREGROUND}};
    ASSERT_TRUE(m_focusManager->acquireChannel(CONTENT_CHANNEL_NAME, contentClient, CONTENT_INTERFACE_NAME));
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test1);

    // Acquire Alert channel and expect notifyOfActivityUpdates() to notify activities to both Content and Alert
    // channels.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test2 = {
        {CONTENT_CHANNEL_NAME, CONTENT_INTERFACE_NAME, FocusState::BACKGROUND},
        {ALERTS_CHANNEL_NAME, ALERTS_INTERFACE_NAME, FocusState::FOREGROUND}};
    ASSERT_TRUE(m_focusManager->acquireChannel(ALERTS_CHANNEL_NAME, alertsClient, ALERTS_INTERFACE_NAME));
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test2);

    // Acquire Dialog channel and expect notifyOfActivityUpdates() to notify activities to both Alert and Dialog
    // channels.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test3 = {
        {ALERTS_CHANNEL_NAME, ALERTS_INTERFACE_NAME, FocusState::BACKGROUND},
        {DIALOG_CHANNEL_NAME, DIALOG_INTERFACE_NAME, FocusState::FOREGROUND}};
    ASSERT_TRUE(m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test3);

    // Release Content channel and expect notifyOfActivityUpdates() to notify activities to Content channel.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test4 = {
        {CONTENT_CHANNEL_NAME, CONTENT_INTERFACE_NAME, FocusState::NONE}};
    ASSERT_TRUE(m_focusManager->releaseChannel(CONTENT_CHANNEL_NAME, contentClient).get());
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test4);

    // Acquire Dialog channel with a different interface and expect notifyOfActivityUpdates() to notify activities to
    // Dialog channels first with focus change on the previous one, and then later with the updated interface.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test5 = {
        {DIALOG_CHANNEL_NAME, DIALOG_INTERFACE_NAME, FocusState::NONE},
        {DIALOG_CHANNEL_NAME, DIFFERENT_DIALOG_INTERFACE_NAME, FocusState::FOREGROUND}};
    ASSERT_TRUE(
        m_focusManager->acquireChannel(DIALOG_CHANNEL_NAME, anotherDialogClient, DIFFERENT_DIALOG_INTERFACE_NAME));
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test5);

    // Release Dialog channel and expect notifyOfActivityUpdates() to notify activities to both Dialog and Alerts
    // channels.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test6 = {
        {DIALOG_CHANNEL_NAME, DIFFERENT_DIALOG_INTERFACE_NAME, FocusState::NONE},
        {ALERTS_CHANNEL_NAME, ALERTS_INTERFACE_NAME, FocusState::FOREGROUND}};
    ASSERT_TRUE(m_focusManager->releaseChannel(DIALOG_CHANNEL_NAME, anotherDialogClient).get());
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test6);

    // Release Alerts channel and expect notifyOfActivityUpdates() to notify activities to Alerts channel.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test7 = {
        {ALERTS_CHANNEL_NAME, ALERTS_INTERFACE_NAME, FocusState::NONE}};
    ASSERT_TRUE(m_focusManager->releaseChannel(ALERTS_CHANNEL_NAME, alertsClient).get());
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test7);
}

/// Test fixture for testing Channel.
class ChannelTest
        : public ::testing::Test
        , public FocusChangeManager {
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
        testChannel = std::make_shared<Channel>(DIALOG_CHANNEL_NAME, DIALOG_CHANNEL_PRIORITY);
    }
};

/// Tests that the getName method of Channel works properly.
TEST_F(ChannelTest, getName) {
    ASSERT_EQ(testChannel->getName(), DIALOG_CHANNEL_NAME);
}

/// Tests that the getPriority method of Channel works properly.
TEST_F(ChannelTest, getPriority) {
    ASSERT_EQ(testChannel->getPriority(), DIALOG_CHANNEL_PRIORITY);
}

/// Tests that the observer properly gets notified of focus changes.
TEST_F(ChannelTest, setObserverThenSetFocus) {
    testChannel->setObserver(clientA);

    ASSERT_TRUE(testChannel->setFocus(FocusState::FOREGROUND));
    assertFocusChange(clientA, FocusState::FOREGROUND);

    ASSERT_TRUE(testChannel->setFocus(FocusState::BACKGROUND));
    assertFocusChange(clientA, FocusState::BACKGROUND);

    ASSERT_TRUE(testChannel->setFocus(FocusState::NONE));
    assertFocusChange(clientA, FocusState::NONE);

    ASSERT_FALSE(testChannel->setFocus(FocusState::NONE));
}

/// Tests that Channels are compared properly
TEST_F(ChannelTest, priorityComparison) {
    std::shared_ptr<Channel> lowerPriorityChannel =
        std::make_shared<Channel>(CONTENT_CHANNEL_NAME, CONTENT_CHANNEL_PRIORITY);
    ASSERT_TRUE(*testChannel > *lowerPriorityChannel);
    ASSERT_FALSE(*lowerPriorityChannel > *testChannel);
}

/// Tests that a Channel correctly reports whether it has an observer.
TEST_F(ChannelTest, hasObserver) {
    ASSERT_FALSE(testChannel->hasObserver());
    testChannel->setObserver(clientA);
    ASSERT_TRUE(testChannel->hasObserver());
    testChannel->setObserver(clientB);
    ASSERT_TRUE(testChannel->hasObserver());
    testChannel->setObserver(nullptr);
    ASSERT_FALSE(testChannel->hasObserver());
}

/*
 * Tests that the timeAtIdle only gets updated when channel goes to idle and not when channel goes to Foreground or
 * Background.
 */
TEST_F(ChannelTest, getTimeAtIdle) {
    auto startTime = testChannel->getState().timeAtIdle;
    ASSERT_TRUE(testChannel->setFocus(FocusState::FOREGROUND));
    auto afterForegroundTime = testChannel->getState().timeAtIdle;
    ASSERT_EQ(startTime, afterForegroundTime);

    ASSERT_TRUE(testChannel->setFocus(FocusState::BACKGROUND));
    auto afterBackgroundTime = testChannel->getState().timeAtIdle;
    ASSERT_EQ(afterBackgroundTime, afterForegroundTime);

    ASSERT_TRUE(testChannel->setFocus(FocusState::NONE));
    auto afterNoneTime = testChannel->getState().timeAtIdle;
    ASSERT_GT(afterNoneTime, afterBackgroundTime);
}

}  // namespace test
}  // namespace afml
}  // namespace alexaClientSDK
