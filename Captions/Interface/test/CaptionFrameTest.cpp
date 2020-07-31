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

/// @file CaptionFrameTest.cpp

#include <gtest/gtest.h>

#include <Captions/CaptionFrame.h>

namespace alexaClientSDK {
namespace captions {
namespace test {

using namespace std;

/**
 * Tests the insertion operator for an empty CaptionFrame object.
 */
TEST(CaptionFrameTest, test_putToOperatorOnEmptyCaptionFrame) {
    auto captionFrame = CaptionFrame();
    std::stringstream out;
    out << captionFrame;
    ASSERT_EQ(out.str(), "CaptionFrame(id:0, duration:0, delay:0, lines:[])");
}

}  // namespace test
}  // namespace captions
}  // namespace alexaClientSDK