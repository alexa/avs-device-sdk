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

#include "IPCServerSampleApp/IPC/IPCHandlerBase.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/// Literal to identify log entries originating from this file.
#define TAG "IPCHandlerBase"

/**
 * Create a LogEntry using this file's TAG and the specified event string
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

using namespace alexaClientSDK::avsCommon::utils::threading;

IPCHandlerBase::IPCHandlerBase(const std::string& namespaceName, std::shared_ptr<Executor> executor) :
        m_namespaceName{namespaceName},
        m_executor{executor} {
    if (!executor) {
        ACSDK_CRITICAL(
            LX("IPCHandlerBase").d("reason", "nullExecutor").m("Expect child class to provide the executor"));
    }
}

void IPCHandlerBase::invokeMethod(const std::string& methodName, const std::string& message) {
    m_executor->submit([this, methodName, message] {
        auto it = m_messageHandlers.find(methodName);
        if (it != m_messageHandlers.end()) {
            it->second(message);
        } else {
            ACSDK_WARN(LX("invokeMethod").d("reason", "unknownMethod").d("IPCMessageName", methodName));
        }
    });
}

bool IPCHandlerBase::registerIPCMessageHandlerMethod(const std::string& methodName, IPCMessageHandler messageHandler) {
    return m_executor
        ->submit([this, methodName, messageHandler] {
            auto it = m_messageHandlers.find(methodName);
            if (it != m_messageHandlers.end()) {
                ACSDK_WARN(LX("registerIPCMessageHandlerMethod")
                               .d("reason", "handlerAlreadyRegistered")
                               .d("methodName", methodName));
                return false;
            }

            m_messageHandlers[methodName] = messageHandler;
            return true;
        })
        .get();
}

bool IPCHandlerBase::deregisterIPCMessageHandlerMethod(const std::string& methodName) {
    return m_executor
        ->submit([this, methodName] {
            auto it = m_messageHandlers.find(methodName);
            if (it == m_messageHandlers.end()) {
                ACSDK_WARN(LX("registerIPCMessageHandlerMethod")
                               .d("reason", "handlerNotRegistered")
                               .d("methodName", methodName));
                return false;
            }

            m_messageHandlers.erase(it);
            return true;
        })
        .get();
}

IPCHandlerBase::~IPCHandlerBase() {
    m_messageHandlers.clear();
}

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
