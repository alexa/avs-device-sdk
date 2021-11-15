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

#ifndef ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONINVOKEHANDLERINTERFACE_H_
#define ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONINVOKEHANDLERINTERFACE_H_

#include <memory>
#include <string>

#include <AVSCommon/Utils/Error/SuccessResult.h>

#include "FunctionInvokerInterface.h"

namespace alexaClientSDK {
namespace acsdkCommunicationInterfaces {

/**
 * The CommunicationInvokeHandlerInterface is used to register, deregister, and invoke functions from another component
 * with only a link to the CommunicationInvokeHandler. The implementation of this interface is not responsible for
 * keeping FunctionInvokerInterface implementations alive.
 */
template <typename ReturnType, typename... ArgTypes>
class CommunicationInvokeHandlerInterface {
public:
    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~CommunicationInvokeHandlerInterface() = default;

    /**
     * Register a new function that other components can trigger with the CommunicationInvokeHandlerInterface
     * @param name The name of the function.
     * @param functionImplementation The class that implements functionToBeInvoked
     * @return true if succeeds, false otherwise
     */
    virtual bool registerFunction(
        const std::string& name,
        std::shared_ptr<FunctionInvokerInterface<ReturnType, ArgTypes...>> functionImplementation) = 0;
    /**
     * Invokes the registered function specified by the name. If the function isn't registered or the function
     * has expired nothing will be invoked.
     * @param name The name of the function
     * @param Args The args that will be passed to the function.
     * @return SuccessResult<ReturnType> if the callback succeeds or not. If successful the return value.
     */
    virtual alexaClientSDK::avsCommon::utils::error::SuccessResult<ReturnType> invoke(
        const std::string& name,
        ArgTypes...) = 0;
    /**
     * Deregister the function
     * @param name The name of the function to deregister
     * @param functionImplementation The function that we are deregistering. Used for confirmation of ownership.
     * @return true if deregister succeeds, false otherwise
     */
    virtual bool deregister(
        const std::string& name,
        const std::shared_ptr<FunctionInvokerInterface<ReturnType, ArgTypes...>>& functionImplementation) = 0;
};

}  // namespace acsdkCommunicationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONINVOKEHANDLERINTERFACE_H_
