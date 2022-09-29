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
#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLCAPABILITYAGENT_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLCAPABILITYAGENT_H_

#include <gmock/gmock.h>

#include "acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentInterface.h"

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {
namespace test {

/**
 * Mock class that implements APLCapabilityAgentInterface
 */
class MockAPLCapabilityAgent : public APLCapabilityAgentInterface {
public:
    MOCK_METHOD2(
        onActiveDocumentChanged,
        void(
            const aplCapabilityCommonInterfaces::PresentationToken& token,
            const alexaClientSDK::aplCapabilityCommonInterfaces::PresentationSession& session));
    MOCK_METHOD2(
        clearExecuteCommands,
        void(const aplCapabilityCommonInterfaces::PresentationToken& token, const bool markAsFailed));
    MOCK_METHOD1(sendUserEvent, void(const aplEventPayload::UserEvent& payload));
    MOCK_METHOD1(sendDataSourceFetchRequestEvent, void(const aplEventPayload::DataSourceFetch& payload));
    MOCK_METHOD1(sendRuntimeErrorEvent, void(const aplEventPayload::RuntimeError& payload));
    MOCK_METHOD2(
        onVisualContextAvailable,
        void(
            const avsCommon::sdkInterfaces::ContextRequestToken requestToken,
            const aplEventPayload::VisualContext& visualContext));
    MOCK_METHOD3(
        processRenderDocumentResult,
        void(const PresentationToken& token, const bool result, const std::string& error));
    MOCK_METHOD3(
        processExecuteCommandsResult,
        void(const PresentationToken& token, APLCommandExecutionEvent event, const std::string& error));
    MOCK_METHOD1(recordRenderComplete, void(const std::chrono::steady_clock::time_point& timestamp));
    MOCK_METHOD0(proactiveStateReport, void());
};

}  // namespace test
}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLCAPABILITYAGENT_H_