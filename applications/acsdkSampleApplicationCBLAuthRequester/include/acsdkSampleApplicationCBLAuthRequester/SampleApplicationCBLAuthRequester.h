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

#ifndef ACSDKSAMPLEAPPLICATIONCBLAUTHREQUESTER_SAMPLEAPPLICATIONCBLAUTHREQUESTER_H_
#define ACSDKSAMPLEAPPLICATIONCBLAUTHREQUESTER_SAMPLEAPPLICATIONCBLAUTHREQUESTER_H_

#include <memory>

#include <acsdkSampleApplicationInterfaces/UIManagerInterface.h>
#include <CBLAuthDelegate/CBLAuthRequesterInterface.h>
#include <acsdkAuthorizationInterfaces/LWA/CBLAuthorizationObserverInterface.h>

namespace alexaClientSDK {
namespace acsdkSampleApplicationCBLAuthRequester {

/**
 * Implementation of CBLAuthRequesterInterface.
 */
class SampleApplicationCBLAuthRequester
        : public authorization::cblAuthDelegate::CBLAuthRequesterInterface
        , public acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface {
public:
    /**
     * Create a new instance of @c CBLAuthRequesterInterface.
     *
     * @param uiManager The instance of @c UIManagerInterface to use to message the user.
     * @return A new instance of @c CBLAuthRequesterInterface.
     */
    static std::shared_ptr<CBLAuthRequesterInterface> createCBLAuthRequesterInterface(
        const std::shared_ptr<acsdkSampleApplicationInterfaces::UIManagerInterface>& uiManager);

    /**
     * Create a new instance of @c CBLAuthorizationObserverInterface.
     *
     * @param uiManager The instance of @c UIManagerInterface to use to message the user.
     * @return A new instance of @c CBLAuthorizationObserverInterface.
     */
    static std::shared_ptr<acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface>
    createCBLAuthorizationObserverInterface(
        const std::shared_ptr<acsdkSampleApplicationInterfaces::UIManagerInterface>& uiManager);

    /// @name CBLAuthDelegateRequester methods
    /// @{
    void onRequestAuthorization(const std::string& url, const std::string& code) override;
    void onCheckingForAuthorization() override;
    /// @}

    /// @name CBLAuthorizationObserverInterface methods
    /// @{
    void onCustomerProfileAvailable(
        const acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface::CustomerProfile& customerProfile)
        override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param uiManager The instance of @c UIManagerInterface to use to message the user.
     */
    SampleApplicationCBLAuthRequester(
        const std::shared_ptr<acsdkSampleApplicationInterfaces::UIManagerInterface>& uiManager);

    /// The instance of @c UIManagerInterface to use to message the user.
    std::shared_ptr<acsdkSampleApplicationInterfaces::UIManagerInterface> m_uiManager;

    /// Counter used to make repeated messages about checking for authorization distinguishable from each other.
    int m_authCheckCounter;
};

}  // namespace acsdkSampleApplicationCBLAuthRequester
}  // namespace alexaClientSDK

#endif  // ACSDKSAMPLEAPPLICATIONCBLAUTHREQUESTER_SAMPLEAPPLICATIONCBLAUTHREQUESTER_H_
