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

#include <memory>

#include <AVSCommon/SDKInterfaces/Audio/EqualizerStorageInterfaceTest.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace audio {
namespace test {

using namespace ::testing;
using namespace sdkInterfaces::audio::test;
using namespace sdkInterfaces::audio;

void EqualizerStorageInterfaceTest::SetUp() {
    auto factory = GetParam();
    m_storage = factory();
}

/**
 * Save state to the storage and see if loadState() returns the saved state.
 */
TEST_P(EqualizerStorageInterfaceTest, saveState_ExpectLoadReturnSame) {
    EqualizerState defaultState = {EqualizerMode::MOVIE,
                                   EqualizerBandLevelMap({{EqualizerBand::TREBLE, 0}, {EqualizerBand::MIDRANGE, 1}})};
    m_storage->saveState(defaultState);

    auto loadedStateResult = m_storage->loadState();

    EXPECT_EQ(loadedStateResult.value(), defaultState);

    EqualizerState state = {EqualizerMode::TV,
                            EqualizerBandLevelMap({{EqualizerBand::TREBLE, 10}, {EqualizerBand::MIDRANGE, 11}})};
    m_storage->saveState(state);

    loadedStateResult = m_storage->loadState();

    EXPECT_EQ(loadedStateResult.value(), state);
}

/**
 * Perform cleaning of the data in the storage and see if next loadState() returns the default state.
 */
TEST_P(EqualizerStorageInterfaceTest, clearSavedData_ExpectAllDefaultsOnLoad) {
    EqualizerState defaultState = {EqualizerMode::MOVIE,
                                   EqualizerBandLevelMap({{EqualizerBand::TREBLE, 0}, {EqualizerBand::MIDRANGE, 1}})};

    EqualizerState state = {EqualizerMode::MOVIE,
                            EqualizerBandLevelMap({{EqualizerBand::TREBLE, 10}, {EqualizerBand::BASS, 11}})};
    m_storage->saveState(state);
    m_storage->clear();

    auto loadedStateResult = m_storage->loadState();
    EXPECT_FALSE(loadedStateResult.isSucceeded());
}

}  // namespace test
}  // namespace audio
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK
