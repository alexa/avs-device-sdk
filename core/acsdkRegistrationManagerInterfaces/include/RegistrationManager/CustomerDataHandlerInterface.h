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
#ifndef REGISTRATIONMANAGER_CUSTOMERDATAHANDLERINTERFACE_H_
#define REGISTRATIONMANAGER_CUSTOMERDATAHANDLERINTERFACE_H_

namespace alexaClientSDK {
namespace registrationManager {

/**
 * Interface class which requires the derived class to implement a @c clearData() function.
 *
 * For changes in the device registration, it is extremely important to remove any customer data saved in the device.
 * Classes that have any data related to the currently logged user must extend this class to guarantee that their data
 * will be wiped out during logout.
 */
class CustomerDataHandlerInterface {
public:
    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~CustomerDataHandlerInterface() = default;

    /**
     * Reset any internal state that may be associated with a particular user.
     *
     * @warning Object must succeed in deleting any customer data.
     * @warning This method is called while CustomerDataManager is in a locked state. Do not call or wait for any
     * CustomerDataManager operation.
     */
    virtual void clearData() = 0;
};

}  // namespace registrationManager
}  // namespace alexaClientSDK

#endif  // REGISTRATIONMANAGER_CUSTOMERDATAHANDLERINTERFACE_H_
