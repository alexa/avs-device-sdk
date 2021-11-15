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

#ifndef ACSDKASSETSINTERFACES_COMMUNICATION_AMDCOMMUNICATIONINTERFACE_H_
#define ACSDKASSETSINTERFACES_COMMUNICATION_AMDCOMMUNICATIONINTERFACE_H_

#include <acsdkCommunicationInterfaces/CommunicationInvokeHandlerInterface.h>
#include <acsdkCommunicationInterfaces/CommunicationPropertiesHandlerInterface.h>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace commonInterfaces {

class AmdCommunicationInterface
        : public virtual acsdkCommunicationInterfaces::CommunicationPropertiesHandlerInterface<int>
        , public virtual acsdkCommunicationInterfaces::CommunicationPropertiesHandlerInterface<std::string>
        , public virtual acsdkCommunicationInterfaces::CommunicationInvokeHandlerInterface<std::string>
        , public virtual acsdkCommunicationInterfaces::CommunicationInvokeHandlerInterface<bool, std::string> {
public:
    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    ~AmdCommunicationInterface() override = default;

    /*
     * Overriding CommunicationPropertyInterface for int and string
     */
    using CommunicationPropertiesHandlerInterface<int>::registerProperty;
    using CommunicationPropertiesHandlerInterface<int>::deregisterProperty;
    using CommunicationPropertiesHandlerInterface<int>::writeProperty;
    using CommunicationPropertiesHandlerInterface<int>::readProperty;
    using CommunicationPropertiesHandlerInterface<int>::subscribeToPropertyChangeEvent;
    using CommunicationPropertiesHandlerInterface<int>::unsubscribeToPropertyChangeEvent;

    using CommunicationPropertiesHandlerInterface<std::string>::registerProperty;
    using CommunicationPropertiesHandlerInterface<std::string>::deregisterProperty;
    using CommunicationPropertiesHandlerInterface<std::string>::writeProperty;
    using CommunicationPropertiesHandlerInterface<std::string>::readProperty;
    using CommunicationPropertiesHandlerInterface<std::string>::subscribeToPropertyChangeEvent;
    using CommunicationPropertiesHandlerInterface<std::string>::unsubscribeToPropertyChangeEvent;

    /*
     * Overriding CommunicationInvokeHandlerInterface for string
     */
    using CommunicationInvokeHandlerInterface<std::string>::registerFunction;
    using CommunicationInvokeHandlerInterface<std::string>::deregister;
    using CommunicationInvokeHandlerInterface<std::string>::invoke;

    using CommunicationInvokeHandlerInterface<bool, std::string>::registerFunction;
    using CommunicationInvokeHandlerInterface<bool, std::string>::deregister;
    using CommunicationInvokeHandlerInterface<bool, std::string>::invoke;
};

}  // namespace commonInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSINTERFACES_COMMUNICATION_AMDCOMMUNICATIONINTERFACE_H_
