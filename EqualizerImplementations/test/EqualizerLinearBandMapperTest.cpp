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
#include <gmock/gmock.h>

#include <functional>
#include <memory>

#include <AVSCommon/SDKInterfaces/Audio/EqualizerTypes.h>

#include "EqualizerImplementations/EqualizerLinearBandMapper.h"

namespace alexaClientSDK {
namespace equalizer {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces::audio;
using namespace alexaClientSDK::equalizer;

/// Valid number of output bands
static constexpr int VALID_NUMBER_OF_BANDS = 3;

/// Invalid number of output bands, below lower bound
static constexpr int INVALID_NUMBER_OF_BANDS_BELOW = 0;

/// Invalid number of output bands, above higher bound
static constexpr int INVALID_NUMBER_OF_BANDS_ABOVE = 999999;

/// Band level representing high value.
static constexpr int BAND_LEVEL_TOP = 10;
/// Band level representing low value.
static constexpr int BAND_LEVEL_BOTTOM = -10;
/// Band level representing no equalization.
static constexpr int BAND_LEVEL_ZERO = 0;

/// Band level representing average of top, bottom and zero levels.
static constexpr int BAND_LEVEL_AVERAGE = (BAND_LEVEL_TOP + BAND_LEVEL_BOTTOM + BAND_LEVEL_ZERO) / 3;

/**
 * Current number of AVS bands assumed in some tests. If the actual number of AVS bands changes these test would have
 * to be rewritten.
 */
static constexpr int CURRENT_NUMBER_OF_AVS_BANDS = 3;

/**
 * Test fixture for @c EqualizerLinearBandMapper tests
 */
class EqualizerLinearBandMapperTest : public ::testing::Test {
protected:
    /**
     * Helper method to test cases when all original band levels are the same and mapped levels are expected to be the
     * same. This ensures the expectation that all both AVS bands and target bands cover whole frequency spectrum.
     *
     * @param numberOfBands Number of bands to map AVS bands to.
     * @param value Band level value to be used for testing.
     */
    void testAllValuesEqual(int numberOfBands, int value);
};

void EqualizerLinearBandMapperTest::testAllValuesEqual(int numberOfBands, int value) {
    auto bandMapper = EqualizerLinearBandMapper::create(numberOfBands);
    ASSERT_THAT(bandMapper, NotNull());

    EqualizerBandLevelMap bandLevelMap;
    for (auto band : EqualizerBandValues) {
        bandLevelMap[band] = value;
    }

    std::function<void(int, int)> callback = [value](int index, int level) { EXPECT_EQ(level, value); };

    bandMapper->mapEqualizerBands(bandLevelMap, callback);
}

/// Valid parameters
TEST_F(EqualizerLinearBandMapperTest, givenValidParameters_create_shouldSucceed) {
    auto bandMapper = EqualizerLinearBandMapper::create(VALID_NUMBER_OF_BANDS);
    EXPECT_THAT(bandMapper, NotNull());
}

/// Invalid parameters
TEST_F(EqualizerLinearBandMapperTest, givenInvalidParameters_create_shouldFail) {
    auto bandMapper = EqualizerLinearBandMapper::create(INVALID_NUMBER_OF_BANDS_BELOW);
    EXPECT_THAT(bandMapper, IsNull());

    bandMapper = EqualizerLinearBandMapper::create(INVALID_NUMBER_OF_BANDS_ABOVE);
    EXPECT_THAT(bandMapper, IsNull());
}

/// Test mapping AVS bands to the same number of bands. No value must be modified.
TEST_F(EqualizerLinearBandMapperTest, givenSameAsAVSBands_map_shouldNotModifyLevels) {
    auto numberOfAVSBands = EqualizerBandValues.size();

    auto bandMapper = EqualizerLinearBandMapper::create(numberOfAVSBands);
    ASSERT_THAT(bandMapper, NotNull());

    EqualizerBandLevelMap bandLevelMap;
    int index = 0;
    for (auto band : EqualizerBandValues) {
        bandLevelMap[band] = index++;
    }

    std::function<void(int, int)> callback = [](int index, int level) { EXPECT_EQ(level, index); };

    bandMapper->mapEqualizerBands(bandLevelMap, callback);
}

/// AVS bands < target bands. Mapping non-zero value.
TEST_F(EqualizerLinearBandMapperTest, givenMoreBandsToMapTo_mapAllSameNonZero_shouldMapToSame) {
    int numberOfAVSBands = EqualizerBandValues.size();
    testAllValuesEqual(numberOfAVSBands + 1, BAND_LEVEL_TOP);
}

/// AVS bands < target bands. Mapping zero value.
TEST_F(EqualizerLinearBandMapperTest, givenMoreBandsToMapTo_mapAllSameZero_shouldMapToSame) {
    int numberOfAVSBands = EqualizerBandValues.size();
    testAllValuesEqual(numberOfAVSBands + 1, BAND_LEVEL_ZERO);
}

/// AVS bands * 2 = target bands. Mapping non-zero value. No averaged bands.
TEST_F(EqualizerLinearBandMapperTest, givenDoubleBandsToMapTo_mapAllSameNonZero_shouldMapToSame) {
    int numberOfAVSBands = EqualizerBandValues.size();
    testAllValuesEqual(numberOfAVSBands * 2, BAND_LEVEL_TOP);
}

/// AVS bands * 2 = target bands. Mapping zero value. No averaged bands.
TEST_F(EqualizerLinearBandMapperTest, givenDoubleBandsToMapTo_mapAllSameZero_shouldMapToSame) {
    int numberOfAVSBands = EqualizerBandValues.size();
    testAllValuesEqual(numberOfAVSBands * 2, BAND_LEVEL_ZERO);
}

/// AVS bands > target bands. Mapping non-zero value.
TEST_F(EqualizerLinearBandMapperTest, givenLessBandsToMapTo_mapAllSameNonZero_shouldMapToSame) {
    int numberOfAVSBands = EqualizerBandValues.size();
    testAllValuesEqual(numberOfAVSBands - 1, BAND_LEVEL_TOP);
}

/// AVS bands > target bands. Mapping zero value.
TEST_F(EqualizerLinearBandMapperTest, givenLessBandsToMapTo_mapAllSameZero_shouldMapToSame) {
    int numberOfAVSBands = EqualizerBandValues.size();
    testAllValuesEqual(numberOfAVSBands - 1, BAND_LEVEL_ZERO);
}

/// Mapping AVS bands to 1 target band. Non-zero value.
TEST_F(EqualizerLinearBandMapperTest, givenOneBandToMapTo_mapNonZero_shouldMapToSame) {
    testAllValuesEqual(1, BAND_LEVEL_TOP);
}

/// Mapping AVS bands to 1 target band. Zero value.
TEST_F(EqualizerLinearBandMapperTest, givenOneBandToMapTo_mapZero_shouldMapToSame) {
    testAllValuesEqual(1, BAND_LEVEL_ZERO);
}

/// Since mapper has a linear nature mapped values must keep the same average as original ones. Testing this here.
TEST_F(EqualizerLinearBandMapperTest, givenAnyNumberOfBands_map_shouldKeepAverageLevel) {
    int numberOfAVSBands = EqualizerBandValues.size();

    for (int targetBandsCount = 1; targetBandsCount < numberOfAVSBands * 10; targetBandsCount++) {
        auto bandMapper = EqualizerLinearBandMapper::create(targetBandsCount);
        ASSERT_THAT(bandMapper, NotNull());

        EqualizerBandLevelMap bandLevelMap = {
            {EqualizerBand::BASS, BAND_LEVEL_BOTTOM},
            {EqualizerBand::MIDRANGE, BAND_LEVEL_ZERO},
            {EqualizerBand::TREBLE, BAND_LEVEL_TOP},
        };

        int targetLevelAccumulator = 0;

        std::function<void(int, int)> callback = [&targetLevelAccumulator](int index, int level) {
            targetLevelAccumulator += level;
        };

        bandMapper->mapEqualizerBands(bandLevelMap, callback);

        EXPECT_EQ(targetLevelAccumulator / targetBandsCount, BAND_LEVEL_AVERAGE);
    }
}

/// Test specific transformation. This test depends on current number of bands supported by AVS.
TEST_F(EqualizerLinearBandMapperTest, givenMoreBandsToMapTo_mapSpecificValues_shouldMapToSpecificOutput) {
    int numberOfAVSBands = EqualizerBandValues.size();
    ASSERT_EQ(numberOfAVSBands, CURRENT_NUMBER_OF_AVS_BANDS);

    EqualizerBandLevelMap bandLevelMap = {
        {EqualizerBand::BASS, BAND_LEVEL_BOTTOM},
        {EqualizerBand::MIDRANGE, BAND_LEVEL_ZERO},
        {EqualizerBand::TREBLE, BAND_LEVEL_TOP},
    };

    std::array<int, CURRENT_NUMBER_OF_AVS_BANDS + 1> mappedValues{};

    auto bandMapper = EqualizerLinearBandMapper::create(CURRENT_NUMBER_OF_AVS_BANDS + 1);
    std::function<void(int, int)> callback = [&mappedValues](int index, int level) { mappedValues[index] = level; };

    bandMapper->mapEqualizerBands(bandLevelMap, callback);

    EXPECT_EQ(mappedValues[0], BAND_LEVEL_BOTTOM);
    EXPECT_EQ(mappedValues[1], (BAND_LEVEL_BOTTOM + BAND_LEVEL_ZERO) / 2);
    EXPECT_EQ(mappedValues[2], (BAND_LEVEL_ZERO + BAND_LEVEL_TOP) / 2);
    EXPECT_EQ(mappedValues[3], BAND_LEVEL_TOP);
}

/// Test specific transformation. This test depends on current number of bands supported by AVS.
TEST_F(EqualizerLinearBandMapperTest, givenEvenMoreBandsToMapTo_mapSpecificValues_shouldMapToSpecificOutput) {
    int numberOfAVSBands = EqualizerBandValues.size();
    ASSERT_EQ(numberOfAVSBands, CURRENT_NUMBER_OF_AVS_BANDS);

    EqualizerBandLevelMap bandLevelMap = {
        {EqualizerBand::BASS, BAND_LEVEL_BOTTOM},
        {EqualizerBand::MIDRANGE, BAND_LEVEL_ZERO},
        {EqualizerBand::TREBLE, BAND_LEVEL_TOP},
    };

    std::array<int, CURRENT_NUMBER_OF_AVS_BANDS + 2> mappedValues{};

    auto bandMapper = EqualizerLinearBandMapper::create(CURRENT_NUMBER_OF_AVS_BANDS + 2);
    std::function<void(int, int)> callback = [&mappedValues](int index, int level) { mappedValues[index] = level; };

    bandMapper->mapEqualizerBands(bandLevelMap, callback);

    EXPECT_EQ(mappedValues[0], BAND_LEVEL_BOTTOM);
    EXPECT_EQ(mappedValues[1], (BAND_LEVEL_BOTTOM + BAND_LEVEL_ZERO) / 2);
    EXPECT_EQ(mappedValues[2], BAND_LEVEL_ZERO);
    EXPECT_EQ(mappedValues[3], (BAND_LEVEL_ZERO + BAND_LEVEL_TOP) / 2);
    EXPECT_EQ(mappedValues[4], BAND_LEVEL_TOP);
}

/// Test specific transformation. This test depends on current number of bands supported by AVS.
TEST_F(EqualizerLinearBandMapperTest, givenLessBandsToMapTo_mapSpecificValues_shouldMapToSpecificOutput) {
    int numberOfAVSBands = EqualizerBandValues.size();
    ASSERT_EQ(numberOfAVSBands, CURRENT_NUMBER_OF_AVS_BANDS);

    EqualizerBandLevelMap bandLevelMap = {
        {EqualizerBand::BASS, BAND_LEVEL_BOTTOM},
        {EqualizerBand::MIDRANGE, BAND_LEVEL_ZERO},
        {EqualizerBand::TREBLE, BAND_LEVEL_TOP},
    };

    std::array<int, CURRENT_NUMBER_OF_AVS_BANDS - 1> mappedValues{};

    auto bandMapper = EqualizerLinearBandMapper::create(CURRENT_NUMBER_OF_AVS_BANDS - 1);
    std::function<void(int, int)> callback = [&mappedValues](int index, int level) { mappedValues[index] = level; };

    bandMapper->mapEqualizerBands(bandLevelMap, callback);

    EXPECT_EQ(mappedValues[0], (BAND_LEVEL_BOTTOM + BAND_LEVEL_ZERO) / 2);
    EXPECT_EQ(mappedValues[1], (BAND_LEVEL_TOP + BAND_LEVEL_ZERO) / 2);
}

/// Test specific transformation. This test depends on current number of bands supported by AVS.
TEST_F(EqualizerLinearBandMapperTest, givenEvenLessBandsToMapTo_mapSpecificValues_shouldMapToSpecificOutput) {
    int numberOfAVSBands = EqualizerBandValues.size();
    ASSERT_EQ(numberOfAVSBands, CURRENT_NUMBER_OF_AVS_BANDS);

    EqualizerBandLevelMap bandLevelMap = {
        {EqualizerBand::BASS, BAND_LEVEL_BOTTOM},
        {EqualizerBand::MIDRANGE, BAND_LEVEL_ZERO},
        {EqualizerBand::TREBLE, BAND_LEVEL_TOP},
    };

    std::array<int, CURRENT_NUMBER_OF_AVS_BANDS - 2> mappedValues{};

    auto bandMapper = EqualizerLinearBandMapper::create(CURRENT_NUMBER_OF_AVS_BANDS - 2);
    std::function<void(int, int)> callback = [&mappedValues](int index, int level) { mappedValues[index] = level; };

    bandMapper->mapEqualizerBands(bandLevelMap, callback);

    EXPECT_EQ(mappedValues[0], (BAND_LEVEL_BOTTOM + BAND_LEVEL_ZERO + BAND_LEVEL_TOP) / 3);
}

///
TEST_F(EqualizerLinearBandMapperTest, givenOneBandSupported_mapToOneBand_shouldMapSameValue) {
    EqualizerBandLevelMap bandLevelMap = {{EqualizerBand::BASS, BAND_LEVEL_BOTTOM}};

    std::array<int, 1> mappedValues{};

    auto bandMapper = EqualizerLinearBandMapper::create(1);
    std::function<void(int, int)> callback = [&mappedValues](int index, int level) { mappedValues[index] = level; };

    bandMapper->mapEqualizerBands(bandLevelMap, callback);

    EXPECT_EQ(mappedValues[0], BAND_LEVEL_BOTTOM);
}

TEST_F(EqualizerLinearBandMapperTest, givenOneBandSupported_mapToTwoBands_shouldMapToBothSame) {
    EqualizerBandLevelMap bandLevelMap = {{EqualizerBand::BASS, BAND_LEVEL_BOTTOM}};

    std::array<int, 2> mappedValues{};

    auto bandMapper = EqualizerLinearBandMapper::create(2);
    std::function<void(int, int)> callback = [&mappedValues](int index, int level) { mappedValues[index] = level; };

    bandMapper->mapEqualizerBands(bandLevelMap, callback);

    EXPECT_EQ(mappedValues[0], BAND_LEVEL_BOTTOM);
    EXPECT_EQ(mappedValues[1], BAND_LEVEL_BOTTOM);
}

TEST_F(EqualizerLinearBandMapperTest, givenTwoBandsSupported_mapToOneBands_shouldMapToAverage) {
    EqualizerBandLevelMap bandLevelMap = {{EqualizerBand::BASS, BAND_LEVEL_BOTTOM},
                                          {EqualizerBand::MIDRANGE, BAND_LEVEL_ZERO}};

    std::array<int, 1> mappedValues{};

    auto bandMapper = EqualizerLinearBandMapper::create(1);
    std::function<void(int, int)> callback = [&mappedValues](int index, int level) { mappedValues[index] = level; };

    bandMapper->mapEqualizerBands(bandLevelMap, callback);

    EXPECT_EQ(mappedValues[0], (BAND_LEVEL_BOTTOM + BAND_LEVEL_ZERO) / 2);
}

}  // namespace test
}  // namespace equalizer
}  // namespace alexaClientSDK
