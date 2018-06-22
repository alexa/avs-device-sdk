/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKDIRECTIVESEQUENCER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKDIRECTIVESEQUENCER_H_

#include "AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h"
#include <gmock/gmock.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace test {

/// Mock class that implements the DirectiveSequencer.
class MockDirectiveSequencer : public DirectiveSequencerInterface {
public:
    MockDirectiveSequencer();
    MOCK_METHOD1(addDirectiveHandler, bool(std::shared_ptr<DirectiveHandlerInterface> handler));
    MOCK_METHOD1(removeDirectiveHandler, bool(std::shared_ptr<DirectiveHandlerInterface> handler));
    MOCK_METHOD1(setDialogRequestId, void(const std::string& dialogRequestId));
    MOCK_METHOD1(onDirective, bool(std::shared_ptr<avsCommon::avs::AVSDirective> directive));
    MOCK_METHOD0(doShutdown, void());
    MOCK_METHOD0(disable, void());
    MOCK_METHOD0(enable, void());
};

inline MockDirectiveSequencer::MockDirectiveSequencer() :
        avsCommon::sdkInterfaces::DirectiveSequencerInterface{"MockDirectiveSequencer"} {
}

}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKDIRECTIVESEQUENCER_H_
