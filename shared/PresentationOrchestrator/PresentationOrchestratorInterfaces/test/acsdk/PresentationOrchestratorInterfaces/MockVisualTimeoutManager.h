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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_TEST_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_MOCKVISUALTIMEOUTMANAGER_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_TEST_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_MOCKVISUALTIMEOUTMANAGER_H_

#include <gmock/gmock.h>

#include <acsdk/PresentationOrchestratorInterfaces/VisualTimeoutManagerInterface.h>
#include <AVSCommon/SDKInterfaces/GUIActivityEvent.h>

namespace alexaClientSDK {
namespace presentationOrchestratorInterfaces {
namespace test {

/**
 * Mock class that implements @c VisualTimeoutManagerInterface
 */
class MockVisualTimeoutManager : public presentationOrchestratorInterfaces::VisualTimeoutManagerInterface {
public:
    MOCK_METHOD2(
        requestTimeout,
        VisualTimeoutId(std::chrono::milliseconds delay, VisualTimeoutCallback timeoutCallback));

    MOCK_METHOD1(stopTimeout, bool(VisualTimeoutId timeoutId));

    MOCK_METHOD1(onDialogUXStateChanged, void(DialogUXStateObserverInterface::DialogUXState newState));

    MOCK_METHOD2(
        onGUIActivityEventReceived,
        void(const std::string& source, const avsCommon::sdkInterfaces::GUIActivityEvent& activityEvent));
};

}  // namespace test
}  // namespace presentationOrchestratorInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_TEST_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_MOCKVISUALTIMEOUTMANAGER_H_