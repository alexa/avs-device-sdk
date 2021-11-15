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

#ifndef AVS_CAPABILITIES_DAVSCLIENT_ACSDKASSETSCOMMON_TEST_MOCKS_AUTHDELEGATEMOCK_H_
#define AVS_CAPABILITIES_DAVSCLIENT_ACSDKASSETSCOMMON_TEST_MOCKS_AUTHDELEGATEMOCK_H_

#include <memory>
#include <mutex>
#include <unordered_set>

#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionObserverInterface.h>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

using namespace alexaClientSDK::avsCommon::sdkInterfaces;
/**
 * Implementation of AuthDelegate that uses MAPLite.
 */
class AuthDelegateMock : public AuthDelegateInterface {
public:
    ~AuthDelegateMock() override = default;

    /**
     * Creates an instance of AuthDelegateInterface to be used with the SDK.
     * @param setupMode Setup Manager used for communication with OOBE
     * @return a new instance of MAPLiteAuthDelegate is created.
     */
    static std::shared_ptr<AuthDelegateMock> create();

    /**
     * @copydoc AuthDelegateInterface::getAuthToken
     */
    std::string getAuthToken() override;

    /**
     * @copydoc AuthDelegateInterface::onAuthFailure
     */
    void onAuthFailure(const std::string& token) override;

    void addAuthObserver(
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface> observer) override;

    void removeAuthObserver(
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface> observer) override;

private:
    /**
     * Constructor.
     */
    AuthDelegateMock() = default;
};

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // AVS_CAPABILITIES_DAVSCLIENT_ACSDKASSETSCOMMON_TEST_MOCKS_AUTHDELEGATEMOCK_H_
