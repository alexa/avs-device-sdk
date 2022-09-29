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
#include <acsdkAuthorizationInterfaces/AuthorizationManagerInterface.h>

namespace alexaClientSDK {
namespace acsdkSampleApplicationCBLAuthRequester {

using namespace acsdkSampleApplicationInterfaces;
using namespace acsdkAuthorizationInterfaces::lwa;

std::shared_ptr<CBLAuthorizationObserverInterface> SampleApplicationCBLAuthRequester::
    createCBLAuthorizationObserverInterface(
        const std::shared_ptr<acsdkSampleApplicationInterfaces::UIManagerInterface>& uiManager) {
    if (!uiManager) {
        return nullptr;
    }

    return std::shared_ptr<CBLAuthorizationObserverInterface>(new SampleApplicationCBLAuthRequester(uiManager));
}

void SampleApplicationCBLAuthRequester::onRequestAuthorization(const std::string& url, const std::string& code) {
    m_uiManager->printMessage("NOT YET AUTHORIZED");
    std::ostringstream oss;
    oss << "To authorize, browse to: '" << url << "' and enter the code: " << code;
    m_uiManager->printMessage(oss.str());

    std::unique_lock<std::mutex> lock(m_mutex);
    m_authCheckCounter = 0;
    m_authCode = code;
    m_authUrl = url;
    lock.unlock();

    if (m_uiAuthNotifier) {
        m_uiAuthNotifier->notifyAuthorizationRequest(url, code);
    }
}

void SampleApplicationCBLAuthRequester::onCheckingForAuthorization() {
    std::ostringstream oss;

    std::unique_lock<std::mutex> lock(m_mutex);
    oss << "Checking for authorization (" << ++m_authCheckCounter << ")...";
    auto code = m_authCode;
    auto url = m_authUrl;
    lock.unlock();

    m_uiManager->printMessage(oss.str());
    if (m_uiAuthNotifier) {
        m_uiAuthNotifier->notifyAuthorizationRequest(url, code);
    }
}

void SampleApplicationCBLAuthRequester::onCustomerProfileAvailable(
    const CBLAuthorizationObserverInterface::CustomerProfile& customerProfile) {
    std::ostringstream oss;
    oss << "Name: " << customerProfile.name << " "
        << " Email: " << customerProfile.email;
    m_uiManager->printMessage(oss.str());
}

void SampleApplicationCBLAuthRequester::setUIAuthNotifier(
    std::shared_ptr<acsdkSampleApplicationInterfaces::UIAuthNotifierInterface> uiAuthNotifier) {
    m_uiAuthNotifier = std::move(uiAuthNotifier);
}

SampleApplicationCBLAuthRequester::SampleApplicationCBLAuthRequester(
    const std::shared_ptr<acsdkSampleApplicationInterfaces::UIManagerInterface>& uiManager) :
        m_uiManager{uiManager},
        m_authCheckCounter{0} {
}

}  // namespace acsdkSampleApplicationCBLAuthRequester
}  // namespace alexaClientSDK
