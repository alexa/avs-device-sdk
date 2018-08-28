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

#include <AVSCommon/SDKInterfaces/MockPlaybackHandler.h>
#include "PlaybackController/PlaybackRouter.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace playbackController {
namespace test {

using namespace testing;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;

class PlaybackRouterTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

protected:
    std::shared_ptr<PlaybackRouter> m_playbackRouter;
    std::shared_ptr<StrictMock<MockPlaybackHandler>> m_defaultPlaybackHandler;
    std::shared_ptr<StrictMock<MockPlaybackHandler>> m_secondPlaybackHandler;
};

/// Set up a test harness for running a test.
void PlaybackRouterTest::SetUp() {
    m_defaultPlaybackHandler = std::make_shared<StrictMock<MockPlaybackHandler>>();
    m_playbackRouter = PlaybackRouter::create(m_defaultPlaybackHandler);
    m_secondPlaybackHandler = std::make_shared<StrictMock<MockPlaybackHandler>>();
}

void PlaybackRouterTest::TearDown() {
    m_playbackRouter->shutdown();
}

/**
 * Test default handler is called.
 */
TEST_F(PlaybackRouterTest, defaultHandler) {
    EXPECT_CALL(*m_defaultPlaybackHandler, onButtonPressed(PlaybackButton::PLAY));
    m_playbackRouter->buttonPressed(PlaybackButton::PLAY);

    EXPECT_CALL(*m_defaultPlaybackHandler, onButtonPressed(PlaybackButton::PAUSE));
    m_playbackRouter->buttonPressed(PlaybackButton::PAUSE);

    EXPECT_CALL(*m_defaultPlaybackHandler, onButtonPressed(PlaybackButton::NEXT));
    m_playbackRouter->buttonPressed(PlaybackButton::NEXT);

    EXPECT_CALL(*m_defaultPlaybackHandler, onButtonPressed(PlaybackButton::PREVIOUS));
    m_playbackRouter->buttonPressed(PlaybackButton::PREVIOUS);

    EXPECT_CALL(*m_defaultPlaybackHandler, onButtonPressed(PlaybackButton::SKIP_FORWARD));
    m_playbackRouter->buttonPressed(PlaybackButton::SKIP_FORWARD);

    EXPECT_CALL(*m_defaultPlaybackHandler, onButtonPressed(PlaybackButton::SKIP_BACKWARD));
    m_playbackRouter->buttonPressed(PlaybackButton::SKIP_BACKWARD);

    EXPECT_CALL(*m_defaultPlaybackHandler, onTogglePressed(PlaybackToggle::SHUFFLE, true));
    m_playbackRouter->togglePressed(PlaybackToggle::SHUFFLE, true);

    EXPECT_CALL(*m_defaultPlaybackHandler, onTogglePressed(PlaybackToggle::LOOP, true));
    m_playbackRouter->togglePressed(PlaybackToggle::LOOP, true);

    EXPECT_CALL(*m_defaultPlaybackHandler, onTogglePressed(PlaybackToggle::REPEAT, true));
    m_playbackRouter->togglePressed(PlaybackToggle::REPEAT, true);

    EXPECT_CALL(*m_defaultPlaybackHandler, onTogglePressed(PlaybackToggle::THUMBS_UP, true));
    m_playbackRouter->togglePressed(PlaybackToggle::THUMBS_UP, true);

    EXPECT_CALL(*m_defaultPlaybackHandler, onTogglePressed(PlaybackToggle::THUMBS_DOWN, true));
    m_playbackRouter->togglePressed(PlaybackToggle::THUMBS_DOWN, true);
}

/**
 * Test 2nd handler is called after registration.
 */
TEST_F(PlaybackRouterTest, secondHandler) {
    m_playbackRouter->setHandler(m_defaultPlaybackHandler);
    EXPECT_CALL(*m_defaultPlaybackHandler, onButtonPressed(PlaybackButton::PLAY));
    m_playbackRouter->buttonPressed(PlaybackButton::PLAY);

    EXPECT_CALL(*m_defaultPlaybackHandler, onTogglePressed(PlaybackToggle::SHUFFLE, true));
    m_playbackRouter->togglePressed(PlaybackToggle::SHUFFLE, true);

    m_playbackRouter->setHandler(m_secondPlaybackHandler);

    EXPECT_CALL(*m_secondPlaybackHandler, onButtonPressed(PlaybackButton::PLAY));
    m_playbackRouter->buttonPressed(PlaybackButton::PLAY);

    EXPECT_CALL(*m_secondPlaybackHandler, onButtonPressed(PlaybackButton::PAUSE));
    m_playbackRouter->buttonPressed(PlaybackButton::PAUSE);

    EXPECT_CALL(*m_secondPlaybackHandler, onButtonPressed(PlaybackButton::NEXT));
    m_playbackRouter->buttonPressed(PlaybackButton::NEXT);

    EXPECT_CALL(*m_secondPlaybackHandler, onButtonPressed(PlaybackButton::PREVIOUS));
    m_playbackRouter->buttonPressed(PlaybackButton::PREVIOUS);

    EXPECT_CALL(*m_secondPlaybackHandler, onButtonPressed(PlaybackButton::SKIP_FORWARD));
    m_playbackRouter->buttonPressed(PlaybackButton::SKIP_FORWARD);

    EXPECT_CALL(*m_secondPlaybackHandler, onButtonPressed(PlaybackButton::SKIP_BACKWARD));
    m_playbackRouter->buttonPressed(PlaybackButton::SKIP_BACKWARD);

    EXPECT_CALL(*m_secondPlaybackHandler, onTogglePressed(PlaybackToggle::SHUFFLE, true));
    m_playbackRouter->togglePressed(PlaybackToggle::SHUFFLE, true);

    EXPECT_CALL(*m_secondPlaybackHandler, onTogglePressed(PlaybackToggle::LOOP, true));
    m_playbackRouter->togglePressed(PlaybackToggle::LOOP, true);

    EXPECT_CALL(*m_secondPlaybackHandler, onTogglePressed(PlaybackToggle::REPEAT, true));
    m_playbackRouter->togglePressed(PlaybackToggle::REPEAT, true);

    EXPECT_CALL(*m_secondPlaybackHandler, onTogglePressed(PlaybackToggle::THUMBS_UP, true));
    m_playbackRouter->togglePressed(PlaybackToggle::THUMBS_UP, true);

    EXPECT_CALL(*m_secondPlaybackHandler, onTogglePressed(PlaybackToggle::THUMBS_DOWN, true));
    m_playbackRouter->togglePressed(PlaybackToggle::THUMBS_DOWN, true);
}

/**
 * Test default handler is called again after @c switchToDefaultHandler has been called.
 */
TEST_F(PlaybackRouterTest, switchToDefaultHandler) {
    EXPECT_CALL(*m_defaultPlaybackHandler, onButtonPressed(PlaybackButton::PLAY));
    m_playbackRouter->buttonPressed(PlaybackButton::PLAY);

    EXPECT_CALL(*m_defaultPlaybackHandler, onTogglePressed(PlaybackToggle::SHUFFLE, true));
    m_playbackRouter->togglePressed(PlaybackToggle::SHUFFLE, true);

    m_playbackRouter->setHandler(m_secondPlaybackHandler);
    EXPECT_CALL(*m_secondPlaybackHandler, onButtonPressed(PlaybackButton::PLAY));
    m_playbackRouter->buttonPressed(PlaybackButton::PLAY);

    EXPECT_CALL(*m_secondPlaybackHandler, onTogglePressed(PlaybackToggle::SHUFFLE, true));
    m_playbackRouter->togglePressed(PlaybackToggle::SHUFFLE, true);

    m_playbackRouter->switchToDefaultHandler();

    EXPECT_CALL(*m_defaultPlaybackHandler, onButtonPressed(PlaybackButton::PLAY));
    m_playbackRouter->buttonPressed(PlaybackButton::PLAY);

    EXPECT_CALL(*m_defaultPlaybackHandler, onButtonPressed(PlaybackButton::PAUSE));
    m_playbackRouter->buttonPressed(PlaybackButton::PAUSE);

    EXPECT_CALL(*m_defaultPlaybackHandler, onButtonPressed(PlaybackButton::NEXT));
    m_playbackRouter->buttonPressed(PlaybackButton::NEXT);

    EXPECT_CALL(*m_defaultPlaybackHandler, onButtonPressed(PlaybackButton::PREVIOUS));
    m_playbackRouter->buttonPressed(PlaybackButton::PREVIOUS);

    EXPECT_CALL(*m_defaultPlaybackHandler, onButtonPressed(PlaybackButton::SKIP_FORWARD));
    m_playbackRouter->buttonPressed(PlaybackButton::SKIP_FORWARD);

    EXPECT_CALL(*m_defaultPlaybackHandler, onButtonPressed(PlaybackButton::SKIP_BACKWARD));
    m_playbackRouter->buttonPressed(PlaybackButton::SKIP_BACKWARD);

    EXPECT_CALL(*m_defaultPlaybackHandler, onTogglePressed(PlaybackToggle::SHUFFLE, true));
    m_playbackRouter->togglePressed(PlaybackToggle::SHUFFLE, true);

    EXPECT_CALL(*m_defaultPlaybackHandler, onTogglePressed(PlaybackToggle::LOOP, true));
    m_playbackRouter->togglePressed(PlaybackToggle::LOOP, true);

    EXPECT_CALL(*m_defaultPlaybackHandler, onTogglePressed(PlaybackToggle::REPEAT, true));
    m_playbackRouter->togglePressed(PlaybackToggle::REPEAT, true);

    EXPECT_CALL(*m_defaultPlaybackHandler, onTogglePressed(PlaybackToggle::THUMBS_UP, true));
    m_playbackRouter->togglePressed(PlaybackToggle::THUMBS_UP, true);

    EXPECT_CALL(*m_defaultPlaybackHandler, onTogglePressed(PlaybackToggle::THUMBS_DOWN, true));
    m_playbackRouter->togglePressed(PlaybackToggle::THUMBS_DOWN, true);
}

}  // namespace test
}  // namespace playbackController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
