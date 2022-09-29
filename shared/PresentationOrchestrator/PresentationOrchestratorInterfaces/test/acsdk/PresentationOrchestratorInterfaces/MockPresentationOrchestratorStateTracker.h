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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_TEST_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_MOCKPRESENTATIONORCHESTRATORSTATETRACKER_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_TEST_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_MOCKPRESENTATIONORCHESTRATORSTATETRACKER_H_

#include <gmock/gmock.h>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateTrackerInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorWindowObserverInterface.h>

namespace alexaClientSDK {
namespace presentationOrchestratorInterfaces {
namespace test {

/**
 * Mock class that implements @c PresentationOrchestratorStateTrackerInterface
 */
class MockPresentationOrchestratorStateTracker
        : public presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface {
public:
    MOCK_METHOD3(
        acquireWindow,
        void(
            const std::string& clientId,
            const std::string& windowId,
            presentationOrchestratorInterfaces::PresentationMetadata metadata));

    MOCK_METHOD3(
        updatePresentationMetadata,
        void(
            const std::string& clientId,
            const std::string& windowId,
            presentationOrchestratorInterfaces::PresentationMetadata metadata));

    MOCK_METHOD2(releaseWindow, void(const std::string& clientId, const std::string& windowId));

    MOCK_METHOD1(setDeviceInterface, void(std::string interface));

    MOCK_METHOD0(releaseDeviceInterface, void());

    MOCK_METHOD0(getFocusedInterface, std::string());

    MOCK_METHOD0(getFocusedWindowId, std::string());

    MOCK_METHOD1(
        setWindows,
        void(const std::vector<presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance>& windows));

    MOCK_METHOD1(
        addWindow,
        bool(const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& window));

    MOCK_METHOD1(removeWindow, void(const std::string& windowId));

    MOCK_METHOD1(
        updateWindow,
        void(const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& window));

    MOCK_METHOD0(
        getWindowInformation,
        std::vector<presentationOrchestratorInterfaces::PresentationOrchestratorWindowInfo>());

    MOCK_METHOD1(
        addWindowObserver,
        void(std::weak_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorWindowObserverInterface>
                 observer));

    MOCK_METHOD1(
        removeWindowObserver,
        void(std::weak_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorWindowObserverInterface>
                 observer));

    MOCK_METHOD1(
        addStateObserver,
        void(std::weak_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateObserverInterface>
                 observer));

    MOCK_METHOD1(
        removeStateObserver,
        void(std::weak_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateObserverInterface>
                 obserevr));
};

}  // namespace test
}  // namespace presentationOrchestratorInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_TEST_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_MOCKPRESENTATIONORCHESTRATORSTATETRACKER_H_