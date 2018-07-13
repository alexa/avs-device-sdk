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

#ifndef ALEXA_CLIENT_SDK_REGISTRATIONMANAGER_INCLUDE_REGISTRATIONMANAGER_CUSTOMERDATAMANAGER_H_
#define ALEXA_CLIENT_SDK_REGISTRATIONMANAGER_INCLUDE_REGISTRATIONMANAGER_CUSTOMERDATAMANAGER_H_

#include <list>
#include <mutex>
#include <unordered_set>

#include "RegistrationManager/CustomerDataHandler.h"

namespace alexaClientSDK {
namespace registrationManager {

/**
 * The @c CustomerDataManager is an object responsible for managing customer data and to ensure that one
 * customer will not have access to another customer's data.
 */
class CustomerDataManager {
public:
    /**
     * CustomerDataManager destructor.
     */
    virtual ~CustomerDataManager();

    /**
     * Add object that tracks any sort of customer data.
     */
    void addDataHandler(CustomerDataHandler* handler);

    /**
     * Remove object that tracks customer data.
     */
    void removeDataHandler(CustomerDataHandler* handler);

    /**
     * Clear every customer data kept in the device.
     *
     * @note We do not guarantee the order that the CustomerDataHandlers are called.
     */
    void clearData();

private:
    /// List of all data handlers.
    std::unordered_set<CustomerDataHandler*> m_dataHandlers;

    /// Mutex used to synchronize m_dataHandlers variable access.
    std::mutex m_mutex;
};

}  // namespace registrationManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_REGISTRATIONMANAGER_INCLUDE_REGISTRATIONMANAGER_CUSTOMERDATAMANAGER_H_
