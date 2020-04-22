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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_TESTTIMINGADAPTERFACTORY_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_TESTTIMINGADAPTERFACTORY_H_

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MockCaptionTimingAdapter.h"
#include "MockSystemClockDelay.h"

namespace alexaClientSDK {
namespace captions {
namespace test {

using namespace ::testing;

/**
 * A concrete implementation of TimingAdapterFactory that returns a MockCaptionTimingAdapter that is known ahead of time
 * for testing purposes.
 */
class TestTimingAdapterFactory : public TimingAdapterFactory {
public:
    TestTimingAdapterFactory() : TimingAdapterFactory{std::shared_ptr<NiceMock<MockSystemClockDelay>>()} {
    }

    ~TestTimingAdapterFactory();

    /// @name CaptionParserInterface methods
    /// @{
    std::shared_ptr<CaptionTimingAdapterInterface> getTimingAdapter(
        std::shared_ptr<CaptionPresenterInterface> presenter) const;
    ///@}

    /**
     * Getter for the mock timing adapter that would be returned by the TimingAdapterFactory instance. If the mock
     * adapter is null, then one is created and then returned. This function is provided so that the EXPECT_CALL can be
     * used on the mock that is later going to be used by the @c getTimingAdapter function.
     *
     * @return The mock timing adapter.
     */
    std::shared_ptr<MockCaptionTimingAdapter> getMockTimingAdapter();

private:
    /// The @c MockCaptionTimingAdapter to be used with the @c CaptionParserInterface method.
    std::shared_ptr<NiceMock<MockCaptionTimingAdapter>> m_timingAdapter;
};

}  // namespace test
}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_TESTTIMINGADAPTERFACTORY_H_
