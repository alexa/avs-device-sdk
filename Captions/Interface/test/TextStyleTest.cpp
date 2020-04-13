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

/// @file TextStyleTest.cpp

#include <gtest/gtest.h>

#include <Captions/TextStyle.h>

namespace alexaClientSDK {
namespace captions {
namespace test {

using namespace std;

/**
 * Tests the insertion operator for an empty TextStyle object.
 */
TEST(TextStyleTest, test_putToOperatorOnEmptyTextStyle) {
    auto textStyle = TextStyle();
    std::stringstream out;
    out << textStyle;
    ASSERT_EQ(out.str(), "TextStyle(charIndex:0, activeStyle:Style(bold=0, underline=0, italic=0))");
}

}  // namespace test
}  // namespace captions
}  // namespace alexaClientSDK