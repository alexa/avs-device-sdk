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

/// @file CaptionLineTest.cpp

#include <gtest/gtest.h>

#include <Captions/CaptionLine.h>
#include <Captions/TextStyle.h>

namespace alexaClientSDK {
namespace captions {
namespace test {

using namespace std;

/**
 * Tests that a zero splitting index returns sanely.
 */
TEST(CaptionLineTest, test_noStylesSplitIndexZero) {
    CaptionLine c1 = CaptionLine("The time is 2:17 PM.", {});
    auto splitC1 = c1.splitAtTextIndex(0);

    ASSERT_EQ(splitC1[0], CaptionLine("", {}));
    ASSERT_EQ(splitC1[1], CaptionLine("The time is 2:17 PM.", {}));
}

/**
 * Tests that an out of bounds splitting index returns sanely.
 */
TEST(CaptionLineTest, test_noStylesSplitIndexOutOfBounds) {
    CaptionLine c1 = CaptionLine("The time is 2:17 PM.", {});
    auto splitC1 = c1.splitAtTextIndex(100);

    ASSERT_EQ(splitC1.size(), (size_t)1);
    ASSERT_EQ(splitC1[0], CaptionLine("The time is 2:17 PM.", {}));
}

/**
 * Tests the insertion operator on an empty CaptionLine.
 */
TEST(CaptionLineTest, test_putToOnEmptyCaptionLine) {
    CaptionLine c1 = CaptionLine();
    std::stringstream out;
    out << c1;
    ASSERT_EQ(out.str(), "CaptionLine(text:\"\", styles:[])");
}

/**
 * Tests splitting on the last character index will not break.
 */
TEST(CaptionLineTest, test_singleStyleSplit) {
    // inputs
    std::vector<TextStyle> styles;

    Style s0 = Style();
    styles.emplace_back(TextStyle(0, s0));

    CaptionLine line1 = CaptionLine("Currently, in Ashburn it's 73 degrees Fahrenheit with clear skies.", styles);

    // test action
    auto splitLine1 = line1.splitAtTextIndex(66);

    // expected outputs
    std::vector<TextStyle> expectedFirstLineStyles;

    Style firstLineStyle1 = Style();
    expectedFirstLineStyles.emplace_back(TextStyle(0, firstLineStyle1));

    std::vector<TextStyle> expectedSecondLineStyles;

    Style secondLineStyle1 = Style();
    expectedSecondLineStyles.emplace_back(TextStyle(0, secondLineStyle1));

    ASSERT_EQ(
        splitLine1[0],
        CaptionLine("Currently, in Ashburn it's 73 degrees Fahrenheit with clear skies.", expectedFirstLineStyles));
    ASSERT_EQ(splitLine1[1], CaptionLine("", expectedSecondLineStyles));
}

/**
 * Tests that splitting before a single style adjusts indices with whitespace present.
 */
TEST(CaptionLineTest, test_singleStyleSplitBeforeWhitespace) {
    // inputs
    std::vector<TextStyle> styles;

    Style s0 = Style();
    styles.emplace_back(TextStyle(0, s0));

    Style s1 = Style();
    s1.m_bold = true;
    styles.emplace_back(TextStyle(4, s1));

    Style s2 = Style();
    s2.m_bold = false;
    styles.emplace_back(TextStyle(8, s2));

    CaptionLine line1 = CaptionLine("The time is 2:17 PM.", styles);

    // test action
    auto splitLine1 = line1.splitAtTextIndex(3);

    // expected outputs
    std::vector<TextStyle> expectedFirstLineStyles;

    Style firstLineStyle1 = Style();
    expectedFirstLineStyles.emplace_back(TextStyle(0, firstLineStyle1));

    std::vector<TextStyle> expectedSecondLineStyles;

    Style secondLineStyle1 = Style();
    secondLineStyle1.m_bold = true;
    expectedSecondLineStyles.emplace_back(TextStyle(0, secondLineStyle1));

    Style secondLineStyle2 = Style();
    secondLineStyle2.m_bold = false;
    expectedSecondLineStyles.emplace_back(TextStyle(4, secondLineStyle2));

    ASSERT_EQ(splitLine1[0], CaptionLine("The", expectedFirstLineStyles));
    ASSERT_EQ(splitLine1[1], CaptionLine("time is 2:17 PM.", expectedSecondLineStyles));
}

/**
 * Test for sane index handling when the text contains only whitespace.
 */
TEST(CaptionLineTest, test_indexAdjustmentWithWhitespaceContent) {
    // inputs
    std::vector<TextStyle> styles;

    Style s0 = Style();
    styles.emplace_back(TextStyle(0, s0));

    Style s1 = Style();
    s1.m_bold = true;
    styles.emplace_back(TextStyle(1, s1));

    Style s2 = Style();
    s2.m_bold = false;
    styles.emplace_back(TextStyle(3, s2));

    CaptionLine line1 = CaptionLine("                    ", styles);

    // test action
    auto splitLine1 = line1.splitAtTextIndex(1);

    // expected outputs
    std::vector<TextStyle> expectedFirstLineStyles;

    Style firstLineStyle1 = Style();
    expectedFirstLineStyles.emplace_back(TextStyle(0, firstLineStyle1));

    std::vector<TextStyle> expectedSecondLineStyles;

    Style secondLineStyle1 = Style();
    secondLineStyle1.m_bold = true;
    expectedSecondLineStyles.emplace_back(TextStyle(0, secondLineStyle1));

    Style secondLineStyle2 = Style();
    secondLineStyle2.m_bold = false;
    expectedSecondLineStyles.emplace_back(TextStyle(0, secondLineStyle2));

    ASSERT_EQ(splitLine1[0], CaptionLine(" ", expectedFirstLineStyles));
    ASSERT_EQ(splitLine1[1], CaptionLine("", expectedSecondLineStyles));
}

/**
 * Test for sane index handling when the caption line contains many spaces before text.
 */
TEST(CaptionLineTest, test_indexAdjustmentWithSeveralWhitespacesBeforeContent) {
    // inputs
    std::vector<TextStyle> styles;

    Style s0 = Style();
    styles.emplace_back(TextStyle(0, s0));

    Style s1 = Style();
    s1.m_bold = true;
    styles.emplace_back(TextStyle(34, s1));

    Style s2 = Style();
    s2.m_bold = false;
    styles.emplace_back(TextStyle(45, s2));

    CaptionLine line1 = CaptionLine("No styles here                    bolded here", styles);

    // test action
    auto splitLine1 = line1.splitAtTextIndex(16);

    // expected outputs
    std::vector<TextStyle> expectedFirstLineStyles;

    Style firstLineStyle1 = Style();
    expectedFirstLineStyles.emplace_back(TextStyle(0, firstLineStyle1));

    std::vector<TextStyle> expectedSecondLineStyles;

    Style secondLineStyle1 = Style();
    secondLineStyle1.m_bold = true;
    expectedSecondLineStyles.emplace_back(TextStyle(0, secondLineStyle1));

    Style secondLineStyle2 = Style();
    secondLineStyle2.m_bold = false;
    expectedSecondLineStyles.emplace_back(TextStyle(11, secondLineStyle2));

    ASSERT_EQ(splitLine1[0], CaptionLine("No styles here  ", expectedFirstLineStyles));
    ASSERT_EQ(splitLine1[1], CaptionLine("bolded here", expectedSecondLineStyles));
}

/**
 * Tests that splitting after a single style maintains indices.
 */
TEST(CaptionLineTest, test_singleStyleSplitAfter) {
    // inputs
    std::vector<TextStyle> styles;

    Style s0 = Style();
    styles.emplace_back(TextStyle(0, s0));

    Style s1 = Style();
    s1.m_bold = true;
    styles.emplace_back(TextStyle(4, s1));

    Style s2 = Style();
    s2.m_bold = false;
    styles.emplace_back(TextStyle(8, s2));

    CaptionLine line1 = CaptionLine("The time is 2:17 PM.", styles);

    // test action
    auto splitLine1 = line1.splitAtTextIndex(9);

    // expected outputs
    std::vector<TextStyle> expectedFirstLineStyles;

    Style firstLineStyle1 = Style();
    expectedFirstLineStyles.emplace_back(TextStyle(0, firstLineStyle1));

    Style firstLineStyle2 = Style();
    firstLineStyle2.m_bold = true;
    expectedFirstLineStyles.emplace_back(TextStyle(4, firstLineStyle2));

    Style firstLineStyle3 = Style();
    firstLineStyle3.m_bold = false;
    expectedFirstLineStyles.emplace_back(TextStyle(8, firstLineStyle3));

    std::vector<TextStyle> expectedSecondLineStyles;

    Style secondLineStyle1 = Style();
    expectedSecondLineStyles.emplace_back(TextStyle(0, secondLineStyle1));

    ASSERT_EQ(splitLine1[0], CaptionLine("The time ", expectedFirstLineStyles));
    ASSERT_EQ(splitLine1[1], CaptionLine("is 2:17 PM.", expectedSecondLineStyles));
}

/**
 * Tests that splitting in the middle of a single style adjusts indices.
 */
TEST(CaptionLineTest, test_singleStyleSplitMiddle) {
    // inputs
    std::vector<TextStyle> styles;

    Style s0 = Style();
    styles.emplace_back(TextStyle(0, s0));

    Style s1 = Style();
    s1.m_bold = true;
    styles.emplace_back(TextStyle(4, s1));

    Style s2 = Style();
    s2.m_bold = false;
    styles.emplace_back(TextStyle(8, s2));

    CaptionLine line1 = CaptionLine("The time is 2:17 PM.", styles);

    // test action
    auto splitLine1 = line1.splitAtTextIndex(6);

    // expected outputs
    std::vector<TextStyle> expectedFirstLineStyles;

    Style firstLineStyle1 = Style();
    expectedFirstLineStyles.emplace_back(TextStyle(0, firstLineStyle1));

    Style firstLineStyle2 = Style();
    firstLineStyle2.m_bold = true;
    expectedFirstLineStyles.emplace_back(TextStyle(4, firstLineStyle2));

    std::vector<TextStyle> expectedSecondLineStyles;

    Style secondLineStyle1 = Style();
    secondLineStyle1.m_bold = true;
    expectedSecondLineStyles.emplace_back(TextStyle(0, secondLineStyle1));

    Style secondLineStyle2 = Style();
    secondLineStyle2.m_bold = false;
    expectedSecondLineStyles.emplace_back(TextStyle(2, secondLineStyle2));

    ASSERT_EQ(splitLine1[0], CaptionLine("The ti", expectedFirstLineStyles));
    ASSERT_EQ(splitLine1[1], CaptionLine("me is 2:17 PM.", expectedSecondLineStyles));
}

/**
 * Tests that splitting before multiple styles adjusts indices.
 */
TEST(CaptionLineTest, test_multipleStyleSplitBefore) {
    // inputs
    std::vector<TextStyle> styles;

    Style s0 = Style();
    styles.emplace_back(TextStyle(0, s0));

    Style s1_open = Style();
    s1_open.m_bold = true;
    styles.emplace_back(TextStyle(4, s1_open));

    Style s1_close = Style();
    s1_close.m_bold = false;
    styles.emplace_back(TextStyle(8, s1_close));

    Style s2_open = Style();
    s2_open.m_italic = true;
    styles.emplace_back(TextStyle(12, s2_open));

    Style s2_close = Style();
    s2_close.m_italic = false;
    styles.emplace_back(TextStyle(19, s2_close));

    CaptionLine line1 = CaptionLine("The time is 2:17 PM.", styles);

    // test action
    auto splitLine1 = line1.splitAtTextIndex(3);

    // expected outputs
    std::vector<TextStyle> expectedFirstLineStyles;

    Style firstLineStyle0 = Style();
    expectedFirstLineStyles.emplace_back(TextStyle(0, firstLineStyle0));

    std::vector<TextStyle> expectedSecondLineStyles;

    Style secondLineStyle1_open = Style();
    secondLineStyle1_open.m_bold = true;
    expectedSecondLineStyles.emplace_back(TextStyle(0, secondLineStyle1_open));

    Style secondLineStyle1_close = Style();
    secondLineStyle1_close.m_bold = false;
    expectedSecondLineStyles.emplace_back(TextStyle(4, secondLineStyle1_close));

    Style secondLineStyle2_open = Style();
    secondLineStyle2_open.m_italic = true;
    expectedSecondLineStyles.emplace_back(TextStyle(8, secondLineStyle2_open));

    Style secondLineStyle2_close = Style();
    secondLineStyle2_close.m_italic = false;
    expectedSecondLineStyles.emplace_back(TextStyle(15, secondLineStyle2_close));

    ASSERT_EQ(splitLine1[0], CaptionLine("The", expectedFirstLineStyles));
    ASSERT_EQ(splitLine1[1], CaptionLine("time is 2:17 PM.", expectedSecondLineStyles));
}

/**
 * Tests that the merge of an empty array returns a valid CaptionLine.
 */
TEST(CaptionLineTest, test_emptySplit) {
    auto merged = CaptionLine::merge({});

    ASSERT_EQ(merged, CaptionLine());
}

/**
 * Tests that the output of the merge of a single style will be equal to the input.
 */
TEST(CaptionLineTest, test_singleStyleMerge) {
    // inputs
    std::vector<TextStyle> firstLineStyles;

    Style firstLineStyle0 = Style();
    firstLineStyles.emplace_back(TextStyle(0, firstLineStyle0));

    auto inputLine = CaptionLine("The time is 2:17 PM.", firstLineStyles);

    // test action
    auto mergedCaptionLines = CaptionLine::merge({inputLine});

    ASSERT_EQ(mergedCaptionLines, inputLine);
}

/**
 * Tests that the output of the merge of a CaptionLine with no style will be equal to the input.
 */
TEST(CaptionLineTest, test_missingStylesMerge) {
    // input
    auto inputLine = CaptionLine("The time is 2:17 PM.", {TextStyle()});

    // test action
    auto mergedCaptionLines = CaptionLine::merge({inputLine});

    ASSERT_EQ(mergedCaptionLines, inputLine);
}

/**
 * Tests that the output of the merge of a single style will be equal to the input.
 */
TEST(CaptionLineTest, test_firstLineMissingStylesMerge) {
    // inputs
    std::vector<TextStyle> firstLineStyles;

    auto inputLine1 = CaptionLine("The time ", firstLineStyles);

    std::vector<TextStyle> secondLineStyles;

    Style secondLineStyle0 = Style();
    secondLineStyle0.m_bold = true;
    secondLineStyles.emplace_back(TextStyle(0, secondLineStyle0));

    auto inputLine2 = CaptionLine("is 2:17 PM.", secondLineStyles);

    // test action
    auto mergedCaptionLines = CaptionLine::merge({inputLine1, inputLine2});

    // expected output
    std::vector<TextStyle> expectedStyles;

    Style s0 = Style();
    expectedStyles.emplace_back(TextStyle(0, s0));

    Style s1 = Style();
    s1.m_bold = true;
    expectedStyles.emplace_back(TextStyle(9, s1));

    CaptionLine expectedLine1 = CaptionLine("The time is 2:17 PM.", expectedStyles);

    ASSERT_EQ(mergedCaptionLines, expectedLine1);
}

/**
 * Tests that the output of the merge of multiple styles will be equal to the joined together input.
 */
TEST(CaptionLineTest, test_multipleStyleMerge) {
    // inputs
    std::vector<TextStyle> firstLineStyles;

    Style firstLineStyle0 = Style();
    firstLineStyles.emplace_back(TextStyle(0, firstLineStyle0));

    std::vector<TextStyle> secondLineStyles;

    Style secondLineStyle1_open = Style();
    secondLineStyle1_open.m_bold = true;
    secondLineStyles.emplace_back(TextStyle(0, secondLineStyle1_open));

    Style secondLineStyle1_close = Style();
    secondLineStyle1_close.m_bold = false;
    secondLineStyles.emplace_back(TextStyle(4, secondLineStyle1_close));

    Style secondLineStyle2_open = Style();
    secondLineStyle2_open.m_italic = true;
    secondLineStyles.emplace_back(TextStyle(8, secondLineStyle2_open));

    Style secondLineStyle2_close = Style();
    secondLineStyle2_close.m_italic = false;
    secondLineStyles.emplace_back(TextStyle(15, secondLineStyle2_close));

    // test action
    auto mergedCaptionLines =
        CaptionLine::merge({CaptionLine("The ", firstLineStyles), CaptionLine("time is 2:17 PM.", secondLineStyles)});

    // expected outputs
    std::vector<TextStyle> expectedStyles;

    Style s0 = Style();
    expectedStyles.emplace_back(TextStyle(0, s0));

    Style s1_open = Style();
    s1_open.m_bold = true;
    expectedStyles.emplace_back(TextStyle(4, s1_open));

    Style s1_close = Style();
    s1_close.m_bold = false;
    expectedStyles.emplace_back(TextStyle(8, s1_close));

    Style s2_open = Style();
    s2_open.m_italic = true;
    expectedStyles.emplace_back(TextStyle(12, s2_open));

    Style s2_close = Style();
    s2_close.m_italic = false;
    expectedStyles.emplace_back(TextStyle(19, s2_close));

    CaptionLine expectedLine1 = CaptionLine("The time is 2:17 PM.", expectedStyles);

    ASSERT_EQ(mergedCaptionLines, expectedLine1);
}

}  // namespace test
}  // namespace captions
}  // namespace alexaClientSDK