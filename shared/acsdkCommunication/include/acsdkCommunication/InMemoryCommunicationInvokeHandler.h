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

#ifndef ACSDKCOMMUNICATION_INMEMORYCOMMUNICATIONINVOKEHANDLER_H_
#define ACSDKCOMMUNICATION_INMEMORYCOMMUNICATIONINVOKEHANDLER_H_

#include <functional>
#include <mutex>
#include <unordered_map>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <acsdkCommunicationInterfaces/CommunicationInvokeHandlerInterface.h>

namespace alexaClientSDK {
namespace acsdkCommunication {

/**
 * The in memory implementation of the CommunicationInvokeHandlerInterface. This is a thread safe class that provides
 * users the ability to register functions and have them be invoked by other components.
 */
template <typename ReturnType, typename... Types>
class InMemoryCommunicationInvokeHandler
        : public virtual acsdkCommunicationInterfaces::CommunicationInvokeHandlerInterface<ReturnType, Types...> {
public:
    /// @name CommunicationInvokeHandlerInterface methods
    /// @{
    bool registerFunction(
        const std::string& name,
        std::shared_ptr<acsdkCommunicationInterfaces::FunctionInvokerInterface<ReturnType, Types...>>
            functionImplementation) override;
    alexaClientSDK::avsCommon::utils::error::SuccessResult<ReturnType> invoke(const std::string& name, Types... args)
        override;
    bool deregister(
        const std::string& name,
        const std::shared_ptr<acsdkCommunicationInterfaces::FunctionInvokerInterface<ReturnType, Types...>>&
            functionImplementation) override;
    /// }@

private:
    // map of names to functions.
    std::unordered_map<
        std::string,
        std::weak_ptr<acsdkCommunicationInterfaces::FunctionInvokerInterface<ReturnType, Types...>>>
        m_functions;
    // guard to protect the functions.
    std::mutex m_functionsGuard;
};
template <typename ReturnType, typename... Types>
bool InMemoryCommunicationInvokeHandler<ReturnType, Types...>::registerFunction(
    const std::string& name,
    std::shared_ptr<acsdkCommunicationInterfaces::FunctionInvokerInterface<ReturnType, Types...>>
        functionImplementation) {
    std::lock_guard<std::mutex> lock(m_functionsGuard);
    auto it = m_functions.find(name);
    if (it != m_functions.end()) {
        if (!it->second.expired()) {
            ACSDK_ERROR(
                alexaClientSDK::avsCommon::utils::logger::LogEntry("InMemoryCommunicationHandler", "registerFunction")
                    .m("Function is already Registered")
                    .d("function", name));
            return false;
        }
        // Erase if function is a nullptr;
        m_functions.erase(it);
    }
    if (!functionImplementation) {
        ACSDK_ERROR(
            alexaClientSDK::avsCommon::utils::logger::LogEntry("InMemoryCommunicationHandler", "registerFunction")
                .m("FunctionImplementation is a nullptr")
                .d("function", name));
        return false;
    }
    auto weakFunction = std::weak_ptr<acsdkCommunicationInterfaces::FunctionInvokerInterface<ReturnType, Types...>>(
        functionImplementation);

    m_functions.insert({name, weakFunction});
    return true;
}
template <typename ReturnType, typename... Types>
alexaClientSDK::avsCommon::utils::error::SuccessResult<ReturnType> InMemoryCommunicationInvokeHandler<
    ReturnType,
    Types...>::invoke(const std::string& name, Types... args) {
    auto it = m_functions.find(name);
    if (it == m_functions.end()) {
        ACSDK_ERROR(alexaClientSDK::avsCommon::utils::logger::LogEntry("InMemoryCommunicationHandler", "invoke")
                        .m("Function is not Registered")
                        .d("function", name));
        return alexaClientSDK::avsCommon::utils::error::SuccessResult<ReturnType>::failure();
    }
    auto function = it->second.lock();
    if (!function) {
        ACSDK_ERROR(alexaClientSDK::avsCommon::utils::logger::LogEntry("InMemoryCommunicationHandler", "invoke")
                        .m("Function is expired")
                        .d("function", name));
        std::lock_guard<std::mutex> lock(m_functionsGuard);
        m_functions.erase(it);
        return alexaClientSDK::avsCommon::utils::error::SuccessResult<ReturnType>::failure();
    }
    auto result = function->functionToBeInvoked(name, args...);
    return alexaClientSDK::avsCommon::utils::error::SuccessResult<ReturnType>::success(result);
}

template <typename ReturnType, typename... Types>
bool InMemoryCommunicationInvokeHandler<ReturnType, Types...>::deregister(
    const std::string& name,
    const std::shared_ptr<acsdkCommunicationInterfaces::FunctionInvokerInterface<ReturnType, Types...>>&
        functionImplementation) {
    if (!functionImplementation) {
        ACSDK_ERROR(alexaClientSDK::avsCommon::utils::logger::LogEntry("InMemoryCommunicationHandler", "deregister")
                        .m("FunctionImplementation is a nullptr")
                        .d("function", name));
        return false;
    }
    std::lock_guard<std::mutex> lock(m_functionsGuard);
    auto it = m_functions.find(name);
    if (it == m_functions.end()) {
        ACSDK_ERROR(alexaClientSDK::avsCommon::utils::logger::LogEntry("InMemoryCommunicationHandler", "deregister")
                        .m("Function is not Registered")
                        .d("function", name));
        return false;
    }
    if (it->second.lock() != functionImplementation) {
        ACSDK_ERROR(alexaClientSDK::avsCommon::utils::logger::LogEntry("InMemoryCommunicationHandler", "deregister")
                        .m("Function is Registered but does not match")
                        .d("function", name));
        return false;
    }
    m_functions.erase(it);
    return true;
}
}  // namespace acsdkCommunication
}  // namespace alexaClientSDK

#endif  // ACSDKCOMMUNICATION_INMEMORYCOMMUNICATIONINVOKEHANDLER_H_
