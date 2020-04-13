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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_MOCKCAPTIONPARSER_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_MOCKCAPTIONPARSER_H_

#include <gtest/gtest.h>

#include <Captions/CaptionParserInterface.h>
#include <Captions/CaptionFrameParseListenerInterface.h>

namespace alexaClientSDK {
namespace captions {
namespace test {

using namespace alexaClientSDK::avsCommon::avs;

class MockCaptionParser : public CaptionParserInterface {
public:
    /// @name CaptionParserInterface methods
    /// @{
    MOCK_METHOD2(parse, void(const CaptionFrame::MediaPlayerSourceId, const CaptionData& captionData));
    MOCK_METHOD1(addListener, void(std::shared_ptr<CaptionFrameParseListenerInterface> parseListener));
    MOCK_METHOD1(releaseResourcesFor, void(CaptionFrame::MediaPlayerSourceId));
    /// }@
};

}  // namespace test
}  // namespace captions
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_MOCKCAPTIONPARSER_H_
