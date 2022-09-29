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

#include "acsdk/PresentationOrchestratorClient/private/PresentationOrchestratorClient.h"

#include <chrono>
#include <vector>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <acsdk/PresentationOrchestratorInterfaces/MockPresentationObserver.h>
#include <acsdk/PresentationOrchestratorInterfaces/MockPresentationOrchestratorStateTracker.h>
#include <acsdk/PresentationOrchestratorInterfaces/MockVisualTimeoutManager.h>
#include <AVSCommon/Utils/PromiseFuturePair.h>

namespace alexaClientSDK {
namespace presentationOrchestratorClient {
namespace test {

using namespace ::testing;
using namespace avsCommon;
using namespace sdkInterfaces;
using namespace presentationOrchestratorInterfaces;
using namespace presentationOrchestratorInterfaces::test;

static const auto SHORT_DELAY = std::chrono::milliseconds(500);
static const char CLIENT_ID[] = "clientId";
static const char WINDOW_ID_1[] = "windowId1";
static const char WINDOW_ID_2[] = "windowId2";
static const char INTERFACE_1[] = "interface1";
static const char INTERFACE_2[] = "interface2";
static const char METADATA_1[] = "metadata1";
static const char METADATA_2[] = "metadata2";
static const auto TIMEOUT = std::chrono::milliseconds(500);

/// Default timeout for SHORT presentations.
static const std::chrono::milliseconds DEFAULT_TIMEOUT_SHORT_PRESENTATION_MS{30000};

/// Test harness for @c PresentationOrchestratorClient class.
class PresentationOrchestratorClientTest : public ::testing::Test {
public:
    PresentationOrchestratorClientTest() = default;

    /// Set up the test harness for running a test.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

protected:
    /// Mock of @c PresentationOrchestratorStateTracker.
    std::shared_ptr<MockPresentationOrchestratorStateTracker> m_stateTracker;

    /// Mock of @c PresentationObserver.
    std::shared_ptr<MockPresentationObserver> m_presentationObserver;

    /// Mock of @c VisualTimeoutManager.
    std::shared_ptr<MockVisualTimeoutManager> m_visualTimeoutManager;

    /// An instance of the @c PresentationOrchestratorClient per test.
    std::shared_ptr<PresentationOrchestratorClient> m_presentationOrchestratorClient;

    /// An instance of @c Executor used by PO Client.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> m_executor;
};

void PresentationOrchestratorClientTest::SetUp() {
    m_presentationObserver = std::make_shared<NiceMock<MockPresentationObserver>>();
    m_stateTracker = std::make_shared<NiceMock<MockPresentationOrchestratorStateTracker>>();
    m_visualTimeoutManager = std::make_shared<NiceMock<MockVisualTimeoutManager>>();
    m_executor = std::make_shared<alexaClientSDK::avsCommon::utils::threading::Executor>();

    m_presentationOrchestratorClient =
        PresentationOrchestratorClient::create(CLIENT_ID, m_stateTracker, m_visualTimeoutManager);
    m_presentationOrchestratorClient->setExecutor(m_executor);
}

void PresentationOrchestratorClientTest::TearDown() {
    m_presentationOrchestratorClient->shutdown();
    m_presentationOrchestratorClient.reset();
    m_presentationObserver.reset();
    m_stateTracker.reset();
    m_visualTimeoutManager.reset();
}

PresentationOptions generatePresentationOptions(
    const std::chrono::milliseconds& timeout,
    const PresentationLifespan& lifespan,
    const std::string& interfaceName,
    const std::string& metadata) {
    PresentationOptions options;
    options.timeout = std::chrono::milliseconds(timeout);
    options.presentationLifespan = lifespan;
    options.interfaceName = interfaceName;
    options.metadata = metadata;
    return options;
}

PresentationOrchestratorWindowInstance generateWindowInstance(
    const std::string& windowId,
    const int& zOrder,
    const std::vector<std::string>& supportedInterfaces) {
    PresentationOrchestratorWindowInstance window;
    window.id = windowId;
    window.zOrderIndex = zOrder;
    window.supportedInterfaces = supportedInterfaces;
    return window;
}

/**
 * Tests requestWindow for a windowId not recognized by PO Client
 */
TEST_F(PresentationOrchestratorClientTest, testRequestWindowInvalidWindowId) {
    auto presentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);

    // promise value won't be set as acquire window isn't supposed to be called
    avsCommon::utils::PromiseFuturePair<void> onAcquireWindowCalled;
    EXPECT_CALL(*m_stateTracker, acquireWindow(_, _, _)).Times(Exactly(0));
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _)).Times(Exactly(0));
    EXPECT_CALL(*m_visualTimeoutManager, requestTimeout(_, _)).Times(Exactly(0));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, presentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();

    ASSERT_FALSE(onAcquireWindowCalled.waitFor(SHORT_DELAY));
}

/**
 * Tests requestWindow for a presentation with an interface not supported by requested window
 */
TEST_F(PresentationOrchestratorClientTest, testRequestWindowUnsupportedInterface) {
    auto presentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);
    std::vector<std::string> supportedInterfaces = {INTERFACE_2};
    auto windowInstance = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance);

    // promise value won't be set as acquire window isn't supposed to be called
    avsCommon::utils::PromiseFuturePair<void> onAcquireWindowCalled;
    EXPECT_CALL(*m_stateTracker, acquireWindow(_, _, _)).Times(Exactly(0));
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _)).Times(Exactly(0));
    EXPECT_CALL(*m_visualTimeoutManager, requestTimeout(_, _)).Times(Exactly(0));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, presentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();

    ASSERT_FALSE(onAcquireWindowCalled.waitFor(SHORT_DELAY));
}

/**
 * Tests whether acquireWindow of state tracker is called with the correct params post requestWindow
 */
TEST_F(PresentationOrchestratorClientTest, testAddAndRequestWindow) {
    std::vector<std::string> supportedInterfaces = {INTERFACE_1};
    auto windowInstance = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance);

    auto expectedOptions = generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<void> onAcquireWindowCalled;
    EXPECT_CALL(*m_stateTracker, acquireWindow(_, _, _))
        .Times(Exactly(1))
        .WillOnce(
            Invoke([&expectedOptions, &onAcquireWindowCalled](
                       const std::string& clientId, const std::string& windowId, const PresentationMetadata& metadata) {
                ASSERT_EQ(CLIENT_ID, clientId);
                ASSERT_EQ(WINDOW_ID_1, windowId);
                ASSERT_EQ(expectedOptions.metadata, metadata.metadata);
                ASSERT_EQ(expectedOptions.interfaceName, metadata.interfaceName);
                onAcquireWindowCalled.setValue();
            }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, expectedOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();

    ASSERT_TRUE(onAcquireWindowCalled.waitFor(SHORT_DELAY));
}

/**
 * Tests whether timeout is requested on requestedWindow and stopped on window removal
 */
TEST_F(PresentationOrchestratorClientTest, testAddRequestAndRemoveWindowWithTimeouts) {
    std::vector<std::string> supportedInterfaces = {INTERFACE_1};
    auto windowInstance = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance);

    auto expectedOptions = generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<void> onAcquireWindowCalled;
    EXPECT_CALL(*m_stateTracker, acquireWindow(_, _, _))
        .Times(Exactly(1))
        .WillOnce(
            Invoke([&onAcquireWindowCalled](
                       const std::string& clientId, const std::string& windowId, const PresentationMetadata& metadata) {
                onAcquireWindowCalled.setValue();
            }));

    VisualTimeoutManagerInterface::VisualTimeoutId expectedTimeoutId = 3;
    EXPECT_CALL(*m_visualTimeoutManager, requestTimeout(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([&expectedTimeoutId, &expectedOptions](
                             const std::chrono::milliseconds& delay,
                             const VisualTimeoutManagerInterface::VisualTimeoutCallback& timeoutCallback) {
            EXPECT_EQ(expectedOptions.timeout, delay);
            return expectedTimeoutId;
        }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, expectedOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onAcquireWindowCalled.waitFor(SHORT_DELAY));

    EXPECT_CALL(*m_visualTimeoutManager, stopTimeout(_))
        .Times(Exactly(1))
        .WillOnce(Invoke([&expectedTimeoutId](const VisualTimeoutManagerInterface::VisualTimeoutId& timeoutId) {
            EXPECT_TRUE(expectedTimeoutId == timeoutId);
            return true;
        }));
    EXPECT_CALL(*m_stateTracker, getFocusedWindowId()).Times(1).WillOnce(Invoke([]() { return ""; }));

    m_presentationOrchestratorClient->onWindowRemoved(WINDOW_ID_1);
    m_executor->waitForSubmittedTasks();
}

/**
 * Tests whether presentation request tokens are returned without awaiting for executor tasks to be completed
 */
TEST_F(PresentationOrchestratorClientTest, testRequestTokens) {
    std::vector<std::string> supportedInterfaces = {INTERFACE_1};
    auto windowInstance = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance);

    auto expectedOptions = generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);

    auto requestToken1 =
        m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, expectedOptions, m_presentationObserver);
    auto requestToken2 =
        m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, expectedOptions, m_presentationObserver);
    auto requestToken3 =
        m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, expectedOptions, m_presentationObserver);

    ASSERT_EQ(requestToken1, PresentationRequestToken(0));
    ASSERT_EQ(requestToken2, PresentationRequestToken(1));
    ASSERT_EQ(requestToken3, PresentationRequestToken(2));
}

/**
 * Tests presentation observer is notified of availability post requestWindow
 */
TEST_F(PresentationOrchestratorClientTest, testOnPresentationAvailable) {
    std::vector<std::string> supportedInterfaces = {INTERFACE_1};
    auto windowInstance = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance);

    auto expectedOptions = generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<PresentationRequestToken> onPresentationAvailableCalled;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([&onPresentationAvailableCalled](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            ASSERT_EQ(presentation->getState(), PresentationState::FOREGROUND);
            onPresentationAvailableCalled.setValue(requestToken);
        }));

    auto requestToken =
        m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, expectedOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();

    ASSERT_TRUE(onPresentationAvailableCalled.waitFor(SHORT_DELAY));
    ASSERT_EQ(onPresentationAvailableCalled.getValue(), requestToken);
}

/**
 * Tests updatePresentationMetadata of state tracker is invoked with correct params
 * Window was requested previously and should be in foreground focused state before another requestWindow call
 */
TEST_F(PresentationOrchestratorClientTest, testMetadataUpdate) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    PresentationOrchestratorWindowInstance windowInstance1 =
        generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);

    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);

    auto presentationOptions1 =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<void> onAcquireWindowCalled;
    EXPECT_CALL(*m_stateTracker, acquireWindow(_, _, _))
        .Times(Exactly(1))
        .WillOnce(
            Invoke([&presentationOptions1, &onAcquireWindowCalled](
                       const std::string& clientId, const std::string& windowId, const PresentationMetadata& metadata) {
                ASSERT_EQ(CLIENT_ID, clientId);
                ASSERT_EQ(WINDOW_ID_1, windowId);
                ASSERT_EQ(presentationOptions1.metadata, metadata.metadata);
                ASSERT_EQ(presentationOptions1.interfaceName, metadata.interfaceName);
                onAcquireWindowCalled.setValue();
            }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, presentationOptions1, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onAcquireWindowCalled.waitFor(SHORT_DELAY));

    auto presentationOptions2 =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::TRANSIENT, INTERFACE_1, METADATA_1);
    avsCommon::utils::PromiseFuturePair<void> onUpdatePresentationMetadataCalled;
    EXPECT_CALL(*m_stateTracker, updatePresentationMetadata(_, _, _))
        .Times(Exactly(1))
        .WillOnce(
            Invoke([&presentationOptions2, &onUpdatePresentationMetadataCalled](
                       const std::string& clientId, const std::string& windowId, const PresentationMetadata& metadata) {
                ASSERT_EQ(CLIENT_ID, clientId);
                ASSERT_EQ(WINDOW_ID_1, windowId);
                ASSERT_EQ(presentationOptions2.metadata, metadata.metadata);
                ASSERT_EQ(presentationOptions2.interfaceName, metadata.interfaceName);
                onUpdatePresentationMetadataCalled.setValue();
            }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, presentationOptions2, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onUpdatePresentationMetadataCalled.waitFor(SHORT_DELAY));
}

/**
 * Tests state of presentations from two windows having same zOrderIndex on requestWindow and clearPresentations
 */
TEST_F(PresentationOrchestratorClientTest, testRequestAndClearMultipleWindowsWithSameZOrderIndex) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    std::vector<std::string> supportedInterfaces2 = {INTERFACE_2};
    auto windowInstance2 = generateWindowInstance(WINDOW_ID_2, 2, supportedInterfaces2);

    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance2);

    auto presentationOptions1 =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);
    auto presentationOptions2 =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::SHORT, INTERFACE_2, METADATA_2);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPresentationAvailable1;
    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPresentationAvailable2;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&onPresentationAvailable1](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onPresentationAvailable1.setValue(presentation);
        }))
        .WillOnce(Invoke([&onPresentationAvailable2](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onPresentationAvailable2.setValue(presentation);
        }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, presentationOptions1, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailable1.waitFor(SHORT_DELAY));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_2, presentationOptions2, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailable2.waitFor(SHORT_DELAY));

    // foregrounded presentation from other window must be unfocused
    auto presentation1 = onPresentationAvailable1.getValue();
    ASSERT_EQ(presentation1->getState(), PresentationState::FOREGROUND_UNFOCUSED);

    auto presentation2 = onPresentationAvailable2.getValue();
    ASSERT_EQ(presentation2->getState(), PresentationState::FOREGROUND);

    // Expect both windows to be released
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([](const std::string& clientId, const std::string& windowId) {
            ASSERT_EQ(CLIENT_ID, clientId);
            ASSERT_EQ(WINDOW_ID_2, windowId);
        }))
        .WillOnce(Invoke([](const std::string& clientId, const std::string& windowId) {
            ASSERT_EQ(CLIENT_ID, clientId);
            ASSERT_EQ(WINDOW_ID_1, windowId);
        }));

    m_presentationOrchestratorClient->clearPresentations();
    m_executor->waitForSubmittedTasks();

    ASSERT_EQ(presentation1->getState(), PresentationState::NONE);
    ASSERT_EQ(presentation2->getState(), PresentationState::NONE);
}

/**
 * Tests state of presentations from two windows having different zOrderIndex on requestWindow and clearPresentations
 * The second requested window has a lower zOrderIndex than the first window in this case
 */
TEST_F(PresentationOrchestratorClientTest, testRequestAndClearMultipleWindowsWithDifferentZOrderIndex) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 3, supportedInterfaces1);
    std::vector<std::string> supportedInterfaces2 = {INTERFACE_2};
    auto windowInstance2 = generateWindowInstance(WINDOW_ID_2, 2, supportedInterfaces2);

    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance2);

    auto presentationOptions1 =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);
    auto presentationOptions2 =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::SHORT, INTERFACE_2, METADATA_2);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPresentationAvailable1;
    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPresentationAvailable2;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&onPresentationAvailable1](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onPresentationAvailable1.setValue(presentation);
        }))
        .WillOnce(Invoke([&onPresentationAvailable2](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onPresentationAvailable2.setValue(presentation);
        }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, presentationOptions1, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailable1.waitFor(SHORT_DELAY));

    // Expect WINDOW_ID_1 to be released when WINDOW_ID_2 is foregrounded
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::string& clientId, const std::string& windowId) {
            ASSERT_EQ(CLIENT_ID, clientId);
            ASSERT_EQ(WINDOW_ID_1, windowId);
        }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_2, presentationOptions2, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailable2.waitFor(SHORT_DELAY));

    // presentations from windows with higher zOrderIndex must be cleared
    auto presentation1 = onPresentationAvailable1.getValue();
    ASSERT_EQ(presentation1->getState(), PresentationState::NONE);

    auto presentation2 = onPresentationAvailable2.getValue();
    ASSERT_EQ(presentation2->getState(), PresentationState::FOREGROUND);

    // Expect WINDOW_ID_2 to be released
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::string& clientId, const std::string& windowId) {
            ASSERT_EQ(CLIENT_ID, clientId);
            ASSERT_EQ(WINDOW_ID_2, windowId);
        }));

    m_presentationOrchestratorClient->clearPresentations();
    m_executor->waitForSubmittedTasks();

    ASSERT_EQ(presentation1->getState(), PresentationState::NONE);
    ASSERT_EQ(presentation2->getState(), PresentationState::NONE);
}

/**
 * Tests back navigation on a window with two presentations
 * When both presentations are dismissed, releaseWindow of state tracker should be invoked
 */
TEST_F(PresentationOrchestratorClientTest, testNavigateBack) {
    std::vector<std::string> supportedInterfaces = {INTERFACE_1};
    auto windowInstance = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance);

    auto presentationOptions1 =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);
    auto presentationOptions2 =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::TRANSIENT, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<void> onPresentationAvailableCalled1;
    avsCommon::utils::PromiseFuturePair<void> onPresentationAvailableCalled2;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&onPresentationAvailableCalled1](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            ASSERT_EQ(presentation->getState(), PresentationState::FOREGROUND);
            onPresentationAvailableCalled1.setValue();
        }))
        .WillOnce(Invoke([&onPresentationAvailableCalled2](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            ASSERT_EQ(presentation->getState(), PresentationState::FOREGROUND);
            onPresentationAvailableCalled2.setValue();
        }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, presentationOptions1, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailableCalled1.waitFor(SHORT_DELAY));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, presentationOptions2, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailableCalled2.waitFor(SHORT_DELAY));

    EXPECT_CALL(*m_stateTracker, getFocusedWindowId()).WillRepeatedly(Invoke([]() { return WINDOW_ID_1; }));
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _)).Times(0);
    EXPECT_CALL(*m_presentationObserver, onNavigateBack(_))
        .Times(Exactly(2))
        .WillRepeatedly(
            Invoke([](const presentationOrchestratorInterfaces::PresentationRequestToken id) { return false; }));

    // should not invoke releaseWindow of state tracker
    m_presentationOrchestratorClient->navigateBack();
    m_executor->waitForSubmittedTasks();

    avsCommon::utils::PromiseFuturePair<void> releaseWindowCalled;
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _))
        .Times(1)
        .WillOnce(Invoke([&releaseWindowCalled](const std::string& clientId, const std::string& windowId) {
            ASSERT_EQ(CLIENT_ID, clientId);
            ASSERT_EQ(WINDOW_ID_1, windowId);
            releaseWindowCalled.setValue();
        }));

    // should invoke releaseWindow of state tracker
    m_presentationOrchestratorClient->navigateBack();
    m_executor->waitForSubmittedTasks();

    ASSERT_TRUE(releaseWindowCalled.waitFor(SHORT_DELAY));
}

/**
 * Tests back navigation with two windows, each with one presentation (with LONG lifespans).
 * When the topmost presentation is dismissed, the window should be released and the topmost presentation of the
 * next window (if one exists) should then be foregrounded.
 */
TEST_F(PresentationOrchestratorClientTest, testNavigateBackMultipleWindowsSingleLongPresentationEach) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    std::vector<std::string> supportedInterfaces2 = {INTERFACE_2};
    auto windowInstance2 = generateWindowInstance(WINDOW_ID_2, 2, supportedInterfaces2);

    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance2);

    auto presentationOptions1 =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);
    auto presentationOptions2 =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_2, METADATA_2);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPresentationAvailable1;
    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPresentationAvailable2;

    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&onPresentationAvailable1](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onPresentationAvailable1.setValue(presentation);
        }))
        .WillOnce(Invoke([&onPresentationAvailable2](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onPresentationAvailable2.setValue(presentation);
        }));

    auto presentation1RequestToken =
        m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, presentationOptions1, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailable1.waitFor(SHORT_DELAY));

    auto presentation2RequestToken =
        m_presentationOrchestratorClient->requestWindow(WINDOW_ID_2, presentationOptions2, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailable2.waitFor(SHORT_DELAY));

    auto presentation1 = onPresentationAvailable1.getValue();
    ASSERT_EQ(presentation1->getState(), PresentationState::FOREGROUND_UNFOCUSED);

    auto presentation2 = onPresentationAvailable2.getValue();
    ASSERT_EQ(presentation2->getState(), PresentationState::FOREGROUND);

    EXPECT_CALL(*m_stateTracker, getFocusedWindowId())
        .WillOnce(Invoke([]() { return WINDOW_ID_2; }))
        .WillRepeatedly(Invoke([]() { return WINDOW_ID_1; }));

    EXPECT_CALL(*m_presentationObserver, onNavigateBack(_))
        .Times(Exactly(2))
        .WillRepeatedly(
            Invoke([](const presentationOrchestratorInterfaces::PresentationRequestToken id) { return false; }));

    avsCommon::utils::PromiseFuturePair<void> onPresentation1StateChangedForeground;
    avsCommon::utils::PromiseFuturePair<void> onPresentation2StateChangedNone;

    // There should be 2 state changes on the first navigateBack():
    // 1. presentation2 state should go from FOREGROUND to NONE, and,
    // 2. presentation1 state should go from FOREGROUND_UNFOCUSED to FOREGROUND
    EXPECT_CALL(*m_presentationObserver, onPresentationStateChanged(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([presentation2RequestToken, &onPresentation2StateChangedNone](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            ASSERT_EQ(id, presentation2RequestToken);
            ASSERT_EQ(newState, PresentationState::NONE);
            onPresentation2StateChangedNone.setValue();
        }))
        .WillOnce(Invoke([presentation1RequestToken, &onPresentation1StateChangedForeground](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            ASSERT_EQ(id, presentation1RequestToken);
            ASSERT_EQ(newState, PresentationState::FOREGROUND);
            onPresentation1StateChangedForeground.setValue();
        }));

    // WINDOW_ID_2 should be released when navigateBack() is called on presentation2
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::string& clientId, const std::string& windowId) {
            ASSERT_EQ(CLIENT_ID, clientId);
            ASSERT_EQ(WINDOW_ID_2, windowId);
        }));

    // navigateBack() on presentation2
    m_presentationOrchestratorClient->navigateBack();
    m_executor->waitForSubmittedTasks();

    onPresentation2StateChangedNone.waitFor(TIMEOUT);
    // Confirm that presentation2 has been dismissed
    ASSERT_EQ(presentation2->getState(), PresentationState::NONE);

    onPresentation1StateChangedForeground.waitFor(TIMEOUT);
    // Confirm that presentation1 is in the foreground
    ASSERT_EQ(presentation1->getState(), PresentationState::FOREGROUND);

    avsCommon::utils::PromiseFuturePair<void> onPresentation1StateChangedNone;
    // On the next navigateBack(), one state change should occur:
    // presentation1 state should go from FOREGROUND to NONE
    EXPECT_CALL(*m_presentationObserver, onPresentationStateChanged(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([presentation1RequestToken, &onPresentation1StateChangedNone](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            ASSERT_EQ(id, presentation1RequestToken);
            ASSERT_EQ(newState, PresentationState::NONE);
            onPresentation1StateChangedNone.setValue();
        }));

    // WINDOW_ID_1 should be released when navigateBack() is called on presentation1
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::string& clientId, const std::string& windowId) {
            ASSERT_EQ(CLIENT_ID, clientId);
            ASSERT_EQ(WINDOW_ID_1, windowId);
        }));

    // navigateBack() on presentation1
    m_presentationOrchestratorClient->navigateBack();
    m_executor->waitForSubmittedTasks();

    onPresentation1StateChangedNone.waitFor(TIMEOUT);
    // Confirm that presentation1 has been dismissed
    ASSERT_EQ(presentation1->getState(), PresentationState::NONE);
}

/**
 * Tests back navigation with two windows each having a presentation with a different lifespan.
 * When the topmost presentation (with LONG lifespan) is dismissed, the window should be released and the PERMANENT
 * presentation of the next window should then be foregrounded. navigateBack() on the PERMANENT presentation should not
 * release the window or cause a state change.
 */
TEST_F(PresentationOrchestratorClientTest, testNavigateBackMultipleWindowsLongOnPermanent) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    std::vector<std::string> supportedInterfaces2 = {INTERFACE_2};
    auto windowInstance2 = generateWindowInstance(WINDOW_ID_2, 2, supportedInterfaces2);

    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance2);

    auto presentationOptions1 =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::PERMANENT, INTERFACE_1, METADATA_1);
    auto presentationOptions2 =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_2, METADATA_2);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPresentationAvailable1;
    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPresentationAvailable2;

    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&onPresentationAvailable1](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onPresentationAvailable1.setValue(presentation);
        }))
        .WillOnce(Invoke([&onPresentationAvailable2](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onPresentationAvailable2.setValue(presentation);
        }));

    auto presentation1RequestToken =
        m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, presentationOptions1, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailable1.waitFor(SHORT_DELAY));

    auto presentation2RequestToken =
        m_presentationOrchestratorClient->requestWindow(WINDOW_ID_2, presentationOptions2, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailable2.waitFor(SHORT_DELAY));

    auto presentation1 = onPresentationAvailable1.getValue();
    ASSERT_EQ(presentation1->getState(), PresentationState::FOREGROUND_UNFOCUSED);

    auto presentation2 = onPresentationAvailable2.getValue();
    ASSERT_EQ(presentation2->getState(), PresentationState::FOREGROUND);

    EXPECT_CALL(*m_stateTracker, getFocusedWindowId())
        .WillOnce(Invoke([]() { return WINDOW_ID_2; }))
        .WillRepeatedly(Invoke([]() { return WINDOW_ID_1; }));
    EXPECT_CALL(*m_presentationObserver, onNavigateBack(_))
        .Times(Exactly(2))
        .WillRepeatedly(
            Invoke([](const presentationOrchestratorInterfaces::PresentationRequestToken id) { return false; }));

    avsCommon::utils::PromiseFuturePair<void> onPresentation1StateChangedForeground;
    avsCommon::utils::PromiseFuturePair<void> onPresentation2StateChangedNone;

    // There should be 2 state changes on the first navigateBack():
    // 1. presentation2 state should go from FOREGROUND to NONE, and,
    // 2. presentation1 state should go from FOREGROUND_UNFOCUSED to FOREGROUND
    EXPECT_CALL(*m_presentationObserver, onPresentationStateChanged(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([presentation2RequestToken, &onPresentation2StateChangedNone](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            ASSERT_EQ(id, presentation2RequestToken);
            ASSERT_EQ(newState, PresentationState::NONE);
            onPresentation2StateChangedNone.setValue();
        }))
        .WillOnce(Invoke([presentation1RequestToken, &onPresentation1StateChangedForeground](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            ASSERT_EQ(id, presentation1RequestToken);
            ASSERT_EQ(newState, PresentationState::FOREGROUND);
            onPresentation1StateChangedForeground.setValue();
        }));

    // WINDOW_ID_2 should be released when navigateBack() is called on presentation2
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::string& clientId, const std::string& windowId) {
            ASSERT_EQ(CLIENT_ID, clientId);
            ASSERT_EQ(WINDOW_ID_2, windowId);
        }));

    // navigateBack() on presentation2
    m_presentationOrchestratorClient->navigateBack();
    m_executor->waitForSubmittedTasks();

    onPresentation2StateChangedNone.waitFor(TIMEOUT);
    // Confirm that presentation2 has been dismissed
    ASSERT_EQ(presentation2->getState(), PresentationState::NONE);

    onPresentation1StateChangedForeground.waitFor(TIMEOUT);
    // Confirm that presentation1 is in the foreground
    ASSERT_EQ(presentation1->getState(), PresentationState::FOREGROUND);

    // No state changes are expected when navigateBack() is called on presentation1
    EXPECT_CALL(*m_presentationObserver, onPresentationStateChanged(_, _)).Times(Exactly(0));

    // WINDOW_ID_1 should not be released when navigateBack() is called on presentation1
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _)).Times(Exactly(0));

    // navigateBack() on presentation1
    m_presentationOrchestratorClient->navigateBack();
    m_executor->waitForSubmittedTasks();
    // Confirm that presentation1 is still in the foreground
    ASSERT_EQ(presentation1->getState(), PresentationState::FOREGROUND);

    // WINDOW_ID_1 should be released during tear down
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::string& clientId, const std::string& windowId) {
            ASSERT_EQ(CLIENT_ID, clientId);
            ASSERT_EQ(WINDOW_ID_1, windowId);
        }));

    // presentation1 state should go from FOREGROUND to NONE
    EXPECT_CALL(*m_presentationObserver, onPresentationStateChanged(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([presentation1RequestToken](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            ASSERT_EQ(id, presentation1RequestToken);
            ASSERT_EQ(newState, PresentationState::NONE);
        }));
}

/**
 * Tests back navigation with two windows each having a presentation with a different lifespan.
 * When navigateBack() on the topmost presentation (with PERMANENT lifespan) is invoked, there should be no state
 * change.
 */
TEST_F(PresentationOrchestratorClientTest, testNavigateBackMultipleWindowsPermanentOnLong) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    std::vector<std::string> supportedInterfaces2 = {INTERFACE_2};
    auto windowInstance2 = generateWindowInstance(WINDOW_ID_2, 2, supportedInterfaces2);

    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance2);

    auto presentationOptions1 =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);
    auto presentationOptions2 =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::PERMANENT, INTERFACE_2, METADATA_2);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPresentationAvailable1;
    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPresentationAvailable2;

    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&onPresentationAvailable1](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onPresentationAvailable1.setValue(presentation);
        }))
        .WillOnce(Invoke([&onPresentationAvailable2](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onPresentationAvailable2.setValue(presentation);
        }));

    auto presentation1RequestToken =
        m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, presentationOptions1, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailable1.waitFor(SHORT_DELAY));

    auto presentation2RequestToken =
        m_presentationOrchestratorClient->requestWindow(WINDOW_ID_2, presentationOptions2, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailable2.waitFor(SHORT_DELAY));

    auto presentation1 = onPresentationAvailable1.getValue();
    ASSERT_EQ(presentation1->getState(), PresentationState::FOREGROUND_UNFOCUSED);

    auto presentation2 = onPresentationAvailable2.getValue();
    ASSERT_EQ(presentation2->getState(), PresentationState::FOREGROUND);

    EXPECT_CALL(*m_stateTracker, getFocusedWindowId()).WillOnce(Invoke([]() { return WINDOW_ID_2; }));
    EXPECT_CALL(*m_presentationObserver, onNavigateBack(_))
        .Times(Exactly(1))
        .WillRepeatedly(
            Invoke([](const presentationOrchestratorInterfaces::PresentationRequestToken id) { return false; }));

    // No state changes are expected when navigateBack() is called on on presentation2
    EXPECT_CALL(*m_presentationObserver, onPresentationStateChanged(_, _)).Times(Exactly(0));

    // WINDOW_ID_2 should not be released when navigateBack() is called on presentation2
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _)).Times(Exactly(0));

    // navigateBack() on presentation2
    m_presentationOrchestratorClient->navigateBack();
    m_executor->waitForSubmittedTasks();
    // presentation2 should still be in the FOREGROUND
    ASSERT_EQ(presentation2->getState(), PresentationState::FOREGROUND);

    // Both windows are released during teardown
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([](const std::string& clientId, const std::string& windowId) {
            ASSERT_EQ(CLIENT_ID, clientId);
            ASSERT_EQ(WINDOW_ID_2, windowId);
        }))
        .WillOnce(Invoke([](const std::string& clientId, const std::string& windowId) {
            ASSERT_EQ(CLIENT_ID, clientId);
            ASSERT_EQ(WINDOW_ID_1, windowId);
        }));

    // Only state changes that occur should be that both presentations are moved to NONE state during teardown
    EXPECT_CALL(*m_presentationObserver, onPresentationStateChanged(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([presentation2RequestToken](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            ASSERT_EQ(id, presentation2RequestToken);
            ASSERT_EQ(newState, PresentationState::NONE);
        }))
        .WillOnce(Invoke([presentation1RequestToken](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            ASSERT_EQ(id, presentation1RequestToken);
            ASSERT_EQ(newState, PresentationState::NONE);
        }));
}

/**
 * Tests back navigation on a single window having a presentation with PERMANENT lifespan
 * Window shouldn't be released in this case
 */
TEST_F(PresentationOrchestratorClientTest, testNavigateBackPermanent) {
    std::vector<std::string> supportedInterfaces = {INTERFACE_1};
    auto windowInstance = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance);

    auto expectedOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::PERMANENT, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<void> onPresentationAvailableCalled;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([&onPresentationAvailableCalled](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            ASSERT_EQ(presentation->getState(), PresentationState::FOREGROUND);
            onPresentationAvailableCalled.setValue();
        }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, expectedOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailableCalled.waitFor(SHORT_DELAY));

    EXPECT_CALL(*m_stateTracker, getFocusedWindowId()).WillRepeatedly(Invoke([]() { return WINDOW_ID_1; }));
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _)).Times(0);
    EXPECT_CALL(*m_presentationObserver, onNavigateBack(_))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const presentationOrchestratorInterfaces::PresentationRequestToken id) { return false; }));

    m_presentationOrchestratorClient->navigateBack();
    m_executor->waitForSubmittedTasks();

    // release window should be invoked in tearDown
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::string& clientId, const std::string& windowId) {
            ASSERT_EQ(CLIENT_ID, clientId);
            ASSERT_EQ(WINDOW_ID_1, windowId);
        }));
}

/**
 * Tests back navigation on a window with two presentations
 * Top PERMANENT presentation should not be dismissed on navigate back
 */
TEST_F(PresentationOrchestratorClientTest, testNavigateBackPermanentOnLong) {
    std::vector<std::string> supportedInterfaces = {INTERFACE_1};
    auto windowInstance = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance);

    auto longPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);
    auto permanentPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::PERMANENT, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<void> onLongPresentationAvailableCalled;
    avsCommon::utils::PromiseFuturePair<void> onPermanentPresentationAvailableCalled;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&onLongPresentationAvailableCalled](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            ASSERT_EQ(presentation->getState(), PresentationState::FOREGROUND);
            onLongPresentationAvailableCalled.setValue();
        }))
        .WillOnce(Invoke([&onPermanentPresentationAvailableCalled](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            ASSERT_EQ(presentation->getState(), PresentationState::FOREGROUND);
            onPermanentPresentationAvailableCalled.setValue();
        }));

    auto longPresentationRequestToken =
        m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, longPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onLongPresentationAvailableCalled.waitFor(SHORT_DELAY));

    auto permanentPresentationRequestToken = m_presentationOrchestratorClient->requestWindow(
        WINDOW_ID_1, permanentPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPermanentPresentationAvailableCalled.waitFor(SHORT_DELAY));

    EXPECT_CALL(*m_stateTracker, getFocusedWindowId()).WillRepeatedly(Invoke([]() { return WINDOW_ID_1; }));
    EXPECT_CALL(*m_presentationObserver, onPresentationStateChanged(_, _)).Times(0);
    EXPECT_CALL(*m_presentationObserver, onNavigateBack(_))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const presentationOrchestratorInterfaces::PresentationRequestToken id) { return false; }));

    // should not invoke onPresentationStateChanged of presentation observer
    m_presentationOrchestratorClient->navigateBack();
    m_executor->waitForSubmittedTasks();

    // onPresentationStateChanged should be invoked in tearDown
    EXPECT_CALL(*m_presentationObserver, onPresentationStateChanged(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([permanentPresentationRequestToken](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            ASSERT_EQ(id, permanentPresentationRequestToken);
            ASSERT_EQ(newState, PresentationState::NONE);
        }))
        .WillOnce(Invoke([longPresentationRequestToken](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            ASSERT_EQ(id, longPresentationRequestToken);
            ASSERT_EQ(newState, PresentationState::NONE);
        }));
}

/**
 * Tests back navigation on a single window having a presentation with SHORT lifespan that handles back navigation
 * Window shouldn't be released in this case
 */
TEST_F(PresentationOrchestratorClientTest, testNavigateBackShortHandlesBack) {
    std::vector<std::string> supportedInterfaces = {INTERFACE_1};
    auto windowInstance = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance);

    auto expectedOptions = generatePresentationOptions(TIMEOUT, PresentationLifespan::SHORT, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<void> onPresentationAvailableCalled;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([&onPresentationAvailableCalled](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            ASSERT_EQ(presentation->getState(), PresentationState::FOREGROUND);
            onPresentationAvailableCalled.setValue();
        }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, expectedOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailableCalled.waitFor(SHORT_DELAY));

    EXPECT_CALL(*m_stateTracker, getFocusedWindowId()).WillRepeatedly(Invoke([]() { return WINDOW_ID_1; }));
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _)).Times(0);
    EXPECT_CALL(*m_presentationObserver, onNavigateBack(_))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const presentationOrchestratorInterfaces::PresentationRequestToken id) { return true; }));

    m_presentationOrchestratorClient->navigateBack();
    m_executor->waitForSubmittedTasks();

    // release window should be invoked in tearDown
    EXPECT_CALL(*m_stateTracker, releaseWindow(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::string& clientId, const std::string& windowId) {
            ASSERT_EQ(CLIENT_ID, clientId);
            ASSERT_EQ(WINDOW_ID_1, windowId);
        }));
}

/**
 * Test LONG presentation being foregrounded after TRANSIENT presentation in same window
 */
TEST_F(PresentationOrchestratorClientTest, testRequestWindowTransientFollowedByLongPresentation) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);

    auto transientPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::TRANSIENT, INTERFACE_1, METADATA_1);
    auto longPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onTransientPresentationAvailable;
    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onLongPresentationAvailable;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&onTransientPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onTransientPresentationAvailable.setValue(presentation);
        }))
        .WillOnce(Invoke([&onLongPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onLongPresentationAvailable.setValue(presentation);
        }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, transientPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onTransientPresentationAvailable.waitFor(SHORT_DELAY));

    auto transientPresentation = onTransientPresentationAvailable.getValue();
    ASSERT_EQ(transientPresentation->getState(), PresentationState::FOREGROUND);

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, longPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onLongPresentationAvailable.waitFor(SHORT_DELAY));

    auto longPresentation = onLongPresentationAvailable.getValue();
    ASSERT_EQ(transientPresentation->getState(), PresentationState::NONE);
    ASSERT_EQ(longPresentation->getState(), PresentationState::FOREGROUND);
}

/**
 * Test LONG presentation being foregrounded after SHORT presentation in same window
 */
TEST_F(PresentationOrchestratorClientTest, testRequestWindowShortFollowedByLongPresentation) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);

    auto shortPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::TRANSIENT, INTERFACE_1, METADATA_1);
    auto longPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onShortPresentationAvailable;
    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onLongPresentationAvailable;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&onShortPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onShortPresentationAvailable.setValue(presentation);
        }))
        .WillOnce(Invoke([&onLongPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onLongPresentationAvailable.setValue(presentation);
        }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, shortPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onShortPresentationAvailable.waitFor(SHORT_DELAY));

    auto shortPresentation = onShortPresentationAvailable.getValue();
    ASSERT_EQ(shortPresentation->getState(), PresentationState::FOREGROUND);

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, longPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onLongPresentationAvailable.waitFor(SHORT_DELAY));

    auto longPresentation = onLongPresentationAvailable.getValue();
    ASSERT_EQ(shortPresentation->getState(), PresentationState::NONE);
    ASSERT_EQ(longPresentation->getState(), PresentationState::FOREGROUND);
}

/**
 * Test TRANSIENT presentation being foregrounded after SHORT presentation in same window
 */
TEST_F(PresentationOrchestratorClientTest, testRequestWindowShortFollowedByTransientPresentation) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);

    auto shortPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::SHORT, INTERFACE_1, METADATA_1);
    auto transientPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::TRANSIENT, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onShortPresentationAvailable;
    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onTransientPresentationAvailable;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&onShortPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onShortPresentationAvailable.setValue(presentation);
        }))
        .WillOnce(Invoke([&onTransientPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onTransientPresentationAvailable.setValue(presentation);
        }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, shortPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onShortPresentationAvailable.waitFor(SHORT_DELAY));

    auto shortPresentation = onShortPresentationAvailable.getValue();
    ASSERT_EQ(shortPresentation->getState(), PresentationState::FOREGROUND);

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, transientPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onTransientPresentationAvailable.waitFor(SHORT_DELAY));

    auto transientPresentation = onTransientPresentationAvailable.getValue();
    ASSERT_EQ(shortPresentation->getState(), PresentationState::BACKGROUND);
    ASSERT_EQ(transientPresentation->getState(), PresentationState::FOREGROUND);
}

/**
 * Test TRANSIENT presentation being foregrounded after LONG presentation in same window
 */
TEST_F(PresentationOrchestratorClientTest, testRequestWindowLongFollowedByTransientPresentation) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);

    auto longPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::LONG, INTERFACE_1, METADATA_1);
    auto transientPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::TRANSIENT, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onLongPresentationAvailable;
    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onTransientPresentationAvailable;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&onLongPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onLongPresentationAvailable.setValue(presentation);
        }))
        .WillOnce(Invoke([&onTransientPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onTransientPresentationAvailable.setValue(presentation);
        }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, longPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onLongPresentationAvailable.waitFor(SHORT_DELAY));

    auto longPresentation = onLongPresentationAvailable.getValue();
    ASSERT_EQ(longPresentation->getState(), PresentationState::FOREGROUND);

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, transientPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onTransientPresentationAvailable.waitFor(SHORT_DELAY));

    auto transientPresentation = onTransientPresentationAvailable.getValue();
    ASSERT_EQ(longPresentation->getState(), PresentationState::BACKGROUND);
    ASSERT_EQ(transientPresentation->getState(), PresentationState::FOREGROUND);
}

/**
 * Test PERMANENT presentation should be foregrounded after SHORT presentation is dismissed
 */
TEST_F(PresentationOrchestratorClientTest, testDismissShortOnPermanent) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);

    auto permanentPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::PERMANENT, INTERFACE_1, METADATA_1);
    auto shortPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::SHORT, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPermanentPresentationAvailable;
    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onShortPresentationAvailable;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&onPermanentPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            ASSERT_EQ(presentation->getState(), PresentationState::FOREGROUND);
            onPermanentPresentationAvailable.setValue(presentation);
        }))
        .WillOnce(Invoke([&onShortPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            ASSERT_EQ(presentation->getState(), PresentationState::FOREGROUND);
            onShortPresentationAvailable.setValue(presentation);
        }));

    auto permanentPresentationRequestToken = m_presentationOrchestratorClient->requestWindow(
        WINDOW_ID_1, permanentPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPermanentPresentationAvailable.waitFor(SHORT_DELAY));

    auto permanentPresentation = onPermanentPresentationAvailable.getValue();
    ASSERT_EQ(permanentPresentation->getState(), PresentationState::FOREGROUND);

    auto shortPresentationRequestToken =
        m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, shortPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onShortPresentationAvailable.waitFor(SHORT_DELAY));

    auto shortPresentation = onShortPresentationAvailable.getValue();
    ASSERT_EQ(permanentPresentation->getState(), PresentationState::BACKGROUND);
    ASSERT_EQ(shortPresentation->getState(), PresentationState::FOREGROUND);

    EXPECT_CALL(*m_presentationObserver, onPresentationStateChanged(_, _))
        .Times(Exactly(3))
        .WillOnce(Invoke([shortPresentationRequestToken](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            ASSERT_EQ(id, shortPresentationRequestToken);
            ASSERT_EQ(newState, PresentationState::NONE);
        }))
        .WillOnce(Invoke([permanentPresentationRequestToken](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            ASSERT_EQ(id, permanentPresentationRequestToken);
            ASSERT_EQ(newState, PresentationState::FOREGROUND);
        }))
        .WillOnce(Invoke([permanentPresentationRequestToken](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            // onPresentationStateChanged for permanent presentation should be invoked in tearDown
            ASSERT_EQ(id, permanentPresentationRequestToken);
            ASSERT_EQ(newState, PresentationState::NONE);
        }));

    // Dismiss SHORT presentation should foreground PERMANENT presentation
    shortPresentation->dismiss();
}

/**
 * Test PERMANENT presentation should be foregrounded after TRANSIENT presentation is dismissed
 */
TEST_F(PresentationOrchestratorClientTest, testDismissTransientOnPermanent) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);

    auto permanentPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::PERMANENT, INTERFACE_1, METADATA_1);
    auto transientPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::TRANSIENT, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPermanentPresentationAvailable;
    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onTransientPresentationAvailable;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&onPermanentPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            ASSERT_EQ(presentation->getState(), PresentationState::FOREGROUND);
            onPermanentPresentationAvailable.setValue(presentation);
        }))
        .WillOnce(Invoke([&onTransientPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            ASSERT_EQ(presentation->getState(), PresentationState::FOREGROUND);
            onTransientPresentationAvailable.setValue(presentation);
        }));

    auto permanentPresentationRequestToken = m_presentationOrchestratorClient->requestWindow(
        WINDOW_ID_1, permanentPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPermanentPresentationAvailable.waitFor(SHORT_DELAY));

    auto permanentPresentation = onPermanentPresentationAvailable.getValue();
    ASSERT_EQ(permanentPresentation->getState(), PresentationState::FOREGROUND);

    auto transientPresentationRequestToken = m_presentationOrchestratorClient->requestWindow(
        WINDOW_ID_1, transientPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onTransientPresentationAvailable.waitFor(SHORT_DELAY));

    auto transientPresentation = onTransientPresentationAvailable.getValue();
    ASSERT_EQ(permanentPresentation->getState(), PresentationState::BACKGROUND);
    ASSERT_EQ(transientPresentation->getState(), PresentationState::FOREGROUND);

    EXPECT_CALL(*m_presentationObserver, onPresentationStateChanged(_, _))
        .Times(Exactly(3))
        .WillOnce(Invoke([transientPresentationRequestToken](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            ASSERT_EQ(id, transientPresentationRequestToken);
            ASSERT_EQ(newState, PresentationState::NONE);
        }))
        .WillOnce(Invoke([permanentPresentationRequestToken](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            ASSERT_EQ(id, permanentPresentationRequestToken);
            ASSERT_EQ(newState, PresentationState::FOREGROUND);
        }))
        .WillOnce(Invoke([permanentPresentationRequestToken](
                             const presentationOrchestratorInterfaces::PresentationRequestToken id,
                             const presentationOrchestratorInterfaces::PresentationState newState) {
            // onPresentationStateChanged for permanent presentation should be invoked in tearDown
            ASSERT_EQ(id, permanentPresentationRequestToken);
            ASSERT_EQ(newState, PresentationState::NONE);
        }));

    // Dismiss TRANSIENT presentation should foreground PERMANENT presentation
    transientPresentation->dismiss();
}

/**
 * Test defaulting presentation timeouts in single window
 */
TEST_F(PresentationOrchestratorClientTest, testDefaultingTimeout) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);

    auto longPresentationOptions = generatePresentationOptions(
        PresentationInterface::getTimeoutDefault(), PresentationLifespan::LONG, INTERFACE_1, METADATA_1);
    auto shortPresentationOptions = generatePresentationOptions(
        PresentationInterface::getTimeoutDefault(), PresentationLifespan::SHORT, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onLongPresentationAvailable;
    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onShortPresentationAvailable;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(2))
        .WillOnce(Invoke([&onLongPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onLongPresentationAvailable.setValue(presentation);
        }))
        .WillOnce(Invoke([&onShortPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onShortPresentationAvailable.setValue(presentation);
        }));

    // Timeout should be disabled on LONG presentation by default
    EXPECT_CALL(*m_visualTimeoutManager, requestTimeout(_, _)).Times(Exactly(0));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, longPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onLongPresentationAvailable.waitFor(SHORT_DELAY));

    auto longPresentation = onLongPresentationAvailable.getValue();
    ASSERT_EQ(longPresentation->getState(), PresentationState::FOREGROUND);

    EXPECT_CALL(*m_visualTimeoutManager, requestTimeout(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::chrono::milliseconds& delay,
                            const VisualTimeoutManagerInterface::VisualTimeoutCallback& timeoutCallback) {
            EXPECT_EQ(DEFAULT_TIMEOUT_SHORT_PRESENTATION_MS, delay);
            // random timeout id; not validated
            return 1;
        }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, shortPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onShortPresentationAvailable.waitFor(SHORT_DELAY));

    auto shortPresentation = onShortPresentationAvailable.getValue();
    ASSERT_EQ(longPresentation->getState(), PresentationState::BACKGROUND);
    ASSERT_EQ(shortPresentation->getState(), PresentationState::FOREGROUND);
}

/**
 * Test presentation initialization with timeout disabled
 */
TEST_F(PresentationOrchestratorClientTest, testPresentationDisabledTimeout) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);

    auto shortPresentationOptions = generatePresentationOptions(
        PresentationInterface::getTimeoutDisabled(), PresentationLifespan::TRANSIENT, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onShortPresentationAvailable;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([&onShortPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onShortPresentationAvailable.setValue(presentation);
        }));

    EXPECT_CALL(*m_visualTimeoutManager, requestTimeout(_, _)).Times(Exactly(0));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, shortPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onShortPresentationAvailable.waitFor(SHORT_DELAY));

    auto shortPresentation = onShortPresentationAvailable.getValue();
    ASSERT_EQ(shortPresentation->getState(), PresentationState::FOREGROUND);
}

/**
 * Test disabling timeout using Presentation::setTimeout
 */
TEST_F(PresentationOrchestratorClientTest, testDisablingTimeoutUsingSetTimeout) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);

    auto shortPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::SHORT, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onShortPresentationAvailable;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([&onShortPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onShortPresentationAvailable.setValue(presentation);
        }));

    // random timeout id
    VisualTimeoutManagerInterface::VisualTimeoutId expectedTimeoutId = 1;
    EXPECT_CALL(*m_visualTimeoutManager, requestTimeout(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([&expectedTimeoutId](
                             const std::chrono::milliseconds& delay,
                             const VisualTimeoutManagerInterface::VisualTimeoutCallback& timeoutCallback) {
            EXPECT_EQ(TIMEOUT, delay);
            return expectedTimeoutId;
        }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, shortPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onShortPresentationAvailable.waitFor(SHORT_DELAY));

    auto shortPresentation = onShortPresentationAvailable.getValue();
    ASSERT_EQ(shortPresentation->getState(), PresentationState::FOREGROUND);

    EXPECT_CALL(*m_visualTimeoutManager, stopTimeout(_))
        .Times(AtLeast(1))
        .WillOnce(Invoke([&expectedTimeoutId](const VisualTimeoutManagerInterface::VisualTimeoutId& timeoutId) {
            EXPECT_TRUE(expectedTimeoutId == timeoutId);
            return true;
        }));

    shortPresentation->setTimeout(PresentationInterface::getTimeoutDisabled());
    m_executor->waitForSubmittedTasks();
}

/**
 * Test that custom timeout is not used for presentation with lifespan PERMANENT.
 * VisualTimeoutManager requestTimeout should not be called.
 */
TEST_F(PresentationOrchestratorClientTest, testPermanentPresentationCustomTimeout) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);

    // provide a custom timeout during initialization
    auto permanentPresentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::PERMANENT, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPermanentPresentationAvailable;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([&onPermanentPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onPermanentPresentationAvailable.setValue(presentation);
        }));

    EXPECT_CALL(*m_visualTimeoutManager, requestTimeout(_, _)).Times(Exactly(0));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, permanentPresentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPermanentPresentationAvailable.waitFor(SHORT_DELAY));

    auto permanentPresentation = onPermanentPresentationAvailable.getValue();
    ASSERT_EQ(permanentPresentation->getState(), PresentationState::FOREGROUND);

    // try setting a custom timeout again
    permanentPresentation->setTimeout(TIMEOUT);
    permanentPresentation->startTimeout();
    m_executor->waitForSubmittedTasks();
}

/**
 * Test that custom timeout is not used for presentation with lifespan PERMANENT.
 * However, once the lifespan is changed to SHORT, it uses the custom timeout instead of default.
 */
TEST_F(PresentationOrchestratorClientTest, testPermanentPresentationUpdateToShortCustomTimeout) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance1 = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance1);

    // provide a custom timeout during initialization
    auto presentationOptions =
        generatePresentationOptions(TIMEOUT, PresentationLifespan::PERMANENT, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPresentationAvailable;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([&onPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onPresentationAvailable.setValue(presentation);
        }));

    EXPECT_CALL(*m_visualTimeoutManager, requestTimeout(_, _)).Times(Exactly(0));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, presentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailable.waitFor(SHORT_DELAY));

    auto presentation = onPresentationAvailable.getValue();
    ASSERT_EQ(presentation->getState(), PresentationState::FOREGROUND);

    EXPECT_CALL(*m_visualTimeoutManager, requestTimeout(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::chrono::milliseconds& delay,
                            const VisualTimeoutManagerInterface::VisualTimeoutCallback& timeoutCallback) {
            EXPECT_EQ(TIMEOUT, delay);
            // random timeout id; not validated
            return 1;
        }));

    // change the lifespan so custom timeout is used when timeout is restarted
    presentation->setLifespan(PresentationLifespan::SHORT);
    presentation->startTimeout();
    m_executor->waitForSubmittedTasks();
}

/**
 * Test timeout is stopped if the updated lifespan has a timeout disabled by default.
 * Presentations with lifespan LONG have timeout disabled by default. This test changes lifespan from SHORT to LONG.
 */
TEST_F(PresentationOrchestratorClientTest, testShortLifespanToLongLifespanChangeDisablesTimeout) {
    std::vector<std::string> supportedInterfaces1 = {INTERFACE_1};
    auto windowInstance = generateWindowInstance(WINDOW_ID_1, 1, supportedInterfaces1);
    m_presentationOrchestratorClient->onWindowAdded(windowInstance);

    // use default timeout for presentation
    auto presentationOptions = generatePresentationOptions(
        PresentationInterface::getTimeoutDefault(), PresentationLifespan::SHORT, INTERFACE_1, METADATA_1);

    avsCommon::utils::PromiseFuturePair<std::shared_ptr<PresentationInterface>> onPresentationAvailable;
    EXPECT_CALL(*m_presentationObserver, onPresentationAvailable(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([&onPresentationAvailable](
                             const PresentationRequestToken& requestToken,
                             const std::shared_ptr<PresentationInterface>& presentation) {
            onPresentationAvailable.setValue(presentation);
        }));

    // random timeout id
    VisualTimeoutManagerInterface::VisualTimeoutId expectedTimeoutId = 3;
    // timeout should be requested for presentation with lifespan SHORT
    EXPECT_CALL(*m_visualTimeoutManager, requestTimeout(_, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([expectedTimeoutId](
                             const std::chrono::milliseconds& delay,
                             const VisualTimeoutManagerInterface::VisualTimeoutCallback& timeoutCallback) {
            return expectedTimeoutId;
        }));

    m_presentationOrchestratorClient->requestWindow(WINDOW_ID_1, presentationOptions, m_presentationObserver);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(onPresentationAvailable.waitFor(SHORT_DELAY));

    auto presentation = onPresentationAvailable.getValue();
    ASSERT_EQ(presentation->getState(), PresentationState::FOREGROUND);

    // active timeout should be stopped once lifespan changes to LONG
    EXPECT_CALL(*m_visualTimeoutManager, stopTimeout(_))
        // first call on lifespan change; second call during teardown
        .Times(Exactly(2))
        .WillRepeatedly(Invoke([expectedTimeoutId](const VisualTimeoutManagerInterface::VisualTimeoutId& timeoutId) {
            EXPECT_TRUE(expectedTimeoutId == timeoutId);
            return true;
        }));

    // change presentation lifespan to LONG
    presentation->setLifespan(PresentationLifespan::LONG);
    m_executor->waitForSubmittedTasks();
}

}  // namespace test
}  // namespace presentationOrchestratorClient
}  // namespace alexaClientSDK