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
#include <gmock/gmock.h>
#include <gmock/gmock-generated-function-mockers.h>
#include "IPCServerSampleApp/RenderCaptionsInterface.h"
#include "IPCServerSampleApp/SmartScreenCaptionPresenter.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace test {

using namespace ::testing;
using namespace captions;

class MockRenderCaptionsInterface : public RenderCaptionsInterface {
public:
    MOCK_METHOD1(renderCaptions, void(const std::string& payload));
};

class SmartScreenCaptionPresenterTest : public ::testing::Test {
public:
    void SetUp() override;

protected:
    std::shared_ptr<StrictMock<MockRenderCaptionsInterface>> m_mockRenderCaptionsInterface;
    std::shared_ptr<SmartScreenCaptionPresenter> m_captionPresenter;
};

void SmartScreenCaptionPresenterTest::SetUp() {
    m_mockRenderCaptionsInterface = std::make_shared<StrictMock<MockRenderCaptionsInterface>>();
    m_captionPresenter = std::make_shared<SmartScreenCaptionPresenter>(m_mockRenderCaptionsInterface);
}

/**
 * Test for empty captions
 */
TEST_F(SmartScreenCaptionPresenterTest, test_renderEmptyCaptions) {
    std::string expectedInput = R"({"duration":0,"delay":0,"captionLines":[]})";
    CaptionFrame captionFrame = CaptionFrame();

    EXPECT_CALL(*m_mockRenderCaptionsInterface, renderCaptions(expectedInput)).Times(Exactly(1));
    m_captionPresenter->onCaptionActivity(captionFrame, avsCommon::avs::FocusState::FOREGROUND);
}

/**
 * Test for Non-Foreground focus state
 */
TEST_F(SmartScreenCaptionPresenterTest, test_renderCaptionsWithBackgroundOrNoneFocusState) {
    CaptionFrame captionFrame = CaptionFrame();

    EXPECT_CALL(*m_mockRenderCaptionsInterface, renderCaptions(_)).Times(Exactly(0));
    m_captionPresenter->onCaptionActivity(captionFrame, avsCommon::avs::FocusState::BACKGROUND);
    m_captionPresenter->onCaptionActivity(captionFrame, avsCommon::avs::FocusState::NONE);
}

/**
 * Test for normal render captions
 */
TEST_F(SmartScreenCaptionPresenterTest, test_renderCaptionsHappyCase) {
    Style style = Style(true, true, true);
    TextStyle textStyle = TextStyle(0, style);
    CaptionLine captionLine = CaptionLine("TestCaptionLine", {textStyle});
    CaptionFrame captionFrame =
        CaptionFrame(0, std::chrono::milliseconds(1000), std::chrono::milliseconds(0), {captionLine});

    std::string expectedPayloadString =
        R"({"duration":1000,"delay":0,"captionLines":[{"text":"TestCaptionLine","styles":[{"activeStyle":{"bold":"1","italic":"1","underline":"1"},"charIndex":"0"}]}]})";
    EXPECT_CALL(*m_mockRenderCaptionsInterface, renderCaptions(expectedPayloadString)).Times(Exactly(1));

    m_captionPresenter->onCaptionActivity(captionFrame, avsCommon::avs::FocusState::FOREGROUND);
}
}  // namespace test
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
