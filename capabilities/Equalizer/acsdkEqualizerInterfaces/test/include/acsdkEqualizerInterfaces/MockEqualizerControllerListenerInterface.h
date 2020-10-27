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

#ifndef ACSDKEQUALIZERINTERFACES_MOCKEQUALIZERCONTROLLERLISTENERINTERFACE_H_
#define ACSDKEQUALIZERINTERFACES_MOCKEQUALIZERCONTROLLERLISTENERINTERFACE_H_

#include <gmock/gmock.h>

#include <acsdkEqualizerInterfaces/EqualizerControllerListenerInterface.h>

namespace alexaClientSDK {
namespace acsdkEqualizerInterfaces {
namespace test {

/**
 * Mock class for @c EqualizerControllerListenerInterface
 */
class MockEqualizerControllerListenerInterface : public EqualizerControllerListenerInterface {
public:
    MOCK_METHOD1(onEqualizerStateChanged, void(const EqualizerState&));
    MOCK_METHOD1(onEqualizerSameStateChanged, void(const EqualizerState&));
};

}  // namespace test
}  // namespace acsdkEqualizerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKEQUALIZERINTERFACES_MOCKEQUALIZERCONTROLLERLISTENERINTERFACE_H_
