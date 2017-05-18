/*
 * MessageTest.cpp
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/AVS/Message.h>

#include <gtest/gtest.h>

namespace alexaClientSDK {
namespace acl {

class MessageTest : public ::testing::Test {

};

TEST_F(MessageTest, default_message_json_content_is_empty) {
    const std::string expected = "";
    avsCommon::avs::Message message(expected);
    std::string actual = message.getJSONContent();

    ASSERT_EQ(expected, actual);
}

TEST_F(MessageTest, message_maintains_json_content) {
    const std::string expected = "{\"foo\":\"bar\"}";
    avsCommon::avs::Message message(expected);
    std::string actual = message.getJSONContent();

    ASSERT_EQ(expected, actual);
}

} // namespace acl
} // namespace alexaClientSDK
