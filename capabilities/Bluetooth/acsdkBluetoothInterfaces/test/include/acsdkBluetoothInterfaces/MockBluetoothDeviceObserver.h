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
#ifndef ACSDKBLUETOOTHINTERFACES_MOCKBLUETOOTHDEVICEOBSERVER_H_
#define ACSDKBLUETOOTHINTERFACES_MOCKBLUETOOTHDEVICEOBSERVER_H_

#include <gmock/gmock.h>

#include "acsdkBluetoothInterfaces/BluetoothDeviceObserverInterface.h"

namespace alexaClientSDK {
namespace acsdkBluetoothInterfaces {
namespace test {

/**
 * Mock class that implements BluetoothDeviceInterface
 */
class MockBluetoothDeviceObserver : public BluetoothDeviceObserverInterface {
public:
    MOCK_METHOD1(onActiveDeviceConnected, void(const DeviceAttributes& attributes));
    MOCK_METHOD1(onActiveDeviceDisconnected, void(const DeviceAttributes& attributes));
};

}  // namespace test
}  // namespace acsdkBluetoothInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKBLUETOOTHINTERFACES_MOCKBLUETOOTHDEVICEOBSERVER_H_
