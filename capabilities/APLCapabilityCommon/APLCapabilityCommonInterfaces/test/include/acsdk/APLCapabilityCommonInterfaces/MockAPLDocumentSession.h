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
#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLDOCUMENTSESSION_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLDOCUMENTSESSION_H_

#include <gmock/gmock.h>

#include "acsdk/APLCapabilityCommonInterfaces/APLDocumentSessionInterface.h"

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {
namespace test {

/**
 * Mock class that implements APLDocumentSessionInterface
 */
class MockAPLDocumentSession : public APLDocumentSessionInterface {
public:
    MOCK_METHOD0(clearDocument, void());
    MOCK_METHOD1(executeCommands, void(const std::string& commands));
    MOCK_METHOD2(dataSourceUpdate, void(const std::string& sourceType, const std::string& payload));
    MOCK_METHOD0(interruptCommandSequence, void());
    MOCK_METHOD1(provideDocumentContext, void(const avsCommon::sdkInterfaces::ContextRequestToken));
    MOCK_METHOD0(requestForeground, void());
    MOCK_METHOD1(updateTimeout, void(std::chrono::milliseconds timeout));
    MOCK_METHOD0(stopTimeout, void());
    MOCK_METHOD0(resetTimeout, void());
    MOCK_METHOD1(updateLifespan, void(presentationOrchestratorInterfaces::PresentationLifespan lifespan));
    MOCK_CONST_METHOD0(getToken, PresentationToken());
    MOCK_METHOD0(isForegroundFocused, bool());
};

}  // namespace test
}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKAPLDOCUMENTSESSION_H_