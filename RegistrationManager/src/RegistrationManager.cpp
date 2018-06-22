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

#include "RegistrationManager/RegistrationManager.h"

#include "AVSCommon/Utils/Logger/Logger.h"
#include "RegistrationManager/CustomerDataManager.h"

/// String to identify log entries originating from this file.
static const std::string TAG("RegistrationManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace registrationManager {

void RegistrationManager::logout() {
    ACSDK_DEBUG(LX("logout"));
    m_directiveSequencer->disable();
    m_connectionManager->disable();
    m_dataManager->clearData();
    notifyObservers();
}

RegistrationManager::RegistrationManager(
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>& directiveSequencer,
    std::shared_ptr<acl::AVSConnectionManager>& connectionManager,
    std::shared_ptr<CustomerDataManager> dataManager) :
        m_directiveSequencer{directiveSequencer},
        m_connectionManager{connectionManager},
        m_dataManager{dataManager} {
    if (!directiveSequencer) {
        ACSDK_ERROR(LX("RegistrationManagerFailed").m("Invalid directiveSequencer."));
    }
    if (!connectionManager) {
        ACSDK_ERROR(LX("RegistrationManagerFailed").m("Invalid connectionManager."));
    }
    if (!dataManager) {
        ACSDK_ERROR(LX("RegistrationManagerFailed").m("Invalid dataManager."));
    }
}

void RegistrationManager::addObserver(std::shared_ptr<RegistrationObserverInterface> observer) {
    std::lock_guard<std::mutex> lock{m_observersMutex};
    m_observers.insert(observer);
}

void RegistrationManager::removeObserver(std::shared_ptr<RegistrationObserverInterface> observer) {
    std::lock_guard<std::mutex> lock{m_observersMutex};
    m_observers.erase(observer);
}

void RegistrationManager::notifyObservers() {
    std::lock_guard<std::mutex> lock{m_observersMutex};
    for (auto& observer : m_observers) {
        observer->onLogout();
    }
}

}  // namespace registrationManager
}  // namespace alexaClientSDK
