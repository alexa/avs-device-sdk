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

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockPlaybackRouter.h>

#include "acsdkBluetooth/BluetoothMediaInputTransformer.h"

namespace alexaClientSDK {
namespace acsdkBluetooth {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::sdkInterfaces::bluetooth::services;
using namespace avsCommon::utils;
using namespace avsCommon::utils::bluetooth;
class MockPlaybackRouter : public PlaybackRouterInterface {
public:
    MOCK_METHOD1(buttonPressed, void(PlaybackButton));
    MOCK_METHOD2(togglePressed, void(PlaybackToggle, bool));
    MOCK_METHOD1(setHandler, void(std::shared_ptr<PlaybackHandlerInterface>));
    MOCK_METHOD2(
        setHandler,
        void(std::shared_ptr<PlaybackHandlerInterface>, std::shared_ptr<LocalPlaybackHandlerInterface>));
    MOCK_METHOD0(switchToDefaultHandler, void());
};

class BluetoothMediaInputTransformerTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp();

protected:
    /// The eventbus in which the @c BluetoothMediaInputTransformer will received Media events on.
    std::shared_ptr<BluetoothEventBus> m_eventBus;

    /// A mock @PlaybackRouterInterface object.
    std::shared_ptr<MockPlaybackRouter> m_mockRouter;

    /// The @c BluetoothMediaInputTransformer instance under test.
    std::shared_ptr<BluetoothMediaInputTransformer> m_mediaInputTransformer;
};

void BluetoothMediaInputTransformerTest::SetUp() {
    m_eventBus = std::make_shared<avsCommon::utils::bluetooth::BluetoothEventBus>();
    m_mockRouter = std::make_shared<MockPlaybackRouter>();
    m_mediaInputTransformer = BluetoothMediaInputTransformer::create(m_eventBus, m_mockRouter);
}

/// Test that create() returns a nullptr if called with invalid arguments.
TEST_F(BluetoothMediaInputTransformerTest, test_createWithNullParams) {
    ASSERT_THAT(BluetoothMediaInputTransformer::create(m_eventBus, nullptr), IsNull());
    ASSERT_THAT(BluetoothMediaInputTransformer::create(nullptr, m_mockRouter), IsNull());
}

/// Test that a Play AVRCP command is transformed to playButtonPressed().
TEST_F(BluetoothMediaInputTransformerTest, test_handlePlayCommand) {
    EXPECT_CALL(*m_mockRouter, buttonPressed(PlaybackButton::PLAY)).Times(1);

    MediaCommandReceivedEvent event(MediaCommand::PLAY);
    m_eventBus->sendEvent(event);
}

/// Test that a Pause AVRCP command is transformed to pauseButtonPressed().
TEST_F(BluetoothMediaInputTransformerTest, test_handlePauseCommand) {
    EXPECT_CALL(*m_mockRouter, buttonPressed(PlaybackButton::PAUSE)).Times(1);

    MediaCommandReceivedEvent event(MediaCommand::PAUSE);
    m_eventBus->sendEvent(event);
}

/// Test that a Next AVRCP command is transformed to nextButtonPressed().
TEST_F(BluetoothMediaInputTransformerTest, test_handleNextCommand) {
    EXPECT_CALL(*m_mockRouter, buttonPressed(PlaybackButton::NEXT)).Times(1);

    MediaCommandReceivedEvent event(MediaCommand::NEXT);
    m_eventBus->sendEvent(event);
}

/// Test that a Previous AVRCP command is transformed to previousButtonPressed().
TEST_F(BluetoothMediaInputTransformerTest, test_handlePreviousCommand) {
    EXPECT_CALL(*m_mockRouter, buttonPressed(PlaybackButton::PREVIOUS)).Times(1);

    MediaCommandReceivedEvent event(MediaCommand::PREVIOUS);
    m_eventBus->sendEvent(event);
}

/// Test that a Play/Pause Media command is transformed to playButtonPressed().
TEST_F(BluetoothMediaInputTransformerTest, handlePlayPauseCommand) {
    EXPECT_CALL(*m_mockRouter, buttonPressed(PlaybackButton::PLAY)).Times(1);

    MediaCommandReceivedEvent event(MediaCommand::PLAY_PAUSE);
    m_eventBus->sendEvent(event);
}

/// Test that an unrelated event does not trigger any calls to the PlaybackRouter.
TEST_F(BluetoothMediaInputTransformerTest, test_unrelatedEvent) {
    auto strictPlaybackRouter = std::make_shared<StrictMock<MockPlaybackRouter>>();
    auto mediaInputTransformer = BluetoothMediaInputTransformer::create(m_eventBus, strictPlaybackRouter);

    DeviceDiscoveredEvent event(nullptr);
    m_eventBus->sendEvent(event);
}

}  // namespace test
}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK
