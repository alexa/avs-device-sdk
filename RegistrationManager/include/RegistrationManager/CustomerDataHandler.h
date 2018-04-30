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

#ifndef ALEXA_CLIENT_SDK_REGISTRATIONMANAGER_INCLUDE_REGISTRATIONMANAGER_CUSTOMERDATAHANDLER_H_
#define ALEXA_CLIENT_SDK_REGISTRATIONMANAGER_INCLUDE_REGISTRATIONMANAGER_CUSTOMERDATAHANDLER_H_

#include <memory>

namespace alexaClientSDK {
namespace registrationManager {

class CustomerDataManager;

/**
 * Abstract base class which requires the derived class to implement a @c clearData() function.
 *
 * For changes in the device registration, it is extremely important to remove any customer data saved in the device.
 * Classes that have any data related to the currently logged user must extend this class to guarantee that their data
 * will be wiped out during logout.
 */
class CustomerDataHandler {
public:
    /**
     * Build and register the new object with the CustomerDataManager.
     *
     * @param customerDataManager The CustomerDataManager that will keep track of the new data handler.
     * @note The customerDataManager must have a valid pointer to a manager instance.
     */
    CustomerDataHandler(std::shared_ptr<CustomerDataManager> customerDataManager);

    /**
     * CustomerDataHandler destructor.
     *
     * Deregister the handler with the CustomerDataManager.
     */
    virtual ~CustomerDataHandler();

    /**
     * Reset any internal state that may be associated with a particular user.
     *
     * @warning Object must succeed in deleting any customer data.
     * @warning This method is called while CustomerDataManager is in a locked state. Do not call or wait for any
     * CustomerDataManager operation.
     */
    virtual void clearData() = 0;

private:
    /**
     * Keep a constant pointer to CustomerDataManager so that the CustomerDataHandler object can auto remove itself.
     *
     * @note The goal is to guarantee that all customerDataHandlers are properly managed. The trade-off is that we have
     * to keep a shared_pointer to its manager and the manager has to keep a raw pointer to each handler.
     */
    const std::shared_ptr<CustomerDataManager> m_dataManager;
};

}  // namespace registrationManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_REGISTRATIONMANAGER_INCLUDE_REGISTRATIONMANAGER_CUSTOMERDATAHANDLER_H_
