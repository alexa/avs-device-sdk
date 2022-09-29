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

#include "IPCServerSampleApp/IPC/IPCRouter.h"

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::json;

#define TAG "IPCRouter"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The header json key in the message.
static const char HEADER_TAG[] = "header";

/// The namespace json key in the message.
static const char NAMESPACE_TAG[] = "namespace";

/// The name json key in the message.
static const char NAME_TAG[] = "name";

/// The version json key in the message.
static const char VERSION_TAG[] = "version";

/// The payload json key in the message.
static const char PAYLOAD_TAG[] = "payload";

std::shared_ptr<IPCRouter> IPCRouter::create(
    std::shared_ptr<communication::MessagingServerInterface> messagingServer,
    std::shared_ptr<IPCDispatcherInterface> ipcDispatcher,
    std::shared_ptr<IPCVersionManager> ipcVersionManager) {
    if (!messagingServer) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessagingServer"));
        return nullptr;
    }

    if (!ipcDispatcher) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIPCDispatcher"));
        return nullptr;
    }

    if (!ipcVersionManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIPCVersionManager"));
        return nullptr;
    }

    return std::shared_ptr<IPCRouter>(
        new IPCRouter(std::move(messagingServer), std::move(ipcDispatcher), std::move(ipcVersionManager)));
}

IPCRouter::IPCRouter(
    std::shared_ptr<communication::MessagingServerInterface> messagingServer,
    std::shared_ptr<IPCDispatcherInterface> ipcDispatcher,
    const std::shared_ptr<IPCVersionManager> ipcVersionManager) :
        RequiresShutdown{TAG},
        m_messagingServer{messagingServer},
        m_ipcVersionManager{ipcVersionManager},
        m_ipcDispatcher{ipcDispatcher} {
}

std::shared_ptr<IPCDispatcherInterface> IPCRouter::registerHandler(
    const std::string& ipcNamespace,
    std::weak_ptr<IPCHandlerBase> handler) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto handlerInstance = getHandlerRegisteredLocked(ipcNamespace);
    if (handlerInstance) {
        ACSDK_ERROR(LX(__func__).d("reason", "ipcComponentAlreadyRegistered"));
        return nullptr;
    }

    m_handlerMap.insert({ipcNamespace, {ipcNamespace, handler}});

    return m_ipcDispatcher;
}

bool IPCRouter::deregisterHandler(const std::string& ipcNamespace) {
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_handlerMap.find(ipcNamespace);
    if (it != m_handlerMap.end()) {
        m_handlerMap.erase(it);
        return true;
    }

    return false;
}

void IPCRouter::onMessage(const std::string& message) {
    ACSDK_DEBUG9(LX("onMessageInExecutor").sensitive("message", message));
    rapidjson::Document jsonMessage;
    rapidjson::ParseResult result = jsonMessage.Parse(message);
    if (!result) {
        ACSDK_ERROR(LX("onMessageFailed").d("reason", "parsingPayloadFailed"));
        return;
    }

    std::string header;
    if (!jsonUtils::retrieveValue(message, HEADER_TAG, &header)) {
        ACSDK_ERROR(LX("onMessageFailed").d("reason", "headerNotFound"));
        return;
    }

    std::string ipcNamespace;
    if (!jsonUtils::retrieveValue(header, NAMESPACE_TAG, &ipcNamespace)) {
        ACSDK_ERROR(LX("onMessageFailed").d("reason", "namespaceNotFound"));
        return;
    }

    std::string methodName;
    if (!jsonUtils::retrieveValue(header, NAME_TAG, &methodName)) {
        ACSDK_ERROR(LX("onMessageFailed").d("reason", "methodNameNotFound"));
        return;
    }

    if (!jsonMessage[HEADER_TAG].HasMember(VERSION_TAG)) {
        ACSDK_ERROR(LX("onMessageFailed").d("reason", "versionNotFound"));
        return;
    }

    int version = jsonMessage[HEADER_TAG][VERSION_TAG].GetInt();
    if (!m_ipcVersionManager->validateVersionForNamespace(ipcNamespace, version)) {
        ACSDK_ERROR(LX("onMessageFailed").d("reason", "versionValidationFailed"));
        return;
    }

    std::string stringPayload;
    if (!jsonUtils::retrieveValue(message, PAYLOAD_TAG, &stringPayload)) {
        ACSDK_ERROR(LX("onMessageFailed").d("reason", "payloadNotFound"));
        return;
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    auto handler = getHandlerRegisteredLocked(ipcNamespace);
    lock.unlock();

    if (!handler) {
        ACSDK_ERROR(LX("onMessageFailed").d("reason", "unableToFindHandler").d("namespace", ipcNamespace));
        return;
    }

    handler->invokeMethod(std::move(methodName), std::move(stringPayload));
}

void IPCRouter::doShutdown() {
    m_messagingServer.reset();
    m_ipcVersionManager.reset();

    std::lock_guard<std::mutex> lock{m_mutex};
    m_handlerMap.clear();
}

std::shared_ptr<IPCHandlerBase> IPCRouter::getHandlerRegisteredLocked(const std::string& ipcNamespace) {
    std::shared_ptr<IPCHandlerBase> handler;
    const auto it = m_handlerMap.find(ipcNamespace);
    if (it == m_handlerMap.end()) {
        return nullptr;
    }

    if (!(handler = it->second.handler.lock())) {
        ACSDK_ERROR(LX("getHandlerRegisteredLocked").d("reason", "invalidHandler"));
        m_handlerMap.erase(it);
        return nullptr;
    }

    return handler;
}

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
