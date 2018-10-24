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

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_COMMON_TESTABLEAUTHDELEGATE_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_COMMON_TESTABLEAUTHDELEGATE_H_

#include <string>
#include <unordered_set>

#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

using namespace avsCommon::sdkInterfaces;

/**
 * A test auth delegate that will send back auth tokens and states that you want to test with.
 */
class TestAuthDelegate : public AuthDelegateInterface {
public:
    /**
     * Constructor
     */
    TestAuthDelegate() = default;

    /// @name HttpPutInterface method overrides.
    /// @{
    void addAuthObserver(std::shared_ptr<AuthObserverInterface> observer) override;
    void removeAuthObserver(std::shared_ptr<AuthObserverInterface> observer) override;
    std::string getAuthToken() override;
    void onAuthFailure(const std::string& token) override;
    /// @}

    /**
     * Sets the auth token to be returned.
     *
     * @param authToken The auth token to be returned.
     */
    void setAuthToken(const std::string& authToken);

    /**
     * Sets the auth state.
     *
     * @param authState The auth state of the delegate.
     * @param authError The error associated with the auth state.
     */
    void setAuthState(
        const AuthObserverInterface::State& authState,
        const AuthObserverInterface::Error& authError = AuthObserverInterface::Error::SUCCESS);

private:
    /// The auth token
    std::string m_authToken;

    /// Auth observers
    std::unordered_set<std::shared_ptr<AuthObserverInterface>> m_authObservers;
};

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif /* ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_COMMON_TESTABLEAUTHDELEGATE_H_ */
