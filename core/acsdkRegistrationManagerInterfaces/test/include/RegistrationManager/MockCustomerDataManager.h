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

#ifndef REGISTRATIONMANAGER_MOCKCUSTOMERDATAMANAGER_H_
#define REGISTRATIONMANAGER_MOCKCUSTOMERDATAMANAGER_H_

#include <RegistrationManager/CustomerDataManagerInterface.h>

namespace alexaClientSDK {
namespace registrationManager {

class MockCustomerDataManager : public registrationManager::CustomerDataManagerInterface {
public:
    /// @name CustomerDataManagerInterface methods
    /// @{
    MOCK_METHOD1(addDataHandler, void(CustomerDataHandlerInterface*));
    MOCK_METHOD1(removeDataHandler, void(CustomerDataHandlerInterface*));
    MOCK_METHOD0(clearData, void());
    /// @}
};

}  // namespace registrationManager
}  // namespace alexaClientSDK

#endif  // REGISTRATIONMANAGER_MOCKCUSTOMERDATAMANAGER_H_
