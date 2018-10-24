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

#include "TestableAuthDelegate.h"

#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

using namespace avsCommon::sdkInterfaces;

void TestAuthDelegate::addAuthObserver(std::shared_ptr<AuthObserverInterface> observer) {
    if (observer) {
        m_authObservers.insert(observer);
    }
}

void TestAuthDelegate::removeAuthObserver(std::shared_ptr<AuthObserverInterface> observer) {
    if (observer) {
        m_authObservers.erase(observer);
    }
}

std::string TestAuthDelegate::getAuthToken() {
    return m_authToken;
}

void TestAuthDelegate::onAuthFailure(const std::string& token) {
}

void TestAuthDelegate::setAuthToken(const std::string& authToken) {
    m_authToken = authToken;
}

void TestAuthDelegate::setAuthState(
    const AuthObserverInterface::State& authState,
    const AuthObserverInterface::Error& authError) {
    for (auto observer : m_authObservers) {
        observer->onAuthStateChange(authState, authError);
    }
}

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
