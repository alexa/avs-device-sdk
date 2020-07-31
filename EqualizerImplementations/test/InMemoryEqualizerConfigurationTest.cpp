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
#include <gtest/gtest.h>

#include <list>
#include <memory>

#include "EqualizerImplementations/InMemoryEqualizerConfiguration.h"

namespace alexaClientSDK {
namespace equalizer {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces::audio;

/// Band level used as a minimum .
static constexpr int MIN_LEVEL = -6;
/// Band level used as a maximum.
static constexpr int MAX_LEVEL = 6;
/// Default value to adjust band level.
static constexpr int DEFAULT_ADJUST_DELTA = 1;

/// Band level below minimum value
static constexpr int BELOW_MIN_LEVEL = MIN_LEVEL - 100;

/// Band level above maximum value
static constexpr int ABOVE_MAX_LEVEL = MAX_LEVEL + 100;

/// Band level used as a default
static constexpr int DEFAULT_LEVEL = 0;

/**
 * Test fixture for @c InMemoryEqualizerConfiguration tests.
 */
class InMemoryEqualizerConfigurationTest : public ::testing::Test {
public:
protected:
    /// Equalizer configuration used in tests.
    std::shared_ptr<InMemoryEqualizerConfiguration> m_configuration;

    /// Returns a set of bands to assumed to be supported by default.
    std::set<EqualizerBand> getDefaultBands();

    /// Returns a set of modes to assumed to be supported by default.
    std::set<EqualizerMode> getDefaultModes();

    /// Returns an equalizer state to assumed to be supported by default
    EqualizerState getDefaultState();
};

std::set<EqualizerBand> InMemoryEqualizerConfigurationTest::getDefaultBands() {
    return std::set<EqualizerBand>({EqualizerBand::BASS, EqualizerBand::MIDRANGE, EqualizerBand::TREBLE});
}

std::set<EqualizerMode> InMemoryEqualizerConfigurationTest::getDefaultModes() {
    return std::set<EqualizerMode>(
        {EqualizerMode::MOVIE, EqualizerMode::MUSIC, EqualizerMode::NIGHT, EqualizerMode::SPORT, EqualizerMode::TV});
}

EqualizerState InMemoryEqualizerConfigurationTest::getDefaultState() {
    return EqualizerState{EqualizerMode::NONE,
                          EqualizerBandLevelMap({{EqualizerBand::BASS, DEFAULT_LEVEL},
                                                 {EqualizerBand::MIDRANGE, DEFAULT_LEVEL},
                                                 {EqualizerBand::TREBLE, DEFAULT_LEVEL}})};
}

/// Simple successful case
TEST_F(InMemoryEqualizerConfigurationTest, test_providedValidParameters_createInstance_shouldSucceed) {
    m_configuration = InMemoryEqualizerConfiguration::create(
        MIN_LEVEL, MAX_LEVEL, DEFAULT_ADJUST_DELTA, getDefaultBands(), getDefaultModes(), getDefaultState());
    ASSERT_THAT(m_configuration, NotNull());
}

/// Min level > Max level
TEST_F(InMemoryEqualizerConfigurationTest, test_providedInvalidLevelRange_createInstance_shouldFail) {
    m_configuration = InMemoryEqualizerConfiguration::create(
        MAX_LEVEL, MIN_LEVEL, DEFAULT_ADJUST_DELTA, getDefaultBands(), getDefaultModes(), getDefaultState());
    ASSERT_THAT(m_configuration, IsNull());
}

/// adjust delta of 0
TEST_F(InMemoryEqualizerConfigurationTest, test_providedInvalidBandDelta_createInstance_shouldFail) {
    m_configuration = InMemoryEqualizerConfiguration::create(
        MIN_LEVEL, MAX_LEVEL, 0, getDefaultBands(), getDefaultModes(), getDefaultState());
    ASSERT_THAT(m_configuration, IsNull());
}

/// Min and Max are equal (DEFAULT_LEVEL), must succeed
TEST_F(InMemoryEqualizerConfigurationTest, test_providedMixMaxLevelSetToDefault_createInstance_shouldSucceed) {
    m_configuration = InMemoryEqualizerConfiguration::create(
        DEFAULT_LEVEL, DEFAULT_LEVEL, DEFAULT_ADJUST_DELTA, getDefaultBands(), getDefaultModes(), getDefaultState());
    ASSERT_THAT(m_configuration, NotNull());
}

/// Min and Max are equal (non-DEFAULT_LEVEL), must fail because all modes use DEFAULT_LEVEL as band levels.
TEST_F(InMemoryEqualizerConfigurationTest, test_providedSameNonDefaultMixMaxLevel_createInstance_shouldSucceed) {
    m_configuration = InMemoryEqualizerConfiguration::create(
        MAX_LEVEL, MAX_LEVEL, DEFAULT_ADJUST_DELTA, getDefaultBands(), getDefaultModes(), getDefaultState());
    ASSERT_THAT(m_configuration, IsNull());
}

/// Invalid band value in default state (above max)
TEST_F(InMemoryEqualizerConfigurationTest, test_providedDefaultStateLevelAboveMax_createInstance_shouldFail) {
    auto state = getDefaultState();
    state.bandLevels[EqualizerBand::TREBLE] = ABOVE_MAX_LEVEL;
    m_configuration = InMemoryEqualizerConfiguration::create(
        MIN_LEVEL, MAX_LEVEL, DEFAULT_ADJUST_DELTA, getDefaultBands(), getDefaultModes(), state);
    ASSERT_THAT(m_configuration, IsNull());
}

/// Invalid band value in default state (below min)
TEST_F(InMemoryEqualizerConfigurationTest, test_providedDefaultStateLevelBelowMin_createInstance_shouldFail) {
    auto state = getDefaultState();
    state.bandLevels[EqualizerBand::TREBLE] = BELOW_MIN_LEVEL;
    m_configuration = InMemoryEqualizerConfiguration::create(
        MIN_LEVEL, MAX_LEVEL, DEFAULT_ADJUST_DELTA, getDefaultBands(), getDefaultModes(), state);
    ASSERT_THAT(m_configuration, IsNull());
}

/// Invalid band value in default state (below min, another band)
TEST_F(
    InMemoryEqualizerConfigurationTest,
    test_providedDefaultStateLevelBelowMinDifferentBand_createInstance_shouldFail) {
    auto state = getDefaultState();
    state.bandLevels[EqualizerBand::BASS] = BELOW_MIN_LEVEL;
    m_configuration = InMemoryEqualizerConfiguration::create(
        MIN_LEVEL, MAX_LEVEL, DEFAULT_ADJUST_DELTA, getDefaultBands(), getDefaultModes(), state);
    ASSERT_THAT(m_configuration, IsNull());
}

// Bands

// Test no modes supported
TEST_F(InMemoryEqualizerConfigurationTest, test_providedNoModes_createInstance_shouldSucceed) {
    auto bands = std::set<EqualizerBand>({EqualizerBand::MIDRANGE});
    auto state = EqualizerState{EqualizerMode::NONE, EqualizerBandLevelMap({{EqualizerBand::MIDRANGE, DEFAULT_LEVEL}})};
    auto modes = std::set<EqualizerMode>({});
    m_configuration =
        InMemoryEqualizerConfiguration::create(MIN_LEVEL, MAX_LEVEL, DEFAULT_ADJUST_DELTA, bands, modes, state);
    ASSERT_THAT(m_configuration, NotNull());
}

// Default state with supported mode
TEST_F(InMemoryEqualizerConfigurationTest, test_providedSupportedModeInDefaultState_createInstance_shouldSucceed) {
    auto bands = std::set<EqualizerBand>({EqualizerBand::MIDRANGE});
    auto state =
        EqualizerState{EqualizerMode::NIGHT, EqualizerBandLevelMap({{EqualizerBand::MIDRANGE, DEFAULT_LEVEL}})};
    auto modes = std::set<EqualizerMode>({EqualizerMode::NIGHT});
    m_configuration =
        InMemoryEqualizerConfiguration::create(MIN_LEVEL, MAX_LEVEL, DEFAULT_ADJUST_DELTA, bands, modes, state);
    ASSERT_THAT(m_configuration, NotNull());
}

// Default state with unsupported mode
TEST_F(InMemoryEqualizerConfigurationTest, test_providedUnsupportedModeInDefaultState_createInstance_shouldFail) {
    auto bands = std::set<EqualizerBand>({EqualizerBand::MIDRANGE});
    auto state = EqualizerState{EqualizerMode::TV, EqualizerBandLevelMap({{EqualizerBand::MIDRANGE, DEFAULT_LEVEL}})};
    auto modes = std::set<EqualizerMode>({EqualizerMode::NIGHT});
    m_configuration =
        InMemoryEqualizerConfiguration::create(MIN_LEVEL, MAX_LEVEL, DEFAULT_ADJUST_DELTA, bands, modes, state);
    ASSERT_THAT(m_configuration, IsNull());
}

// EqualizerMode::NONE could be provided as valid mode but will be ignored
TEST_F(InMemoryEqualizerConfigurationTest, test_providedNONEModeAsSupported_createInstance_shouldSucceed) {
    auto bands = std::set<EqualizerBand>({EqualizerBand::MIDRANGE});
    auto state = EqualizerState{EqualizerMode::NONE, EqualizerBandLevelMap({{EqualizerBand::MIDRANGE, DEFAULT_LEVEL}})};
    auto modes = std::set<EqualizerMode>({EqualizerMode::NIGHT, EqualizerMode::NONE});
    m_configuration =
        InMemoryEqualizerConfiguration::create(MIN_LEVEL, MAX_LEVEL, DEFAULT_ADJUST_DELTA, bands, modes, state);
    ASSERT_THAT(m_configuration, NotNull());
}

TEST_F(InMemoryEqualizerConfigurationTest, test_providedValidConfiguration_isSupportedMethods_shouldSucceed) {
    auto bands = std::set<EqualizerBand>({EqualizerBand::MIDRANGE});
    auto state = EqualizerState{EqualizerMode::NONE, EqualizerBandLevelMap({{EqualizerBand::MIDRANGE, DEFAULT_LEVEL}})};
    auto modes = std::set<EqualizerMode>({EqualizerMode::NIGHT, EqualizerMode::TV});
    m_configuration =
        InMemoryEqualizerConfiguration::create(MIN_LEVEL, MAX_LEVEL, DEFAULT_ADJUST_DELTA, bands, modes, state);
    ASSERT_THAT(m_configuration, NotNull());

    EXPECT_TRUE(m_configuration->isBandSupported(EqualizerBand::MIDRANGE));
    EXPECT_FALSE(m_configuration->isBandSupported(EqualizerBand::TREBLE));

    // NONE mode, usual case, but let's check it anywhere
    EXPECT_FALSE(m_configuration->isModeSupported(EqualizerMode::NONE));

    EXPECT_TRUE(m_configuration->isModeSupported(EqualizerMode::NIGHT));
    EXPECT_TRUE(m_configuration->isModeSupported(EqualizerMode::TV));
    EXPECT_FALSE(m_configuration->isModeSupported(EqualizerMode::SPORT));
}

}  // namespace test
}  // namespace equalizer
}  // namespace alexaClientSDK
