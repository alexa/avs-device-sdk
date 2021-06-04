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
#ifndef REGISTRATIONMANAGER_REGISTRATIONMANAGERINTERFACE_H_
#define REGISTRATIONMANAGER_REGISTRATIONMANAGERINTERFACE_H_

namespace alexaClientSDK {
namespace registrationManager {

/**
 * This interface for @c RegistrationManager.
 */
class RegistrationManagerInterface {
public:
    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~RegistrationManagerInterface() = default;

    /**
     * Log out current customer. This will clear any persistent data.
     */
    virtual void logout() = 0;
};

}  // namespace registrationManager
}  // namespace alexaClientSDK

#endif  // REGISTRATIONMANAGER_REGISTRATIONMANAGERINTERFACE_H_
