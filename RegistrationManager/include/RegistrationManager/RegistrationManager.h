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

#ifndef ALEXA_CLIENT_SDK_REGISTRATIONMANAGER_INCLUDE_REGISTRATIONMANAGER_REGISTRATIONMANAGER_H_
#define ALEXA_CLIENT_SDK_REGISTRATIONMANAGER_INCLUDE_REGISTRATIONMANAGER_REGISTRATIONMANAGER_H_

#include <memory>
#include <unordered_set>

#include <ACL/AVSConnectionManager.h>
#include <ADSL/DirectiveSequencer.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>

#include "RegistrationManager/CustomerDataManager.h"
#include "RegistrationManager/RegistrationObserverInterface.h"

namespace alexaClientSDK {
namespace registrationManager {

/**
 * The @c RegistrationManager is responsible for logout and deregister actions.
 *
 * When a user is logging out of the device, the registration manager will close down the AVS connection,
 * cancel ongoing directives and delete any customer data saved in the device.
 */
class RegistrationManager {
public:
    /**
     * RegistrationManager constructor.
     *
     * @param directiveSequencer Object used to clear directives during logout process.
     * @param connectionManager Connection manager must be disabled during customer logout.
     * @param dataManager Object that manages customer data, which must be cleared during logout.
     */
    RegistrationManager(
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>& directiveSequencer,
        std::shared_ptr<acl::AVSConnectionManager>& connectionManager,
        std::shared_ptr<CustomerDataManager> dataManager);

    /**
     * RegistrationManager destructor
     */
    virtual ~RegistrationManager() = default;

    /**
     * Log out current customer. This will clear any persistent data.
     */
    void logout();

    /**
     * Add a new registration observer object which will get notified after the registration state has changed.
     *
     * @param observer Object to be notified of any registration event.
     */
    void addObserver(std::shared_ptr<RegistrationObserverInterface> observer);

    /**
     * Remove the given observer object which will no longer get any registration notification.
     *
     * @param observer Object to be removed from observers set.
     */
    void removeObserver(std::shared_ptr<RegistrationObserverInterface> observer);

private:
    /**
     * Notify all observers that a new registration even has happened.
     */
    void notifyObservers();

    /// Used to cancel all directives.
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> m_directiveSequencer;

    // Used to enable / disable connection during logout to avoid any interruption.
    std::shared_ptr<acl::AVSConnectionManager> m_connectionManager;

    // Used to clear customer data to ensure that a future login will not have access to previous customer data
    std::shared_ptr<CustomerDataManager> m_dataManager;

    /// Mutex for registration observers.
    std::mutex m_observersMutex;

    // Observers
    std::unordered_set<std::shared_ptr<RegistrationObserverInterface> > m_observers;
};

}  // namespace registrationManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_REGISTRATIONMANAGER_INCLUDE_REGISTRATIONMANAGER_REGISTRATIONMANAGER_H_
