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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_MOCKCAPTIONPRESENTER_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_MOCKCAPTIONPRESENTER_H_

#include <gmock/gmock.h>

#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>

#include <Captions/CaptionPresenterInterface.h>

namespace alexaClientSDK {
namespace captions {
namespace test {

using namespace ::testing;

/**
 * A mock CaptionPresenter for validating client-side expectations.
 */
class MockCaptionPresenter : public CaptionPresenterInterface {
public:
    /// @name CaptionPresenterInterface methods
    /// @{
    MOCK_METHOD2(onCaptionActivity, void(const CaptionFrame&, avsCommon::avs::FocusState));
    MOCK_METHOD1(getWrapIndex, std::pair<bool, int>(const CaptionLine&));
    /// }@
};

}  // namespace test
}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_MOCKCAPTIONPRESENTER_H_