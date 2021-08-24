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

#include <iostream>
#include <sstream>

#include "acsdkSampleApplicationCBLAuthRequester/SampleApplicationCBLAuthRequester.h"

namespace alexaClientSDK {
namespace acsdkSampleApplicationCBLAuthRequester {

std::shared_ptr<authorization::cblAuthDelegate::CBLAuthRequesterInterface> SampleApplicationCBLAuthRequester::
    createCBLAuthRequesterInterface(
        const std::shared_ptr<acsdkSampleApplicationInterfaces::UIManagerInterface>& uiManager) {
    if (!uiManager) {
        return nullptr;
    }

    return std::shared_ptr<authorization::cblAuthDelegate::CBLAuthRequesterInterface>(
        new SampleApplicationCBLAuthRequester(uiManager));
}

std::shared_ptr<acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface>
SampleApplicationCBLAuthRequester::createCBLAuthorizationObserverInterface(
    const std::shared_ptr<acsdkSampleApplicationInterfaces::UIManagerInterface>& uiManager) {
    if (!uiManager) {
        return nullptr;
    }

    return std::shared_ptr<acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface>(
        new SampleApplicationCBLAuthRequester(uiManager));
}

void SampleApplicationCBLAuthRequester::onRequestAuthorization(const std::string& url, const std::string& code) {
    m_authCheckCounter = 0;
    m_uiManager->printMessage("NOT YET AUTHORIZED");
    std::ostringstream oss;
    oss << "To authorize, browse to: '" << url << "' and enter the code: " << code;
    m_uiManager->printMessage(oss.str());
}

void SampleApplicationCBLAuthRequester::onCheckingForAuthorization() {
    std::ostringstream oss;
    oss << "Checking for authorization (" << ++m_authCheckCounter << ")...";
    m_uiManager->printMessage(oss.str());
}

void SampleApplicationCBLAuthRequester::onCustomerProfileAvailable(
    const acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface::CustomerProfile& customerProfile) {
    std::ostringstream oss;
    oss << "Name: " << customerProfile.name << " "
        << " Email: " << customerProfile.email;
    m_uiManager->printMessage(oss.str());
}

SampleApplicationCBLAuthRequester::SampleApplicationCBLAuthRequester(
    const std::shared_ptr<acsdkSampleApplicationInterfaces::UIManagerInterface>& uiManager) :
        m_uiManager{uiManager},
        m_authCheckCounter{0} {
}

}  // namespace acsdkSampleApplicationCBLAuthRequester
}  // namespace alexaClientSDK
