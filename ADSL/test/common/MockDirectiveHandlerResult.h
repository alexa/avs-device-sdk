/*
 * MockDirectiveHandlerResult.h
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

#ifndef ALEXA_CLIENT_SDK_ADSL_TEST_COMMON_MOCK_DIRECTIVE_HANDLER_RESULT_H_
#define ALEXA_CLIENT_SDK_ADSL_TEST_COMMON_MOCK_DIRECTIVE_HANDLER_RESULT_H_

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/DirectiveHandlerResultInterface.h>

namespace alexaClientSDK {
namespace adsl {
namespace test {

/**
 * MockDirectiveHandlerResult
 */
class MockDirectiveHandlerResult : public avsCommon::sdkInterfaces::DirectiveHandlerResultInterface {
    MOCK_METHOD0(setCompleted, void());
    MOCK_METHOD1(setFailed, void(const std::string&));
};

} // namespace test
} // namespace adsl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ADSL_TEST_COMMON_MOCK_DIRECTIVE_HANDLER_RESULT_H_
