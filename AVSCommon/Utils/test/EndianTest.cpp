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

#include <gtest/gtest.h>

#include "AVSCommon/Utils/Endian.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

using namespace ::testing;

class EndianTest : public Test {};

TEST_F(EndianTest, test_SwapEndian16) {
    uint16_t AB = 0x4142;
    uint16_t BA = 0x4241;

    EXPECT_EQ(AB, swapEndian(BA));
}

TEST_F(EndianTest, test_SwapEndian32) {
    uint32_t ABCD = 0x41424344;
    uint32_t DCBA = 0x44434241;

    EXPECT_EQ(ABCD, swapEndian(DCBA));
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK