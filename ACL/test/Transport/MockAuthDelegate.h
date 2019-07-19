/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKAUTHDELEGATE_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKAUTHDELEGATE_H_

#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>

#include "AVSCommon/SDKInterfaces/AuthDelegateInterface.h"

#include <gmock/gmock.h>

#include <memory>
#include <string>

namespace alexaClientSDK {
namespace acl {
namespace test {

class MockAuthDelegate : public avsCommon::sdkInterfaces::AuthDelegateInterface {
public:
    /// @name AuthDelegateInterface methods
    /// @{
    MOCK_METHOD1(addAuthObserver, void(std::shared_ptr<avsCommon::sdkInterfaces::AuthObserverInterface>));
    MOCK_METHOD1(removeAuthObserver, void(std::shared_ptr<avsCommon::sdkInterfaces::AuthObserverInterface>));
    std::string getAuthToken();
    MOCK_METHOD1(onAuthFailure, void(const std::string& token));
    /// @}

    /*
     * Set the token string.
     * @param authToken The string to be returned when @c getAuthToken() is called.
     */
    void setAuthToken(std::string authToken);

private:
    /// Holds the token string to be returned when @c getAuthToken() is called.
    std::string m_authToken;
};

std::string MockAuthDelegate::getAuthToken() {
    return m_authToken;
}
void MockAuthDelegate::setAuthToken(std::string authToken) {
    m_authToken = authToken;
}

}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKAUTHDELEGATE_H_
