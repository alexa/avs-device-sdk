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

#include "IPCServerSampleApp/RevokeAuthorizationObserver.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

using namespace alexaClientSDK;

RevokeAuthorizationObserver::RevokeAuthorizationObserver(
    std::shared_ptr<registrationManager::RegistrationManagerInterface> manager) :
        m_manager{manager} {
}

void RevokeAuthorizationObserver::onRevokeAuthorization() {
    if (m_manager) {
        m_manager->logout();
    }
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
