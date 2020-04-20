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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_MOCKBLUETOOTHSERVICE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_MOCKBLUETOOTHSERVICE_H_

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/Bluetooth/Services/BluetoothServiceInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace bluetooth {
namespace services {
namespace test {

/**
 * Mock class that implements BluetoothServiceInterface
 */
class MockBluetoothService : public BluetoothServiceInterface {
public:
    void setup() override;
    void cleanup() override;
    std::shared_ptr<SDPRecordInterface> getRecord() override;

    /// Constructor.
    MockBluetoothService(std::shared_ptr<SDPRecordInterface> record);

protected:
    std::shared_ptr<SDPRecordInterface> m_record;
};

inline std::shared_ptr<SDPRecordInterface> MockBluetoothService::getRecord() {
    return m_record;
}

MockBluetoothService::MockBluetoothService(std::shared_ptr<SDPRecordInterface> record) : m_record{record} {
}

void MockBluetoothService::setup() {
    // no-op
}

void MockBluetoothService::cleanup() {
    // no op
}

}  // namespace test
}  // namespace services
}  // namespace bluetooth
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_BLUETOOTH_SERVICES_MOCKBLUETOOTHSERVICE_H_