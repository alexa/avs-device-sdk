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

#ifndef REGISTRATIONMANAGER_REGISTRATIONNOTIFIER_H_
#define REGISTRATIONMANAGER_REGISTRATIONNOTIFIER_H_

#include <memory>

#include <acsdk/Notifier/Notifier.h>
#include <RegistrationManager/RegistrationNotifierInterface.h>
#include <RegistrationManager/RegistrationObserverInterface.h>

namespace alexaClientSDK {
namespace registrationManager {

/**
 * Implementation of the @c RegistrationNotifier.
 */
class RegistrationNotifier : public notifier::Notifier<registrationManager::RegistrationObserverInterface> {
public:
    /**
     * Factory method to create a @c RegistrationNotifier.
     *
     * @return A new instance of the @c RegistrationNotifier.
     */
    static std::shared_ptr<registrationManager::RegistrationNotifierInterface> createRegistrationNotifierInterface();
};

}  // namespace registrationManager
}  // namespace alexaClientSDK

#endif  // REGISTRATIONMANAGER_REGISTRATIONNOTIFIER_H_
