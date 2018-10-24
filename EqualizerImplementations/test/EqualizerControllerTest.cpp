/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <functional>
#include <memory>

#include <AVSCommon/SDKInterfaces/Audio/EqualizerConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/MockEqualizerControllerListenerInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/MockEqualizerInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/MockEqualizerModeControllerInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/MockEqualizerStorageInterface.h>

#include <AVSCommon/Utils/Error/SuccessResult.h>

#include "EqualizerImplementations/EqualizerController.h"
#include "EqualizerImplementations/InMemoryEqualizerConfiguration.h"

namespace alexaClientSDK {
namespace equalizer {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces::audio;
using namespace avsCommon::sdkInterfaces::audio::test;
using namespace avsCommon::utils::error;
using namespace equalizer;

/// Band level used as minimum value in tests
static constexpr int MIN_LEVEL = -10;
/// Band level used as maximum value in tests
static constexpr int MAX_LEVEL = 10;
/// Band level below the minimum allowed.
static constexpr int BELOW_MIN_LEVEL = -11;
/// Band level above the maximum allowed.
static constexpr int ABOVE_MAX_LEVEL = 11;
/// Band level used as a default
static constexpr int DEFAULT_LEVEL = 0;

/// A sample default band level for MIDRANGE band
static constexpr int DEFAULT_MIDRANGE = DEFAULT_LEVEL;
/// A sample band level for MIDRANGE band different from default
static constexpr int NON_DEFAULT_MIDRANGE = 4;
/// A sample default band level for TREBLE band
static constexpr int DEFAULT_TREBLE = 5;
/// A sample band level for TREBLE band different from default
static constexpr int NON_DEFAULT_TREBLE = -5;
/// A sample default mode
static constexpr EqualizerMode DEFAULT_MODE = EqualizerMode::NONE;

/// Number of times to perform an operation to make sure that it provides consistent results.
static constexpr int STABILITY_CHECK_ITERATIONS = 100;

/**
 * Test fixture for @c EqualizerController tests
 */
class EqualizerControllerTest : public ::testing::Test {
protected:
    void SetUp() override;

    /// Persistent storage mock.
    std::shared_ptr<MockEqualizerStorageInterface> m_storage;

    /// Equalizer configuration used in tests.
    std::shared_ptr<EqualizerConfigurationInterface> m_configuration;

    /// Equalizer mode controller mock.
    std::shared_ptr<MockEqualizerModeControllerInterface> m_modeController;
};

void EqualizerControllerTest::SetUp() {
    auto bands = std::set<EqualizerBand>({EqualizerBand::MIDRANGE, EqualizerBand::TREBLE});
    auto defaultState = EqualizerState{
        DEFAULT_MODE,
        EqualizerBandLevelMap({{EqualizerBand::MIDRANGE, DEFAULT_MIDRANGE}, {EqualizerBand::TREBLE, DEFAULT_TREBLE}})};
    auto modes = std::set<EqualizerMode>({EqualizerMode::NIGHT, EqualizerMode::TV});
    m_configuration = InMemoryEqualizerConfiguration::create(MIN_LEVEL, MAX_LEVEL, bands, modes, defaultState);
    m_storage = std::make_shared<NiceMock<MockEqualizerStorageInterface>>();
    m_modeController = std::make_shared<NiceMock<MockEqualizerModeControllerInterface>>();

    // Make sure storage will never contain a valid state so that we always use the hardcoded defaults.
    ON_CALL(*(m_storage.get()), loadState()).WillByDefault(Invoke([]() {
        return SuccessResult<EqualizerState>::failure();
    }));
    // Make sure setEqualizerMode() will return true (success), rather than mocked default - false.
    ON_CALL(*(m_modeController.get()), setEqualizerMode(_)).WillByDefault(Return(true));
}

// Test internal state right after controller creation
TEST_F(EqualizerControllerTest, providedEmptyConfig_shouldUseDefaults) {
    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    auto configuration = controller->getConfiguration();
    ASSERT_NE(configuration, nullptr);

    EXPECT_EQ(controller->getBandLevel(EqualizerBand::TREBLE).value(), DEFAULT_TREBLE);
    EXPECT_EQ(controller->getBandLevel(EqualizerBand::MIDRANGE).value(), DEFAULT_MIDRANGE);

    auto bandLevelsResult =
        controller->getBandLevels(std::set<EqualizerBand>({EqualizerBand::TREBLE, EqualizerBand::MIDRANGE}));
    EXPECT_TRUE(bandLevelsResult.isSucceeded());

    // Must return levels only for supported bands
    auto bandLevels = bandLevelsResult.value();
    EXPECT_TRUE(bandLevels.end() != bandLevels.find(EqualizerBand::MIDRANGE));
    EXPECT_TRUE(bandLevels.end() != bandLevels.find(EqualizerBand::TREBLE));
    EXPECT_FALSE(bandLevels.end() != bandLevels.find(EqualizerBand::BASS));
    // Check values
    EXPECT_EQ(bandLevels[EqualizerBand::TREBLE], DEFAULT_TREBLE);
    EXPECT_EQ(bandLevels[EqualizerBand::MIDRANGE], DEFAULT_MIDRANGE);

    // Check if current mode is NONE
    EXPECT_EQ(controller->getCurrentMode(), DEFAULT_MODE);
}

// Test simple changes
TEST_F(EqualizerControllerTest, changeBandLevels_shouldSucceed) {
    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    controller->setBandLevel(EqualizerBand::TREBLE, NON_DEFAULT_TREBLE);
    EXPECT_EQ(controller->getBandLevel(EqualizerBand::TREBLE).value(), NON_DEFAULT_TREBLE);

    controller->setBandLevel(EqualizerBand::MIDRANGE, NON_DEFAULT_MIDRANGE);
    EXPECT_EQ(controller->getBandLevel(EqualizerBand::MIDRANGE).value(), NON_DEFAULT_MIDRANGE);

    auto bandLevelsResult =
        controller->getBandLevels(std::set<EqualizerBand>({EqualizerBand::TREBLE, EqualizerBand::MIDRANGE}));
    EXPECT_TRUE(bandLevelsResult.isSucceeded());

    auto bandLevels = bandLevelsResult.value();
    EXPECT_EQ(bandLevels[EqualizerBand::TREBLE], NON_DEFAULT_TREBLE);
    EXPECT_EQ(bandLevels[EqualizerBand::MIDRANGE], NON_DEFAULT_MIDRANGE);

    controller->setBandLevel(EqualizerBand::MIDRANGE, DEFAULT_LEVEL);
    controller->adjustBandLevels(EqualizerBandLevelMap({{EqualizerBand::MIDRANGE, -1}}));
    EXPECT_EQ(controller->getBandLevel(EqualizerBand::MIDRANGE).value(), DEFAULT_LEVEL - 1);
}

// Test simple changes with invalid levels
TEST_F(EqualizerControllerTest, setInvalidBandLevels_shouldClampToSupportedRange) {
    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    controller->setBandLevel(EqualizerBand::TREBLE, BELOW_MIN_LEVEL);
    EXPECT_EQ(controller->getBandLevel(EqualizerBand::TREBLE).value(), MIN_LEVEL);

    controller->setBandLevel(EqualizerBand::MIDRANGE, ABOVE_MAX_LEVEL);
    EXPECT_EQ(controller->getBandLevel(EqualizerBand::MIDRANGE).value(), MAX_LEVEL);

    // Should crop to min
    controller->adjustBandLevels(EqualizerBandLevelMap({{EqualizerBand::MIDRANGE, -(MAX_LEVEL - MIN_LEVEL + 1)}}));
    EXPECT_EQ(controller->getBandLevel(EqualizerBand::MIDRANGE).value(), MIN_LEVEL);

    // Should crop to max
    controller->adjustBandLevels(EqualizerBandLevelMap({{EqualizerBand::MIDRANGE, (MAX_LEVEL - MIN_LEVEL + 1)}}));
    EXPECT_EQ(controller->getBandLevel(EqualizerBand::MIDRANGE).value(), MAX_LEVEL);
}

// Test mode changes
TEST_F(EqualizerControllerTest, setMode_shouldSucceed) {
    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    controller->setCurrentMode(EqualizerMode::NIGHT);

    // Check if modifications have been applied
    ASSERT_EQ(controller->getCurrentMode(), EqualizerMode::NIGHT);
}

// Test invalid modes
TEST_F(EqualizerControllerTest, setInvalidMode_shouldNotChangeMode) {
    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    controller->setCurrentMode(EqualizerMode::MOVIE);

    // Check if state remains the same
    ASSERT_EQ(controller->getCurrentMode(), DEFAULT_MODE);
}

// Test single listener reaction on band level changes
TEST_F(EqualizerControllerTest, providedBandLevelChanges_addRemoveListener_shouldFollowSubscriptionStatus) {
    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    auto listener = std::make_shared<NiceMock<MockEqualizerControllerListenerInterface>>();
    controller->addListener(listener);

    // Variable to hold state reported by onEqualizerStateChanged()
    EqualizerState reportedState;

    EXPECT_CALL(*(listener.get()), onEqualizerStateChanged(_)).Times(1).WillOnce(SaveArg<0>(&reportedState));

    controller->setBandLevel(EqualizerBand::TREBLE, NON_DEFAULT_TREBLE);
    EXPECT_EQ(reportedState.bandLevels[EqualizerBand::TREBLE], NON_DEFAULT_TREBLE);

    // Call again with the same value. Must not report changes
    EXPECT_CALL(*(listener.get()), onEqualizerStateChanged(_)).Times(0);
    controller->setBandLevel(EqualizerBand::TREBLE, NON_DEFAULT_TREBLE);

    // Reset level to default and try to adjust.
    // NOTE: Putting AtLeast(0) here to handle the case when DEFAULT_MIDRANGE == DEFAULT_LEVEL
    EXPECT_CALL(*(listener.get()), onEqualizerStateChanged(_)).Times(AtLeast(0));
    controller->setBandLevel(EqualizerBand::MIDRANGE, DEFAULT_LEVEL);
    EXPECT_CALL(*(listener.get()), onEqualizerStateChanged(_)).Times(1).WillOnce(SaveArg<0>(&reportedState));
    controller->adjustBandLevels(EqualizerBandLevelMap({{EqualizerBand::MIDRANGE, 1}}));
    ASSERT_EQ(reportedState.bandLevels[EqualizerBand::MIDRANGE], DEFAULT_LEVEL + 1);

    // Remove listener and see if it is called again
    controller->removeListener(listener);
    EXPECT_CALL(*(listener.get()), onEqualizerStateChanged(_)).Times(0);
    controller->setBandLevel(EqualizerBand::TREBLE, DEFAULT_TREBLE);
}

// Test single listener reaction on mode changes
TEST_F(EqualizerControllerTest, providedModeChanges_addRemoveListener_shouldFollowSubscriptionStatus) {
    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    auto listener = std::make_shared<NiceMock<MockEqualizerControllerListenerInterface>>();
    controller->addListener(listener);

    // Variable to hold state reported by onEqualizerStateChanged()
    EqualizerState reportedState;

    EXPECT_CALL(*(listener.get()), onEqualizerStateChanged(_)).Times(1).WillOnce(SaveArg<0>(&reportedState));
    controller->setCurrentMode(EqualizerMode::NIGHT);
    ASSERT_EQ(reportedState.mode, EqualizerMode::NIGHT);

    // Repeat same mode. Should not notify
    EXPECT_CALL(*(listener.get()), onEqualizerStateChanged(_)).Times(0);
    controller->setCurrentMode(EqualizerMode::NIGHT);

    // Change back to make sure that it works more than once
    EXPECT_CALL(*(listener.get()), onEqualizerStateChanged(_)).Times(1).WillOnce(SaveArg<0>(&reportedState));
    controller->setCurrentMode(EqualizerMode::TV);
    ASSERT_EQ(reportedState.mode, EqualizerMode::TV);

    // Try unsupported mode. Should not notify
    EXPECT_CALL(*(listener.get()), onEqualizerStateChanged(_)).Times(0);
    controller->setCurrentMode(EqualizerMode::MUSIC);
}

// Test multiple listeners reaction on changes
TEST_F(EqualizerControllerTest, providedBandLevelChanges_addRemoveMultipleListeners_shouldFollowSubscriptionStatus) {
    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    // Variables to hold state reported by onEqualizerStateChanged()
    EqualizerState reportedState1;
    EqualizerState reportedState2;

    auto listener1 = std::make_shared<NiceMock<MockEqualizerControllerListenerInterface>>();
    EXPECT_CALL(*(listener1.get()), onEqualizerStateChanged(_)).Times(1).WillOnce(SaveArg<0>(&reportedState1));
    controller->addListener(listener1);

    auto listener2 = std::make_shared<NiceMock<MockEqualizerControllerListenerInterface>>();
    EXPECT_CALL(*(listener2.get()), onEqualizerStateChanged(_)).Times(1).WillOnce(SaveArg<0>(&reportedState2));
    controller->addListener(listener2);

    // Perform change
    controller->setBandLevel(EqualizerBand::MIDRANGE, NON_DEFAULT_MIDRANGE);

    ASSERT_EQ(reportedState1.bandLevels[EqualizerBand::MIDRANGE], NON_DEFAULT_MIDRANGE);
    ASSERT_EQ(reportedState2.bandLevels[EqualizerBand::MIDRANGE], NON_DEFAULT_MIDRANGE);

    // Remove one and make sure second still receives updates
    controller->removeListener(listener1);
    EXPECT_CALL(*(listener1.get()), onEqualizerStateChanged(_)).Times(0);
    EXPECT_CALL(*(listener2.get()), onEqualizerStateChanged(_)).Times(1).WillOnce(SaveArg<0>(&reportedState2));

    // Perform change
    controller->setBandLevel(EqualizerBand::MIDRANGE, DEFAULT_MIDRANGE);

    ASSERT_EQ(reportedState2.bandLevels[EqualizerBand::MIDRANGE], DEFAULT_MIDRANGE);
}

// Test the synchronous nature of callbacks
TEST_F(EqualizerControllerTest, triggerChangesMultipleTimes_ExpectListenersNotifiedSameTimes) {
    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    auto listener = std::make_shared<NiceMock<MockEqualizerControllerListenerInterface>>();
    controller->addListener(listener);

    EXPECT_CALL(*(listener.get()), onEqualizerStateChanged(_)).Times(STABILITY_CHECK_ITERATIONS);

    for (int i = 0; i < STABILITY_CHECK_ITERATIONS; i++) {
        controller->setBandLevel(EqualizerBand::TREBLE, DEFAULT_TREBLE + 1 + i % 2);
    }

    // Set expectation again to force gmock count validation at this point
    EXPECT_CALL(*(listener.get()), onEqualizerStateChanged(_)).Times(0);
}

// Test single equalizer registrations
TEST_F(EqualizerControllerTest, providedBandLevelChanges_addRemoveSingleEqualizer_shouldFollowRegistrationStatus) {
    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    auto equalizer = std::make_shared<NiceMock<MockEqualizerInterface>>();
    ON_CALL(*(equalizer.get()), getMinimumBandLevel()).WillByDefault(Return(MIN_LEVEL));
    ON_CALL(*(equalizer.get()), getMaximumBandLevel()).WillByDefault(Return(MAX_LEVEL));
    controller->registerEqualizer(equalizer);

    // Variable to hold state reported by onEqualizerStateChanged()
    EqualizerBandLevelMap reportedBandLevels;

    EXPECT_CALL(*(equalizer.get()), setEqualizerBandLevels(_)).Times(1).WillOnce(SaveArg<0>(&reportedBandLevels));

    controller->setBandLevel(EqualizerBand::TREBLE, NON_DEFAULT_TREBLE);
    EXPECT_EQ(reportedBandLevels[EqualizerBand::TREBLE], NON_DEFAULT_TREBLE);

    // Remove equalizer and make sure no updates received
    EXPECT_CALL(*(equalizer.get()), setEqualizerBandLevels(_)).Times(0);
    controller->unregisterEqualizer(equalizer);

    controller->setBandLevel(EqualizerBand::TREBLE, DEFAULT_TREBLE);
}

// Test multiple equalizer registrations
TEST_F(EqualizerControllerTest, providedBandLevelChanges_addRemoveMultipleEqualizers_shouldFollowRegistrationStatus) {
    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    auto equalizer1 = std::make_shared<NiceMock<MockEqualizerInterface>>();
    ON_CALL(*(equalizer1.get()), getMinimumBandLevel()).WillByDefault(Return(MIN_LEVEL));
    ON_CALL(*(equalizer1.get()), getMaximumBandLevel()).WillByDefault(Return(MAX_LEVEL));
    controller->registerEqualizer(equalizer1);
    auto equalizer2 = std::make_shared<NiceMock<MockEqualizerInterface>>();
    ON_CALL(*(equalizer2.get()), getMinimumBandLevel()).WillByDefault(Return(MIN_LEVEL));
    ON_CALL(*(equalizer2.get()), getMaximumBandLevel()).WillByDefault(Return(MAX_LEVEL));
    controller->registerEqualizer(equalizer2);

    // Variables to hold state reported by onEqualizerStateChanged()
    EqualizerBandLevelMap reportedBandLevels1;
    EqualizerBandLevelMap reportedBandLevels2;

    EXPECT_CALL(*(equalizer1.get()), setEqualizerBandLevels(_)).Times(1).WillOnce(SaveArg<0>(&reportedBandLevels1));

    EXPECT_CALL(*(equalizer2.get()), setEqualizerBandLevels(_)).Times(1).WillOnce(SaveArg<0>(&reportedBandLevels2));

    controller->setBandLevel(EqualizerBand::TREBLE, NON_DEFAULT_TREBLE);
    EXPECT_EQ(reportedBandLevels1[EqualizerBand::TREBLE], NON_DEFAULT_TREBLE);
    EXPECT_EQ(reportedBandLevels2[EqualizerBand::TREBLE], NON_DEFAULT_TREBLE);

    // Remove one and make sure second still receives updates
    EXPECT_CALL(*(equalizer1.get()), setEqualizerBandLevels(_)).Times(0);
    EXPECT_CALL(*(equalizer2.get()), setEqualizerBandLevels(_)).Times(1).WillOnce(SaveArg<0>(&reportedBandLevels2));

    controller->unregisterEqualizer(equalizer1);

    controller->setBandLevel(EqualizerBand::TREBLE, DEFAULT_TREBLE);
    EXPECT_EQ(reportedBandLevels2[EqualizerBand::TREBLE], DEFAULT_TREBLE);
}

// Test synchronous nature of equalizer handling
TEST_F(EqualizerControllerTest, triggerChangesMultipleTimes_ExpectEqualizersCalledSameTimes) {
    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    auto equalizer = std::make_shared<NiceMock<MockEqualizerInterface>>();
    controller->registerEqualizer(equalizer);

    EXPECT_CALL(*(equalizer.get()), setEqualizerBandLevels(_)).Times(STABILITY_CHECK_ITERATIONS);

    for (int i = 0; i < STABILITY_CHECK_ITERATIONS; i++) {
        controller->setBandLevel(EqualizerBand::TREBLE, DEFAULT_TREBLE + 1 + i % 2);
    }

    // Set expectation again to force gmock count validation at this point
    EXPECT_CALL(*(equalizer.get()), setEqualizerBandLevels(_)).Times(0);
}

// Test persistent storage operations
TEST_F(EqualizerControllerTest, saveLoadStateWithPersistentStorage_shouldSucceed) {
    EXPECT_CALL(*(m_storage.get()), loadState()).Times(1);

    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    // Perform change and see if state is updated
    EXPECT_CALL(*(m_storage.get()), saveState(_)).Times(1);
    controller->setBandLevel(EqualizerBand::TREBLE, DEFAULT_TREBLE + 1);

    EXPECT_CALL(*(m_storage.get()), saveState(_)).Times(1);
    controller->setBandLevels(EqualizerBandLevelMap(
        {{EqualizerBand::TREBLE, DEFAULT_TREBLE + 1}, {EqualizerBand::MIDRANGE, DEFAULT_MIDRANGE + 1}}));

    EXPECT_CALL(*(m_storage.get()), saveState(_)).Times(1);
    controller->adjustBandLevels(EqualizerBandLevelMap({{EqualizerBand::TREBLE, -1}}));

    EXPECT_CALL(*(m_storage.get()), saveState(_)).Times(1);
    controller->setCurrentMode(EqualizerMode::TV);
}

TEST_F(EqualizerControllerTest, setLevelBelowEqualizerMinimum_shouldClamp) {
    EXPECT_CALL(*(m_storage.get()), loadState()).Times(1);

    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    auto equalizer = std::make_shared<NiceMock<MockEqualizerInterface>>();
    ON_CALL(*(equalizer.get()), getMinimumBandLevel()).WillByDefault(Return(MAX_LEVEL));
    ON_CALL(*(equalizer.get()), getMaximumBandLevel()).WillByDefault(Return(MAX_LEVEL));
    controller->registerEqualizer(equalizer);

    EqualizerBandLevelMap bandLevelMap;
    EXPECT_CALL(*(equalizer.get()), setEqualizerBandLevels(_)).Times(1).WillOnce(SaveArg<0>(&bandLevelMap));

    controller->setBandLevel(EqualizerBand::MIDRANGE, MIN_LEVEL);
    EXPECT_EQ(bandLevelMap[EqualizerBand::MIDRANGE], MAX_LEVEL);
}

TEST_F(EqualizerControllerTest, setLevelAboveEqualizerMaximum_shouldClamp) {
    EXPECT_CALL(*(m_storage.get()), loadState()).Times(1);

    auto controller = EqualizerController::create(m_modeController, m_configuration, m_storage);

    auto equalizer = std::make_shared<NiceMock<MockEqualizerInterface>>();
    ON_CALL(*(equalizer.get()), getMinimumBandLevel()).WillByDefault(Return(MIN_LEVEL));
    ON_CALL(*(equalizer.get()), getMaximumBandLevel()).WillByDefault(Return(MIN_LEVEL));
    controller->registerEqualizer(equalizer);

    EqualizerBandLevelMap bandLevelMap;
    EXPECT_CALL(*(equalizer.get()), setEqualizerBandLevels(_)).Times(1).WillOnce(SaveArg<0>(&bandLevelMap));

    controller->setBandLevel(EqualizerBand::MIDRANGE, MAX_LEVEL);
    EXPECT_EQ(bandLevelMap[EqualizerBand::MIDRANGE], MIN_LEVEL);
}

}  // namespace test
}  // namespace equalizer
}  // namespace alexaClientSDK
