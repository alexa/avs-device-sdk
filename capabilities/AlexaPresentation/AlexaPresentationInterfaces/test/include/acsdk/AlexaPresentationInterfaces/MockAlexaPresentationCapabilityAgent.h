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
#ifndef ACSDK_ALEXAPRESENTATIONINTERFACES_MOCKALEXAPRESENTATIONCAPABILITYAGENT_H_
#define ACSDK_ALEXAPRESENTATIONINTERFACES_MOCKALEXAPRESENTATIONCAPABILITYAGENT_H_

#include <gmock/gmock.h>

#include "acsdk/AlexaPresentationInterfaces/AlexaPresentationCapabilityAgentInterface.h"

namespace alexaClientSDK {
namespace alexaPresentationInterfaces {
namespace test {

/**
 * Mock class that implements AlexaPresentationCAInterface
 */
class MockAlexaPresentationCapabilityAgent
        : public alexaPresentationInterfaces::AlexaPresentationCapabilityAgentInterface {
public:
    MOCK_METHOD1(onPresentationDismissed, void(const aplCapabilityCommon::PresentationToken& token));
};

}  // namespace test
}  // namespace alexaPresentationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAPRESENTATIONINTERFACES_MOCKALEXAPRESENTATIONCAPABILITYAGENT_H_