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
#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLDOCUMENTOBSERVER_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLDOCUMENTOBSERVER_H_

#include <gmock/gmock.h>

#include "acsdk/APLCapabilityCommonInterfaces/APLCommandExecutionEvent.h"
#include "acsdk/APLCapabilityCommonInterfaces/APLDocumentObserverInterface.h"

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {
namespace test {

/**
 * Mock class that implements APLDocumentObserverInterface
 */
class MockAPLDocumentObserver : public APLDocumentObserverInterface {
public:
    MOCK_METHOD3(
        onAPLDocumentSessionAvailableTest,
        void(
            const PresentationSession& presentationSession,
            const PresentationToken& token,
            APLDocumentSessionInterface* session));
    void onAPLDocumentSessionAvailable(
        const PresentationSession& presentationSession,
        const PresentationToken& token,
        std::unique_ptr<APLDocumentSessionInterface>&& session) {
        onAPLDocumentSessionAvailableTest(presentationSession, token, session.get());
    }
    MOCK_METHOD1(onDocumentFinished, void(const PresentationToken& token));
    MOCK_METHOD2(
        onActiveDocumentChanged,
        void(
            const PresentationToken& token,
            const alexaClientSDK::aplCapabilityCommonInterfaces::PresentationSession& session));
    MOCK_METHOD4(
        onRenderDocumentComplete,
        void(
            const PresentationToken& token,
            bool result,
            const std::string& error,
            const std::chrono::steady_clock::time_point& timestamp));
    MOCK_METHOD3(
        onCommandExecutionComplete,
        void(const PresentationToken& token, APLCommandExecutionEvent result, const std::string& error));
    MOCK_METHOD3(
        onDataSourceUpdateComplete,
        void(const PresentationToken& token, bool result, const std::string& error));
    MOCK_METHOD2(
        onVisualContextAvailable,
        void(
            const avsCommon::sdkInterfaces::ContextRequestToken requestToken,
            const aplEventPayload::VisualContext& context));
    MOCK_METHOD1(onDataSourceFetch, void(const aplEventPayload::DataSourceFetch& payload));
    MOCK_METHOD1(onRuntimeError, void(const aplEventPayload::RuntimeError& payload));
    MOCK_METHOD1(
        onSessionEnded,
        void(const alexaClientSDK::aplCapabilityCommonInterfaces::PresentationSession& presentationSession));
    MOCK_METHOD2(
        onActivityEvent,
        void(const PresentationToken& token, const avsCommon::sdkInterfaces::GUIActivityEvent& event));
};

}  // namespace test
}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLDOCUMENTOBSERVER_H_