/*
 * MockDirectiveSequencer.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ADSL_TEST_ADSL_MOCK_DIRECTIVE_SEQUENCER_H_
#define ALEXA_CLIENT_SDK_ADSL_TEST_ADSL_MOCK_DIRECTIVE_SEQUENCER_H_

#include <AVSCommon/AVSDirective.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>


namespace alexaClientSDK {
namespace adsl {
namespace test {

/**
 * The mock class that implements the DirectiveSequencerInterface.
 */
class MockDirectiveSequencer : public avsCommon::sdkInterfaces::DirectiveSequencerInterface {
public:
    MOCK_METHOD0(shutdown,void());

    MOCK_METHOD1(addDirectiveHandlers, bool(const avsCommon::avs::DirectiveHandlerConfiguration& configuration));

    MOCK_METHOD1(removeDirectiveHandlers, bool(const avsCommon::avs::DirectiveHandlerConfiguration& configuration));

    MOCK_METHOD1(setDialogRequestId, void(const std::string& dialogRequestId));

    MOCK_METHOD1(onDirective, bool(std::shared_ptr<avsCommon::AVSDirective> directive));
};

}  // namespace test
}  // namespace adsl
}  // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ADSL_TEST_ADSL_MOCK_DIRECTIVE_SEQUENCER_H_
