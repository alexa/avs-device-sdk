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

#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLCAPABILITYAGENTOBSERVER_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLCAPABILITYAGENTOBSERVER_H_

#include <gmock/gmock.h>

#include "acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentObserverInterface.h"

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {
namespace test {

/// Mock of APLCapabilityAgentObserverInterface for testing.
class MockAPLCapabilityAgentObserver : public APLCapabilityAgentObserverInterface {
public:
    MOCK_METHOD2(
        onExecuteCommands,
        void(const std::string& jsonPayload, const aplCapabilityCommonInterfaces::PresentationToken& token));
    MOCK_METHOD10(
        onRenderDocument,
        void(
            const std::string& document,
            const std::string& datasource,
            const aplCapabilityCommonInterfaces::PresentationToken& token,
            const std::string& windowId,
            const aplCapabilityCommonInterfaces::APLTimeoutType timeoutType,
            const std::string& interfaceName,
            const std::string& supportedViewports,
            const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
            const std::chrono::steady_clock::time_point& receiveTime,
            std::shared_ptr<alexaClientSDK::aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> agent));
    MOCK_METHOD3(
        onDataSourceUpdate,
        void(
            const std::string& sourceType,
            const std::string& jsonPayload,
            const aplCapabilityCommonInterfaces::PresentationToken& token));
    MOCK_METHOD1(onShowDocument, void(const aplCapabilityCommonInterfaces::PresentationToken& token));
};

}  // namespace test
}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLCAPABILITYAGENTOBSERVER_H_