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

#ifndef ACSDKEQUALIZERINTERFACES_MOCKEQUALIZERINTERFACE_H_
#define ACSDKEQUALIZERINTERFACES_MOCKEQUALIZERINTERFACE_H_

#include <gmock/gmock.h>

#include <acsdkEqualizerInterfaces/EqualizerInterface.h>

namespace alexaClientSDK {
namespace acsdkEqualizerInterfaces {
namespace test {

/**
 * Mock class for @c EqualizerInterface
 */
class MockEqualizerInterface : public EqualizerInterface {
public:
    MOCK_METHOD1(setEqualizerBandLevels, void(EqualizerBandLevelMap));
    MOCK_METHOD0(getMinimumBandLevel, int());
    MOCK_METHOD0(getMaximumBandLevel, int());
};

}  // namespace test
}  // namespace acsdkEqualizerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKEQUALIZERINTERFACES_MOCKEQUALIZERINTERFACE_H_
