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

/// @file CaptionTimingAdapterTest.cpp

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <Captions/CaptionTimingAdapter.h>

#include "MockSystemClockDelay.h"
#include "MockCaptionPresenter.h"

namespace alexaClientSDK {
namespace captions {
namespace test {

using namespace ::testing;
using namespace avsCommon::utils;
using namespace avsCommon::avs;
using namespace std::chrono;

/**
 * Test rig.
 */
class CaptionTimingAdapterTest : public ::testing::Test {
public:
    void SetUp() override;

    void TearDown() override;

    /// The system under test.
    std::shared_ptr<CaptionTimingAdapter> m_timingAdapter;

    /// Mock CaptionManager with which to exercise the CaptionParser
    std::shared_ptr<MockCaptionPresenter> m_mockCaptionPresenter;

    /// Mock CaptionManager with which to exercise the CaptionParser
    std::shared_ptr<MockSystemClockDelay> m_mockSystemClockDelay;
};

void CaptionTimingAdapterTest::SetUp() {
    avsCommon::utils::logger::getConsoleLogger()->setLevel(logger::Level::DEBUG9);

    m_mockSystemClockDelay = std::make_shared<NiceMock<MockSystemClockDelay>>();
    m_mockCaptionPresenter = std::make_shared<NiceMock<MockCaptionPresenter>>();

    m_timingAdapter =
        std::shared_ptr<CaptionTimingAdapter>(new CaptionTimingAdapter(m_mockCaptionPresenter, m_mockSystemClockDelay));
}

void CaptionTimingAdapterTest::TearDown() {
}

/**
 * Tests that the queueForDisplay function will eventually notify the presenter.
 */
TEST_F(CaptionTimingAdapterTest, test_queueForDisplayNotifiesPresenter) {
    WaitEvent finishedEvent;
    auto TIMEOUT = milliseconds(50);

    // The input and what is sent to the presenter should remain unchanged
    std::vector<CaptionLine> lines;
    CaptionLine line = CaptionLine("The time is 2:17 PM.", {});
    lines.emplace_back(line);
    auto captionFrame = CaptionFrame(1, milliseconds(1), milliseconds(0), lines);

    EXPECT_CALL(*(m_mockCaptionPresenter.get()), onCaptionActivity(captionFrame, FocusState::FOREGROUND)).Times(1);
    EXPECT_CALL(*(m_mockCaptionPresenter.get()), onCaptionActivity(_, FocusState::NONE))
        .Times(1)
        .WillOnce(InvokeWithoutArgs([&finishedEvent]() { finishedEvent.wakeUp(); }));

    // This is the delay
    EXPECT_CALL(*(m_mockSystemClockDelay.get()), delay(milliseconds(0))).Times(1);

    // This is the duration
    EXPECT_CALL(*(m_mockSystemClockDelay.get()), delay(milliseconds(1))).Times(1);

    m_timingAdapter->queueForDisplay(captionFrame, true);
    EXPECT_TRUE(finishedEvent.wait(TIMEOUT));
}

/**
 * Tests that delays will notify the presenter after honoring the delay period.
 */
TEST_F(CaptionTimingAdapterTest, test_queueForDisplayWithDelay) {
    WaitEvent finishedEvent;
    auto TIMEOUT = milliseconds(50);

    // The input and what is sent to the presenter should remain unchanged
    std::vector<CaptionLine> lines;
    CaptionLine line = CaptionLine("The time is 2:17 PM.", {});
    lines.emplace_back(line);
    auto captionFrame = CaptionFrame(1, milliseconds(10), milliseconds(5), lines);

    EXPECT_CALL(*(m_mockCaptionPresenter.get()), onCaptionActivity(captionFrame, FocusState::FOREGROUND)).Times(1);
    EXPECT_CALL(*(m_mockCaptionPresenter.get()), onCaptionActivity(_, FocusState::NONE))
        .Times(1)
        .WillOnce(InvokeWithoutArgs([&finishedEvent]() { finishedEvent.wakeUp(); }));

    // This is the delay
    EXPECT_CALL(*(m_mockSystemClockDelay.get()), delay(milliseconds(5))).Times(1);

    // This is the duration
    EXPECT_CALL(*(m_mockSystemClockDelay.get()), delay(milliseconds(10))).Times(1);

    m_timingAdapter->queueForDisplay(captionFrame, true);
    EXPECT_TRUE(finishedEvent.wait(TIMEOUT));
}

}  // namespace test
}  // namespace captions
}  // namespace alexaClientSDK