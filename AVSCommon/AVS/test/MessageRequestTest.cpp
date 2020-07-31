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

#include "AVSCommon/AVS/MessageRequest.h"

using namespace ::testing;

namespace alexaClientSDK {

namespace avsCommon {
namespace avs {
namespace test {

class MessageRequestTest : public ::testing::Test {};

/**
 * Test functionality of adding extra headers
 */
TEST_F(MessageRequestTest, test_extraHeaders) {
    std::vector<std::pair<std::string, std::string>> expected({{"k1", "v1"}, {"k2", "v2"}});
    MessageRequest messageRequest("{}", true, "", expected);
    auto actual = messageRequest.getHeaders();

    EXPECT_EQ(expected, actual);
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
