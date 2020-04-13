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

#include <memory>
#include <string>

#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_MOCKAUTHDELEGATE_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_MOCKAUTHDELEGATE_H_

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

/**
 * A Mock class for @c AuthDelegateInterface.
 */
class MockAuthDelegate : public avsCommon::sdkInterfaces::AuthDelegateInterface {
public:
    MOCK_METHOD1(addAuthObserver, void(std::shared_ptr<avsCommon::sdkInterfaces::AuthObserverInterface>));
    MOCK_METHOD1(removeAuthObserver, void(std::shared_ptr<avsCommon::sdkInterfaces::AuthObserverInterface>));
    MOCK_METHOD0(getAuthToken, std::string());
    MOCK_METHOD1(onAuthFailure, void(const std::string&));
};

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_MOCKAUTHDELEGATE_H_
