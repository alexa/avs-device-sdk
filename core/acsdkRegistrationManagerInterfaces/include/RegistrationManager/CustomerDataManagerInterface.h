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
#ifndef REGISTRATIONMANAGER_CUSTOMERDATAMANAGERINTERFACE_H_
#define REGISTRATIONMANAGER_CUSTOMERDATAMANAGERINTERFACE_H_

#include "RegistrationManager/CustomerDataHandlerInterface.h"

namespace alexaClientSDK {
namespace registrationManager {

/**
 * The @c CustomerDataManagerInterface is an interface for an object that is responsible for managing customer data
 * and to ensure that one customer will not have access to another customer's data.
 */
class CustomerDataManagerInterface {
public:
    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~CustomerDataManagerInterface() = default;

    /**
     * Add object that tracks any sort of customer data.
     */
    virtual void addDataHandler(CustomerDataHandlerInterface* handler) = 0;

    /**
     * Remove object that tracks customer data.
     */
    virtual void removeDataHandler(CustomerDataHandlerInterface* handler) = 0;

    /**
     * Clear every customer data kept in the device.
     *
     * @note We do not guarantee the order that the CustomerDataHandlers are called.
     */
    virtual void clearData() = 0;
};

}  // namespace registrationManager
}  // namespace alexaClientSDK

#endif  // REGISTRATIONMANAGER_CUSTOMERDATAMANAGERINTERFACE_H_
