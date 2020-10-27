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

#ifndef ACSDKEQUALIZERINTERFACES_MOCKEQUALIZERCONFIGURATIONINTERFACE_H_
#define ACSDKEQUALIZERINTERFACES_MOCKEQUALIZERCONFIGURATIONINTERFACE_H_

#include <gmock/gmock.h>

#include <set>

#include <acsdkEqualizerInterfaces/EqualizerConfigurationInterface.h>

namespace alexaClientSDK {
namespace acsdkEqualizerInterfaces {
namespace test {

/**
 * Mock class for @c EqualizerConfigurationInterface
 */
class MockEqualizerConfigurationInterface : public EqualizerConfigurationInterface {
public:
    MOCK_CONST_METHOD0(getSupportedBands, std::set<EqualizerBand>());
    MOCK_CONST_METHOD0(getSupportedModes, std::set<EqualizerMode>());
    MOCK_CONST_METHOD0(getMinBandLevel, int());
    MOCK_CONST_METHOD0(getMaxBandLevel, int());
    MOCK_CONST_METHOD0(getDefaultBandDelta, int());
    MOCK_CONST_METHOD0(getDefaultState, EqualizerState());
    MOCK_CONST_METHOD1(isBandSupported, bool(EqualizerBand));
    MOCK_CONST_METHOD1(isModeSupported, bool(EqualizerMode));
};

}  // namespace test
}  // namespace acsdkEqualizerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKEQUALIZERINTERFACES_MOCKEQUALIZERCONFIGURATIONINTERFACE_H_
