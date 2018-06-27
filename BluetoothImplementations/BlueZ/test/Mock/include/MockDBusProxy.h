/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_TEST_MOCK_INCLUDE_MOCKDBUSPROXY_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_TEST_MOCK_INCLUDE_MOCKDBUSPROXY_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "BlueZ/BlueZUtils.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {
namespace test {

using namespace ::testing;

/**
 * A mock class of @c DBusProxy.
 */
class MockDBusProxy : public DBusProxy {
public:
    MockDBusProxy() : DBusProxy{nullptr, ""} {};

    MOCK_METHOD2(create, std::shared_ptr<DBusProxy>(const std::string&, const std::string&));
    MOCK_METHOD3(callMethod, ManagedGVariant(const std::string&, GVariant*, GError**));
    MOCK_METHOD4(callMethodWithFDList, ManagedGVariant(const std::string&, GVariant*, GUnixFDList**, GError**));
    MOCK_METHOD0(getObjectPath, std::string());
    MOCK_METHOD0(get, GDBusProxy*());
};

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_TEST_MOCK_INCLUDE_MOCKDBUSPROXY_H_

}  // namespace test
}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
