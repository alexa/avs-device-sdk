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

/// @file LibwebvttParserAdapterTest.cpp

#include <gtest/gtest.h>
#include <chrono>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h>
#include <Captions/CaptionData.h>
#include <Captions/CaptionFormat.h>
#include <Captions/CaptionFrame.h>
#include <Captions/LibwebvttParserAdapter.h>
#include <Captions/TextStyle.h>

#include "MockCaptionManager.h"

namespace alexaClientSDK {
namespace captions {
namespace test {

using namespace ::testing;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::utils;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::utils::mediaPlayer::test;
using namespace std::chrono;

#ifdef ENABLE_CAPTIONS

/**
 * Test rig.
 */
class LibwebvttParserAdapterTest : public ::testing::Test {
public:
    void SetUp() override;

    void TearDown() override;

    /// The system under test.
    std::shared_ptr<CaptionParserInterface> m_libwebvttParser;

    /// Mock CaptionManager with which to exercise the CaptionParser
    std::shared_ptr<MockCaptionManager> m_mockCaptionManager;
};

void LibwebvttParserAdapterTest::SetUp() {
    avsCommon::utils::logger::getConsoleLogger()->setLevel(logger::Level::DEBUG9);

    m_libwebvttParser = LibwebvttParserAdapter::getInstance();

    m_mockCaptionManager = std::make_shared<NiceMock<MockCaptionManager>>();
}

void LibwebvttParserAdapterTest::TearDown() {
    m_libwebvttParser->addListener(nullptr);
}

/**
 * Test that parse does not call onParsed with malformed WebVTT header.
 */
TEST_F(LibwebvttParserAdapterTest, test_createWithNullArgs) {
    m_libwebvttParser->addListener(m_mockCaptionManager);
    EXPECT_CALL(*(m_mockCaptionManager.get()), onParsed(_)).Times(0);

    const CaptionData inputData = CaptionData(CaptionFormat::WEBVTT, "");
    m_libwebvttParser->parse(0, inputData);
    m_libwebvttParser->releaseResourcesFor(0);
}

/**
 * Test that parse succeeds for a single, sane caption data and returns the same captionId back to the listener.
 */
TEST_F(LibwebvttParserAdapterTest, test_parseSingleCaptionFrame) {
    m_libwebvttParser->addListener(m_mockCaptionManager);
    std::vector<TextStyle> expectedStyles;

    Style s0 = Style();
    expectedStyles.emplace_back(TextStyle{0, s0});

    std::vector<CaptionLine> expectedCaptionLines;
    expectedCaptionLines.emplace_back(CaptionLine{"The time is 2:17 PM.", expectedStyles});
    CaptionFrame expectedCaptionFrame = CaptionFrame(123, milliseconds(1260), milliseconds(0), expectedCaptionLines);

    EXPECT_CALL(*(m_mockCaptionManager.get()), onParsed(expectedCaptionFrame)).Times(1);

    const std::string webvttContent =
        "WEBVTT\n"
        "\n"
        "1\n"
        "00:00.000 --> 00:01.260\n"
        "The time is 2:17 PM.";
    const CaptionData inputData = CaptionData(CaptionFormat::WEBVTT, webvttContent);
    m_libwebvttParser->parse(123, inputData);
    m_libwebvttParser->releaseResourcesFor(123);
}

/**
 * Test that parse succeeds for multiple, sane caption data and returns the appropriate captionIds back to the
 * listener, along with the correct caption frame.
 */
TEST_F(LibwebvttParserAdapterTest, test_parseMultipleCaptionFrames) {
    m_libwebvttParser->addListener(m_mockCaptionManager);

    // Expected frame #1
    std::vector<TextStyle> frame1_expectedStyles;
    frame1_expectedStyles.emplace_back(TextStyle{0, Style()});

    std::vector<CaptionLine> frame1_expectedCaptionLines;
    frame1_expectedCaptionLines.emplace_back(CaptionLine{"The time is 2:17 PM.", frame1_expectedStyles});
    CaptionFrame frame1_expectedCaptionFrame =
        CaptionFrame(101, milliseconds(1260), milliseconds(0), frame1_expectedCaptionLines);

    // Expected frame #2
    std::vector<TextStyle> frame2_expectedStyles;
    frame2_expectedStyles.emplace_back(TextStyle{0, Style()});

    std::vector<CaptionLine> frame2_expectedCaptionLines;
    frame2_expectedCaptionLines.emplace_back(CaptionLine{"Never drink liquid nitrogen.", frame2_expectedStyles});
    CaptionFrame frame2_expectedCaptionFrame =
        CaptionFrame(102, milliseconds(3000), milliseconds(1000), frame2_expectedCaptionLines);

    EXPECT_CALL(*(m_mockCaptionManager.get()), onParsed(frame1_expectedCaptionFrame)).Times(1);
    EXPECT_CALL(*(m_mockCaptionManager.get()), onParsed(frame2_expectedCaptionFrame)).Times(1);

    const std::string frame1_webvttContent =
        "WEBVTT\n"
        "\n"
        "1\n"
        "00:00.000 --> 00:01.260\n"
        "The time is 2:17 PM.";
    const CaptionData frame1_inputData = CaptionData(CaptionFormat::WEBVTT, frame1_webvttContent);

    const std::string frame2_webvttContent =
        "WEBVTT\n"
        "\n"
        "00:01.000 --> 00:04.000\n"
        "Never drink liquid nitrogen.";
    const CaptionData frame2_inputData = CaptionData(CaptionFormat::WEBVTT, frame2_webvttContent);

    m_libwebvttParser->parse(101, frame1_inputData);
    m_libwebvttParser->parse(102, frame2_inputData);
    m_libwebvttParser->releaseResourcesFor(101);
    m_libwebvttParser->releaseResourcesFor(102);
}

/**
 * Test that parse succeeds for a single, sane caption data and returns multiple caption frames, both with the same ID.
 */
TEST_F(LibwebvttParserAdapterTest, test_parseSingleCaptionDataToCaptionFrames) {
    m_libwebvttParser->addListener(m_mockCaptionManager);

    // Expected frame #1
    std::vector<CaptionLine> frame1_expectedCaptionLines;
    frame1_expectedCaptionLines.emplace_back(CaptionLine{"Never drink liquid nitrogen.", {TextStyle{0, Style()}}});
    CaptionFrame frame1_expectedCaptionFrame =
        CaptionFrame(101, milliseconds(3000), milliseconds(0), frame1_expectedCaptionLines);

    // Expected frame #2
    std::vector<CaptionLine> frame2_expectedCaptionLines;
    frame2_expectedCaptionLines.emplace_back(CaptionLine{"- It will perforate your stomach.", {TextStyle{0, Style()}}});
    frame2_expectedCaptionLines.emplace_back(CaptionLine{"- You could die.", {TextStyle{0, Style()}}});
    CaptionFrame frame2_expectedCaptionFrame =
        CaptionFrame(101, milliseconds(4000), milliseconds(0), frame2_expectedCaptionLines);

    EXPECT_CALL(*(m_mockCaptionManager.get()), onParsed(frame1_expectedCaptionFrame)).Times(1);
    EXPECT_CALL(*(m_mockCaptionManager.get()), onParsed(frame2_expectedCaptionFrame)).Times(1);

    const std::string webvttContent =
        "WEBVTT\n"
        "\n"
        "00:00.000 --> 00:03.000\n"
        "Never drink liquid nitrogen.\n"
        "\n"
        "00:03.000 --> 00:07.000\n"
        "- It will perforate your stomach.\n"
        "- You could die.";
    const CaptionData inputData = CaptionData(CaptionFormat::WEBVTT, webvttContent);

    m_libwebvttParser->parse(101, inputData);
    m_libwebvttParser->releaseResourcesFor(101);
}

/**
 * Test that parse honors a time gap between two caption frames.
 */
TEST_F(LibwebvttParserAdapterTest, test_parseDelayBetweenCaptionFrames) {
    m_libwebvttParser->addListener(m_mockCaptionManager);

    // Expected frame #1
    std::vector<CaptionLine> frame1_expectedCaptionLines;
    frame1_expectedCaptionLines.emplace_back(CaptionLine{"Never drink liquid nitrogen.", {TextStyle{0, Style()}}});
    CaptionFrame frame1_expectedCaptionFrame =
        CaptionFrame(101, milliseconds(3000), milliseconds(1000), frame1_expectedCaptionLines);

    // Expected frame #2
    std::vector<CaptionLine> frame2_expectedCaptionLines;
    frame2_expectedCaptionLines.emplace_back(CaptionLine{"- It will perforate your stomach.", {TextStyle{0, Style()}}});
    frame2_expectedCaptionLines.emplace_back(CaptionLine{"- You could die.", {TextStyle{0, Style()}}});
    CaptionFrame frame2_expectedCaptionFrame =
        CaptionFrame(101, milliseconds(4000), milliseconds(1000), frame2_expectedCaptionLines);

    EXPECT_CALL(*(m_mockCaptionManager.get()), onParsed(frame1_expectedCaptionFrame)).Times(1);
    EXPECT_CALL(*(m_mockCaptionManager.get()), onParsed(frame2_expectedCaptionFrame)).Times(1);

    const std::string webvttContent =
        "WEBVTT\n"
        "\n"
        "00:01.000 --> 00:04.000\n"
        "Never drink liquid nitrogen.\n"
        "\n"
        "00:05.000 --> 00:09.000\n"
        "- It will perforate your stomach.\n"
        "- You could die.";
    const CaptionData inputData = CaptionData(CaptionFormat::WEBVTT, webvttContent);

    m_libwebvttParser->parse(101, inputData);
    m_libwebvttParser->releaseResourcesFor(101);
}

/**
 * Test that parse converts the bold tag to bold style.
 */
TEST_F(LibwebvttParserAdapterTest, test_parseBoldTagToStyle) {
    m_libwebvttParser->addListener(m_mockCaptionManager);

    std::vector<TextStyle> expectedStyles;

    Style s0 = Style();
    expectedStyles.emplace_back(TextStyle{0, s0});

    Style s1 = Style();
    s1.m_bold = true;
    expectedStyles.emplace_back(TextStyle{4, s1});

    Style s2 = Style();
    s2.m_bold = false;
    expectedStyles.emplace_back(TextStyle{8, s2});

    std::vector<CaptionLine> expectedCaptionLines;
    expectedCaptionLines.emplace_back(CaptionLine{"The time is 2:17 PM.", expectedStyles});
    CaptionFrame expectedCaptionFrame = CaptionFrame(123, milliseconds(1260), milliseconds(0), expectedCaptionLines);

    EXPECT_CALL(*(m_mockCaptionManager.get()), onParsed(expectedCaptionFrame)).Times(1);

    const std::string webvttContent =
        "WEBVTT\n"
        "\n"
        "1\n"
        "00:00.000 --> 00:01.260\n"
        "The <b>time</b> is 2:17 PM.";
    const CaptionData inputData = CaptionData(CaptionFormat::WEBVTT, webvttContent);
    m_libwebvttParser->parse(123, inputData);
    m_libwebvttParser->releaseResourcesFor(123);
}

/**
 * Test that parse converts the italic tag to italic style.
 */
TEST_F(LibwebvttParserAdapterTest, test_parseItalicTagToStyle) {
    m_libwebvttParser->addListener(m_mockCaptionManager);

    std::vector<TextStyle> expectedStyles;

    Style s0 = Style();
    expectedStyles.emplace_back(TextStyle{0, s0});

    Style s1 = Style();
    s1.m_italic = true;
    expectedStyles.emplace_back(TextStyle{4, s1});

    Style s2 = Style();
    s2.m_italic = false;
    expectedStyles.emplace_back(TextStyle{8, s2});

    std::vector<CaptionLine> expectedCaptionLines;
    expectedCaptionLines.emplace_back(CaptionLine{"The time is 2:17 PM.", expectedStyles});
    CaptionFrame expectedCaptionFrame = CaptionFrame(123, milliseconds(1260), milliseconds(0), expectedCaptionLines);

    EXPECT_CALL(*(m_mockCaptionManager.get()), onParsed(expectedCaptionFrame)).Times(1);

    const std::string webvttContent =
        "WEBVTT\n"
        "\n"
        "1\n"
        "00:00.000 --> 00:01.260\n"
        "The <i>time</i> is 2:17 PM.";
    const CaptionData inputData = CaptionData(CaptionFormat::WEBVTT, webvttContent);
    m_libwebvttParser->parse(123, inputData);
    m_libwebvttParser->releaseResourcesFor(123);
}
#endif

}  // namespace test
}  // namespace captions
}  // namespace alexaClientSDK