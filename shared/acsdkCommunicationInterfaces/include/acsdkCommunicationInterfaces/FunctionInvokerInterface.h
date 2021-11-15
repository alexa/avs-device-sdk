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
#ifndef ACSDKCOMMUNICATIONINTERFACES_FUNCTIONINVOKERINTERFACE_H_
#define ACSDKCOMMUNICATIONINTERFACES_FUNCTIONINVOKERINTERFACE_H_

#include <functional>
#include <string>

namespace alexaClientSDK {
namespace acsdkCommunicationInterfaces {

/**
 * The interface for the functions that can be invoked by external components. The implementation of the interface will
 * need to be registered with CommunicationInvokeHandlerInterface. This interface the first template type will be the
 * return type and the rest will be the arguments for the function.
 */
template <typename ReturnType, typename... Types>
class FunctionInvokerInterface {
public:
    /**
     * default destructor
     */
    virtual ~FunctionInvokerInterface() = default;
    /**
     * The function or functions that will be registered and invoked by external components. We will specify different
     * functions by using the name param.
     * @param name the name of the function we want to invoke.
     * @param args variadic args that are passed as arguments to the function.
     * @return ReturnType the result of the function.
     */
    virtual ReturnType functionToBeInvoked(const std::string& name, Types... args) = 0;
};

}  // namespace acsdkCommunicationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKCOMMUNICATIONINTERFACES_FUNCTIONINVOKERINTERFACE_H_
