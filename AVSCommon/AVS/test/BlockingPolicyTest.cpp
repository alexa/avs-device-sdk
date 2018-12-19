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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AVSCommon/AVS/BlockingPolicy.h"

using namespace testing;

namespace alexaClientSDK {
namespace avsCommon {
namespace test {

using namespace avsCommon::avs;

class BlockingPolicyTest : public ::testing::Test {};

// test defaultConstructor
TEST_F(BlockingPolicyTest, testDefaultConstructor) {
    BlockingPolicy blockingPolicy;

    ASSERT_FALSE(blockingPolicy.isValid());
}

/// Test isBlocking
TEST_F(BlockingPolicyTest, testisBlocking) {
    BlockingPolicy blocking(BlockingPolicy::MEDIUM_VISUAL, true);
    BlockingPolicy nonBlocking(BlockingPolicy::MEDIUM_VISUAL, false);

    ASSERT_TRUE(blocking.isBlocking());
    ASSERT_FALSE(nonBlocking.isBlocking());
}

/// Test getMediums
TEST_F(BlockingPolicyTest, testgetMediums) {
    BlockingPolicy audio(BlockingPolicy::MEDIUM_AUDIO, false);
    BlockingPolicy visual(BlockingPolicy::MEDIUM_VISUAL, false);
    BlockingPolicy audioAndVisual(BlockingPolicy::MEDIUMS_AUDIO_AND_VISUAL, false);
    BlockingPolicy none(BlockingPolicy::MEDIUMS_NONE, false);

    ASSERT_EQ(audio.getMediums(), BlockingPolicy::MEDIUM_AUDIO);
    ASSERT_EQ(visual.getMediums(), BlockingPolicy::MEDIUM_VISUAL);
    ASSERT_EQ(audioAndVisual.getMediums(), BlockingPolicy::MEDIUMS_AUDIO_AND_VISUAL);
    ASSERT_EQ(none.getMediums(), BlockingPolicy::MEDIUMS_NONE);
}

}  // namespace test
}  // namespace avsCommon
}  // namespace alexaClientSDK
