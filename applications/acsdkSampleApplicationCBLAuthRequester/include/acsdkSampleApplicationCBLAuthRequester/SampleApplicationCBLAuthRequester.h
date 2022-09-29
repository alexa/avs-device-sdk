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
#include <mutex>
#include <string>

#include <acsdkAuthorizationInterfaces/LWA/CBLAuthorizationObserverInterface.h>
#include <acsdkSampleApplicationInterfaces/UIAuthNotifierInterface.h>
#include <acsdkSampleApplicationInterfaces/UIManagerInterface.h>

namespace alexaClientSDK {
namespace acsdkSampleApplicationCBLAuthRequester {

/**
 * Implementation of CBLAuthRequesterInterface.
 */
class SampleApplicationCBLAuthRequester : public acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface {
public:
    /**
     * Create a new instance of @c CBLAuthorizationObserverInterface.
     *
     * @param uiManager The instance of @c UIManagerInterface to use to message the user.
     * @return A new instance of @c CBLAuthorizationObserverInterface.
     */
    static std::shared_ptr<acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface>
    createCBLAuthorizationObserverInterface(
        const std::shared_ptr<acsdkSampleApplicationInterfaces::UIManagerInterface>& uiManager);

    /// @name CBLAuthorizationObserverInterface methods
    /// @{
    void onRequestAuthorization(const std::string& url, const std::string& code) override;
    void onCheckingForAuthorization() override;
    void onCustomerProfileAvailable(
        const acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface::CustomerProfile& customerProfile)
        override;
    /// @}

    /**
     * Set a notifier that bridges Authorization state from authorization requester to the user interface.
     * @param uiAuthNotifier An instance of @c UIAuthNotifierInterface.
     */
    void setUIAuthNotifier(std::shared_ptr<acsdkSampleApplicationInterfaces::UIAuthNotifierInterface> uiAuthNotifier);

private:
    /**
     * Constructor.
     *
     * @param uiManager The instance of @c UIManagerInterface to use to message the user.
     */
    SampleApplicationCBLAuthRequester(
        const std::shared_ptr<acsdkSampleApplicationInterfaces::UIManagerInterface>& uiManager);

    /// The instance of @c UIManagerInterface used to message the user.
    std::shared_ptr<acsdkSampleApplicationInterfaces::UIManagerInterface> m_uiManager;

    /// The instance of @c UIAuthNotifierInterface used to notify UI about the authorization events.
    std::shared_ptr<acsdkSampleApplicationInterfaces::UIAuthNotifierInterface> m_uiAuthNotifier;

    /// Counter used to make repeated messages about checking for authorization distinguishable from each other.
    int m_authCheckCounter;

    /// URL returned by CBLAuthRequesterInterface for authorization
    std::string m_authUrl;

    /// CBL Code returned by CBLAuthRequesterInterface for authorization
    std::string m_authCode;

    /// Mutex to synchronize rd/wr to the class.
    std::mutex m_mutex;
};

}  // namespace acsdkSampleApplicationCBLAuthRequester
}  // namespace alexaClientSDK

#endif  // ACSDKSAMPLEAPPLICATIONCBLAUTHREQUESTER_SAMPLEAPPLICATIONCBLAUTHREQUESTER_H_
