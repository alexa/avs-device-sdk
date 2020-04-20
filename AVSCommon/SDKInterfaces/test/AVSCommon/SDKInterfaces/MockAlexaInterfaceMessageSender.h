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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKALEXAINTERFACEMESSAGESENDER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKALEXAINTERFACEMESSAGESENDER_H_

#include <gmock/gmock.h>

#include "AVSCommon/AVS/AVSMessageEndpoint.h"
#include "AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace test {

/// Mock class that implements the AlexaInterfaceResponseSender.
class MockAlexaInterfaceMessageSender : public AlexaInterfaceMessageSenderInterface {
public:
    MOCK_METHOD4(
        sendResponseEvent,
        bool(
            const std::string& instance,
            const std::string& correlationToken,
            const avsCommon::avs::AVSMessageEndpoint& endpoint,
            const std::string& jsonPayload));
    MOCK_METHOD5(
        sendErrorResponseEvent,
        bool(
            const std::string& instance,
            const std::string& correlationToken,
            const avsCommon::avs::AVSMessageEndpoint& endpoint,
            const ErrorResponseType errorType,
            const std::string& errorMessage));
    MOCK_METHOD3(
        sendDeferredResponseEvent,
        bool(const std::string& instance, const std::string& correlationToken, const int estimatedDeferralInSeconds));
    MOCK_METHOD1(
        alexaResponseTypeToErrorType,
        ErrorResponseType(const avsCommon::avs::AlexaResponseType& responseType));
};

}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKALEXAINTERFACEMESSAGESENDER_H_
