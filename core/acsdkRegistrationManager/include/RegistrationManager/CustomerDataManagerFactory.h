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

#ifndef REGISTRATIONMANAGER_CUSTOMERDATAMANAGERFACTORY_H_
#define REGISTRATIONMANAGER_CUSTOMERDATAMANAGERFACTORY_H_

#include <memory>

#include <RegistrationManager/CustomerDataManagerInterface.h>

namespace alexaClientSDK {
namespace registrationManager {

/**
 * Factory class that creates an instance of @c CustomerDataManager.
 */
class CustomerDataManagerFactory {
public:
    /**
     * Creates an instance of @c CustomerDataManager.
     *
     * @note This method is @deprecated. Consider using RegistrationManagerComponent to create an instance of
     * @c CustomerDataManager.
     * @return A pointer to the @c CustomerDataManagerInterface.
     */
    static std::shared_ptr<registrationManager::CustomerDataManagerInterface> createCustomerDataManagerInterface();
};

}  // namespace registrationManager
}  // namespace alexaClientSDK

#endif  // REGISTRATIONMANAGER_CUSTOMERDATAMANAGERFACTORY_H_
