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
#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLRUNTIME_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLRUNTIME_H_

#include <gmock/gmock.h>

#include "acsdk/APLCapabilityCommonInterfaces/APLRuntimeInterface.h"

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {
namespace test {

/**
 * Mock class that implements APLRuntimeInterface
 */
class MockAPLRuntime : public APLRuntimeInterface {
public:
    MOCK_METHOD5(
        renderDocument,
        void(
            const std::string& document,
            const std::string& data,
            const PresentationSession& presentationSession,
            const PresentationOptions& presentationOptions,
            std::shared_ptr<APLDocumentObserverInterface> observer));
    MOCK_CONST_METHOD0(getMaxAPLVersion, std::string());
};

}  // namespace test
}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLRUNTIME_H_