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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_TEST_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_MOCKPRESENTATIONOBSERVER_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_TEST_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_MOCKPRESENTATIONOBSERVER_H_

#include <gmock/gmock.h>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationObserverInterface.h>

namespace alexaClientSDK {
namespace presentationOrchestratorInterfaces {
namespace test {

/**
 * Mock class that implements @c PresentationObserverInterface
 */
class MockPresentationObserver : public presentationOrchestratorInterfaces::PresentationObserverInterface {
public:
    MOCK_METHOD2(
        onPresentationAvailable,
        void(
            presentationOrchestratorInterfaces::PresentationRequestToken id,
            std::shared_ptr<presentationOrchestratorInterfaces::PresentationInterface> presentation));

    MOCK_METHOD2(
        onPresentationStateChanged,
        void(
            presentationOrchestratorInterfaces::PresentationRequestToken id,
            presentationOrchestratorInterfaces::PresentationState newState));

    MOCK_METHOD1(onNavigateBack, bool(presentationOrchestratorInterfaces::PresentationRequestToken id));
};

}  // namespace test
}  // namespace presentationOrchestratorInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_TEST_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_MOCKPRESENTATIONOBSERVER_H_