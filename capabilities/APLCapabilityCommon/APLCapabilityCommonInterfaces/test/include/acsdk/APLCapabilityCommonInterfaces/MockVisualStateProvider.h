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
#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKVISUALSTATEPROVIDER_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKVISUALSTATEPROVIDER_H_

#include <gmock/gmock.h>

#include "acsdk/APLCapabilityCommonInterfaces/VisualStateProviderInterface.h"

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {
namespace test {

/**
 * Mock of VisualStateProviderInterface for testing.
 */
class MockVisualStateProvider : public VisualStateProviderInterface {
public:
    MOCK_METHOD2(
        provideState,
        void(const PresentationToken& token, const avsCommon::sdkInterfaces::ContextRequestToken stateRequestToken));
};

}  // namespace test
}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_MOCKVISUALSTATEPROVIDER_H_