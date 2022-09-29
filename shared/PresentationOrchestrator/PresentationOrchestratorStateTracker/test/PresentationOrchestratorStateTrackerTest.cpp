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

#include "acsdk/PresentationOrchestratorStateTracker/private/PresentationOrchestratorStateTracker.h"

#include <functional>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace alexaClientSDK {
namespace presentationOrchestratorStateTracker {

using namespace ::testing;
using namespace avsCommon;
using namespace avs;
using namespace visualCharacteristicsInterfaces;
using namespace presentationOrchestratorInterfaces;

static const std::string CLIENT_ID = "clientId";
static const std::string CLIENT_ID_2 = "clientId2";
static const std::string WINDOW_ID = "windowId";
static const std::string WINDOW_ID_2 = "windowId2";
static const PresentationMetadata EMPTY_METADATA = {"", "", ""};
static const PresentationMetadata METADATA = {"endpoint", "interface", "metadata"};
static const PresentationMetadata METADATA_2 = {"endpoint2", "interface2", "metadata2"};

class MockVisualCharacteristics : public VisualCharacteristicsInterface {
public:
    MOCK_METHOD0(getVisualCharacteristicsCapabilityConfiguration, void());
};

class MockVisualActivityTracker : public alexaClientSDK::afml::ActivityTrackerInterface {
public:
    MOCK_METHOD1(notifyOfActivityUpdates, void(const std::vector<alexaClientSDK::afml::Channel::State>& channelStates));
};

class MockStateObserver : public PresentationOrchestratorStateObserverInterface {
public:
    MOCK_METHOD2(onStateChanged, void(const std::string& windowId, const PresentationMetadata& metadata));
};

class MockWindowObserver : public PresentationOrchestratorWindowObserverInterface {
public:
    MOCK_METHOD1(onWindowAdded, void(const PresentationOrchestratorWindowInstance& instance));
    MOCK_METHOD1(onWindowModified, void(const PresentationOrchestratorWindowInstance& instance));
    MOCK_METHOD1(onWindowRemoved, void(const std::string& windowId));
};

MATCHER_P(PresentationMetadataEqual, other, "Compares if PresentationMetadata is equal") {
    return arg.endpoint == other.endpoint && arg.interfaceName == other.interfaceName && arg.metadata == other.metadata;
}

MATCHER_P2(ActivityUpdateEqual, focusState, interfaceName, "Compares if Activity Update is Equal") {
    return arg.size() == 1 && arg[0].focusState == focusState && arg[0].interfaceName == interfaceName &&
           arg[0].name == avsCommon::sdkInterfaces::FocusManagerInterface::VISUAL_CHANNEL_NAME;
}

/// Test harness for @c PresentationOrchestratorStateTracker class.
class PresentationOrchestratorStateTrackerTest : public ::testing::Test {
protected:
    /// Set up the test harness for running a test.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    static PresentationOrchestratorWindowInstance generateWindow(const std::string& windowId, int zOrder);

    std::shared_ptr<MockVisualActivityTracker> m_visualActivityTracker;

    std::shared_ptr<MockStateObserver> m_stateObserver;

    std::shared_ptr<MockWindowObserver> m_windowObserver;

    std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> m_executor;

    std::shared_ptr<PresentationOrchestratorStateTracker> m_presentationOrchestrator;
};

void PresentationOrchestratorStateTrackerTest::SetUp() {
    m_visualActivityTracker = std::make_shared<NiceMock<MockVisualActivityTracker>>();
    m_stateObserver = std::make_shared<NiceMock<MockStateObserver>>();
    m_windowObserver = std::make_shared<NiceMock<MockWindowObserver>>();
    m_executor = std::make_shared<alexaClientSDK::avsCommon::utils::threading::Executor>();

    m_presentationOrchestrator = PresentationOrchestratorStateTracker::create(m_visualActivityTracker);

    m_presentationOrchestrator->setExecutor(m_executor);

    m_presentationOrchestrator->addStateObserver(m_stateObserver);
    m_presentationOrchestrator->addWindowObserver(m_windowObserver);

    m_executor->waitForSubmittedTasks();
}

void PresentationOrchestratorStateTrackerTest::TearDown() {
    m_presentationOrchestrator->shutdown();
}

PresentationOrchestratorWindowInstance PresentationOrchestratorStateTrackerTest::generateWindow(
    const std::string& windowId,
    int zOrder) {
    PresentationOrchestratorWindowInstance window;
    window.id = windowId;
    window.zOrderIndex = zOrder;
    return window;
}

TEST_F(PresentationOrchestratorStateTrackerTest, testCreate) {
    EXPECT_TRUE(PresentationOrchestratorStateTracker::create(m_visualActivityTracker) != nullptr);
}

TEST_F(PresentationOrchestratorStateTrackerTest, testAddRemoveStateObserver) {
    auto stateObserver = std::make_shared<NiceMock<MockStateObserver>>();
    m_presentationOrchestrator->addStateObserver(stateObserver);
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));
    EXPECT_CALL(*m_stateObserver, onStateChanged(_, _)).Times(2);
    EXPECT_CALL(*stateObserver, onStateChanged(_, _)).Times(1);
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID, {});
    m_executor->waitForSubmittedTasks();
    m_presentationOrchestrator->removeStateObserver(stateObserver);
    m_presentationOrchestrator->releaseWindow(CLIENT_ID, WINDOW_ID);
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testAddRemoveWindowObserver) {
    auto windowObserver = std::make_shared<NiceMock<MockWindowObserver>>();
    EXPECT_CALL(*m_windowObserver, onWindowAdded(_)).Times(2);
    EXPECT_CALL(*windowObserver, onWindowAdded(_)).Times(1);
    m_presentationOrchestrator->addWindowObserver(windowObserver);
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));
    m_executor->waitForSubmittedTasks();
    m_presentationOrchestrator->removeWindowObserver(windowObserver);
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID_2, 0));
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testAddWindow) {
    EXPECT_CALL(*m_windowObserver, onWindowAdded(_))
        .Times(1)
        .WillOnce(Invoke([](PresentationOrchestratorWindowInstance instance) {
            EXPECT_TRUE(instance.id == WINDOW_ID);
            EXPECT_TRUE(instance.zOrderIndex == 0);
        }));
    EXPECT_TRUE(m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0)));
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testAddDuplicateWindow) {
    EXPECT_CALL(*m_windowObserver, onWindowAdded(_))
        .Times(1)
        .WillOnce(Invoke([](PresentationOrchestratorWindowInstance instance) {
            EXPECT_TRUE(instance.id == WINDOW_ID);
            EXPECT_TRUE(instance.zOrderIndex == 0);
        }));
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));
    EXPECT_FALSE(m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 1)));
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testRemoveWindow) {
    EXPECT_CALL(*m_windowObserver, onWindowRemoved(WINDOW_ID)).Times(1);
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));
    m_presentationOrchestrator->removeWindow(WINDOW_ID);
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testUpdateWindow) {
    auto modifiedWindow = generateWindow(WINDOW_ID, 1);
    EXPECT_CALL(*m_windowObserver, onWindowModified(_))
        .Times(1)
        .WillOnce(Invoke([](PresentationOrchestratorWindowInstance instance) {
            EXPECT_TRUE(instance.id == WINDOW_ID);
            EXPECT_TRUE(instance.zOrderIndex == 1);
        }));
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));
    m_presentationOrchestrator->updateWindow(modifiedWindow);
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testAcquireSingleWindow) {
    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID, PresentationMetadataEqual(METADATA))).Times(1);
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID, METADATA);
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testReleaseSingleWindow) {
    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID, PresentationMetadataEqual(METADATA))).Times(1);
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID, METADATA);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID, PresentationMetadataEqual(EMPTY_METADATA))).Times(1);
    m_presentationOrchestrator->releaseWindow(CLIENT_ID, WINDOW_ID);
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testReleaseWindowTwice) {
    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID, PresentationMetadataEqual(METADATA))).Times(1);
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID, METADATA);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID, PresentationMetadataEqual(EMPTY_METADATA))).Times(1);
    m_presentationOrchestrator->releaseWindow(CLIENT_ID, WINDOW_ID);
    m_presentationOrchestrator->releaseWindow(CLIENT_ID, WINDOW_ID);
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testAcquireMultiWindow) {
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID_2, 1));
    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID, PresentationMetadataEqual(METADATA))).Times(1);
    EXPECT_CALL(
        *m_visualActivityTracker,
        notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::FOREGROUND, METADATA.interfaceName)))
        .Times(1);
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID, METADATA);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID_2, PresentationMetadataEqual(METADATA_2))).Times(1);
    EXPECT_CALL(
        *m_visualActivityTracker,
        notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::FOREGROUND, METADATA_2.interfaceName)))
        .Times(1);
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID_2, METADATA_2);
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testAcquireReverseZorderMultiWindow) {
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID_2, 1));
    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID_2, PresentationMetadataEqual(METADATA_2))).Times(1);
    EXPECT_CALL(
        *m_visualActivityTracker,
        notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::FOREGROUND, METADATA_2.interfaceName)))
        .Times(1);
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID_2, METADATA_2);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID, PresentationMetadataEqual(METADATA))).Times(1);
    // Not expecting the second visual activity tracker update as window 2 is above window 1
    EXPECT_CALL(*m_visualActivityTracker, notifyOfActivityUpdates(_)).Times(0);
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID, METADATA);
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testReleaseMultiWindow) {
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID_2, 1));
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID, METADATA);
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID_2, METADATA_2);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID_2, PresentationMetadataEqual(EMPTY_METADATA))).Times(1);
    EXPECT_CALL(
        *m_visualActivityTracker,
        notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::FOREGROUND, METADATA.interfaceName)))
        .Times(1);
    m_presentationOrchestrator->releaseWindow(CLIENT_ID, WINDOW_ID_2);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID, PresentationMetadataEqual(EMPTY_METADATA))).Times(1);
    EXPECT_CALL(*m_visualActivityTracker, notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::NONE, ""))).Times(1);
    m_presentationOrchestrator->releaseWindow(CLIENT_ID, WINDOW_ID);
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testMultiWindowSameZorder) {
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID_2, 0));

    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID, PresentationMetadataEqual(METADATA))).Times(1);
    EXPECT_CALL(
        *m_visualActivityTracker,
        notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::FOREGROUND, METADATA.interfaceName)))
        .Times(1);
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID, METADATA);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID_2, PresentationMetadataEqual(METADATA_2))).Times(1);
    EXPECT_CALL(
        *m_visualActivityTracker,
        notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::FOREGROUND, METADATA_2.interfaceName)))
        .Times(1);
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID_2, METADATA_2);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID, PresentationMetadataEqual(EMPTY_METADATA))).Times(1);
    EXPECT_CALL(*m_visualActivityTracker, notifyOfActivityUpdates(_)).Times(0);
    m_presentationOrchestrator->releaseWindow(CLIENT_ID, WINDOW_ID);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID_2, PresentationMetadataEqual(EMPTY_METADATA))).Times(1);
    EXPECT_CALL(*m_visualActivityTracker, notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::NONE, ""))).Times(1);
    m_presentationOrchestrator->releaseWindow(CLIENT_ID, WINDOW_ID_2);
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testReacquireWindow) {
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));

    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID, PresentationMetadataEqual(METADATA))).Times(1);
    EXPECT_CALL(
        *m_visualActivityTracker,
        notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::FOREGROUND, METADATA.interfaceName)))
        .Times(1);
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID, METADATA);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID, PresentationMetadataEqual(METADATA_2))).Times(1);
    EXPECT_CALL(
        *m_visualActivityTracker,
        notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::FOREGROUND, METADATA_2.interfaceName)))
        .Times(1);
    m_presentationOrchestrator->acquireWindow(CLIENT_ID_2, WINDOW_ID, METADATA_2);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID, PresentationMetadataEqual(METADATA))).Times(1);
    EXPECT_CALL(
        *m_visualActivityTracker,
        notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::FOREGROUND, METADATA.interfaceName)))
        .Times(1);
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID, METADATA);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_stateObserver, onStateChanged(WINDOW_ID, PresentationMetadataEqual(METADATA_2))).Times(1);
    EXPECT_CALL(
        *m_visualActivityTracker,
        notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::FOREGROUND, METADATA_2.interfaceName)))
        .Times(1);
    m_presentationOrchestrator->releaseWindow(CLIENT_ID, WINDOW_ID);
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testSetReleaseDeviceInterface) {
    EXPECT_CALL(
        *m_visualActivityTracker, notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::FOREGROUND, "DEVICE")))
        .Times(1);
    m_presentationOrchestrator->setDeviceInterface("DEVICE");
    EXPECT_CALL(*m_visualActivityTracker, notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::NONE, ""))).Times(1);
    m_presentationOrchestrator->releaseDeviceInterface();
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testDeviceInterfaceWithWindows) {
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));

    EXPECT_CALL(
        *m_visualActivityTracker,
        notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::FOREGROUND, METADATA.interfaceName)))
        .Times(1);
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID, METADATA);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_visualActivityTracker, notifyOfActivityUpdates(_)).Times(0);
    m_presentationOrchestrator->setDeviceInterface("DEVICE");
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(
        *m_visualActivityTracker, notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::FOREGROUND, "DEVICE")))
        .Times(1);
    m_presentationOrchestrator->releaseWindow(CLIENT_ID, WINDOW_ID);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_visualActivityTracker, notifyOfActivityUpdates(ActivityUpdateEqual(FocusState::NONE, "")));
    m_presentationOrchestrator->releaseDeviceInterface();
    m_executor->waitForSubmittedTasks();
}

TEST_F(PresentationOrchestratorStateTrackerTest, testGetWindowInfo) {
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID, 0));
    m_presentationOrchestrator->addWindow(generateWindow(WINDOW_ID_2, 1));
    m_presentationOrchestrator->acquireWindow(CLIENT_ID, WINDOW_ID, METADATA);
    auto info = m_presentationOrchestrator->getWindowInformation();
    EXPECT_TRUE(info.size() == 2);
    if (info[0].configuration.id == WINDOW_ID) {
        EXPECT_TRUE(info[0].configuration.zOrderIndex == 0);
        EXPECT_TRUE(info[0].state.interfaceName == METADATA.interfaceName);
        EXPECT_TRUE(info[0].state.metadata == METADATA.metadata);
        EXPECT_TRUE(info[1].configuration.id == WINDOW_ID_2);
        EXPECT_TRUE(info[1].configuration.zOrderIndex == 1);
        EXPECT_TRUE(info[1].state.interfaceName.empty());
        EXPECT_TRUE(info[1].state.metadata.empty());
    } else {
        EXPECT_TRUE(info[1].configuration.id == WINDOW_ID);
        EXPECT_TRUE(info[1].configuration.zOrderIndex == 0);
        EXPECT_TRUE(info[1].state.interfaceName == METADATA.interfaceName);
        EXPECT_TRUE(info[1].state.metadata == METADATA.metadata);
        EXPECT_TRUE(info[0].configuration.id == WINDOW_ID_2);
        EXPECT_TRUE(info[0].configuration.zOrderIndex == 1);
        EXPECT_TRUE(info[0].state.interfaceName.empty());
        EXPECT_TRUE(info[0].state.metadata.empty());
    }
}

}  // namespace presentationOrchestratorStateTracker
}  // namespace alexaClientSDK