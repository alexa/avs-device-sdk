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

#ifndef ALEXA_CLIENT_SDK_REGISTRATIONMANAGER_INCLUDE_REGISTRATIONMANAGER_REGISTRATIONOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_REGISTRATIONMANAGER_INCLUDE_REGISTRATIONMANAGER_REGISTRATIONOBSERVERINTERFACE_H_

namespace alexaClientSDK {
namespace registrationManager {

/**
 * This interface is used to observe changes to the device registration, such as user logout.
 */
class RegistrationObserverInterface {
public:
    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~RegistrationObserverInterface() = default;

    /**
     * Notification that the current customer has logged out.
     *
     * @warning This method is called while RegistrationManager is in a locked state. The callback must not block on
     * calls to RegistrationManager methods either.
     */
    virtual void onLogout() = 0;
};

}  // namespace registrationManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_REGISTRATIONMANAGER_INCLUDE_REGISTRATIONMANAGER_REGISTRATIONOBSERVERINTERFACE_H_
