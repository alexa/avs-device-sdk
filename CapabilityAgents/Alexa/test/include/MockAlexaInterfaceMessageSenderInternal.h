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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_TEST_INCLUDE_MOCKALEXAINTERFACEMESSAGESENDERINTERNAL_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_TEST_INCLUDE_MOCKALEXAINTERFACEMESSAGESENDERINTERNAL_H_

#include <Alexa/AlexaInterfaceMessageSenderInternalInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alexa {
namespace test {

/// Mock class that implements the AlexaInterfaceMessageSenderInternalInterface.
class MockAlexaInterfaceMessageSenderInternal : public AlexaInterfaceMessageSenderInternalInterface {
public:
    MOCK_METHOD4(
        sendResponseEvent,
        bool(
            const std::string& instance,
            const std::string& correlationToken,
            const avsCommon::avs::AVSMessageEndpoint& endpoint,
            const std::string& jsonPayload));
    MOCK_METHOD6(
        sendResponseEvent,
        bool(
            const std::string& instance,
            const std::string& correlationToken,
            const avsCommon::avs::AVSMessageEndpoint& endpoint,
            const std::string& responseNamespace,
            const std::string& responseName,
            const std::string& jsonPayload));
    MOCK_METHOD5(
        sendErrorResponseEvent,
        bool(
            const std::string& instance,
            const std::string& correlationToken,
            const avsCommon::avs::AVSMessageEndpoint& endpoint,
            const ErrorResponseType errorType,
            const std::string& errorMessage));
    MOCK_METHOD5(
        sendErrorResponseEvent,
        bool(
            const std::string& instance,
            const std::string& correlationToken,
            const avsCommon::avs::AVSMessageEndpoint& endpoint,
            const std::string& responseNamespace,
            const std::string& payload));
    MOCK_METHOD3(
        sendDeferredResponseEvent,
        bool(const std::string& instance, const std::string& correlationToken, const int estimatedDeferralInSeconds));
    MOCK_METHOD1(
        alexaResponseTypeToErrorType,
        ErrorResponseType(const avsCommon::avs::AlexaResponseType& responseType));
    MOCK_METHOD3(
        sendStateReportEvent,
        bool(
            const std::string& instance,
            const std::string& correlationToken,
            const avsCommon::avs::AVSMessageEndpoint& endpoint));
};

}  // namespace test
}  // namespace alexa
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_TEST_INCLUDE_MOCKALEXAINTERFACEMESSAGESENDERINTERNAL_H_
