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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_REVOKEAUTHORIZATIONOBSERVER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_REVOKEAUTHORIZATIONOBSERVER_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/RevokeAuthorizationObserverInterface.h>
#include <RegistrationManager/RegistrationManagerInterface.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

/**
 * Observes callbacks from revoke authorization requests and notifies the default client.
 */
class RevokeAuthorizationObserver : public avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface {
public:
    /**
     * Constructor.
     *
     * @param manager The registration manager providing logout functionality.
     */
    RevokeAuthorizationObserver(std::shared_ptr<registrationManager::RegistrationManagerInterface> manager);

    /// @name RevokeAuthorizationObserverInterface Functions
    /// @{
    void onRevokeAuthorization() override;
    /// @}

private:
    /// The registration manager used to logout.
    std::shared_ptr<registrationManager::RegistrationManagerInterface> m_manager;
};

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_REVOKEAUTHORIZATIONOBSERVER_H_
