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

#ifndef REGISTRATIONMANAGER_CUSTOMERDATAMANAGER_H_
#define REGISTRATIONMANAGER_CUSTOMERDATAMANAGER_H_

#include <memory>
#include <mutex>
#include <unordered_set>

#include <RegistrationManager/CustomerDataManagerInterface.h>

namespace alexaClientSDK {
namespace registrationManager {

class CustomerDataManager : public registrationManager::CustomerDataManagerInterface {
public:
    /**
     * Creates an instance of @c CustomerDataManager.
     *
     * @return A pointer to @c CustomerDataManagerInterface.
     */
    static std::shared_ptr<registrationManager::CustomerDataManagerInterface> createCustomerDataManagerInteface();

    /**
     * Destructor.
     */
    virtual ~CustomerDataManager();

    /// @name CustomerDataManagerInterface methods.
    /// @{
    void addDataHandler(registrationManager::CustomerDataHandlerInterface* handler) override;
    void removeDataHandler(registrationManager::CustomerDataHandlerInterface* handler) override;
    void clearData() override;
    /// @}

private:
    /// Mutex to synchronize access to data handlers.
    std::mutex m_mutex;

    /// The list of all data handlers.
    std::unordered_set<registrationManager::CustomerDataHandlerInterface*> m_dataHandlers;
};

}  // namespace registrationManager
}  // namespace alexaClientSDK

#endif  // REGISTRATIONMANAGER_CUSTOMERDATAMANAGER_H_
