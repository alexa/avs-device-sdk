/*
 * ContextManager.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <string>
#include <AVSUtils/Logging/Logger.h>
#include "ContextManager/ContextManager.h"

/// A state provider is expected to respond to a @c provideState request within this timeout period.
static const std::chrono::seconds PROVIDE_STATE_DEFAULT_TIMEOUT = std::chrono::seconds(2);

namespace alexaClientSDK {
namespace contextManager {

using namespace rapidjson;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsUtils;

/// The namespace json key in the header of the state.
static const std::string NAMESPACE_JSON_KEY = "namespace";

/// The name json key in the header of the state.
static const std::string NAME_JSON_KEY = "name";

/// The header json key in the state.
static const std::string HEADER_JSON_KEY = "header";

/// The payload json key in the state.
static const std::string PAYLOAD_JSON_KEY = "payload";

/// The context json key.
static const std::string CONTEXT_JSON_KEY = "context";

std::shared_ptr<ContextManager> ContextManager::create() {
    std::shared_ptr<ContextManager> contextManager(new ContextManager());
    contextManager->init();
    return contextManager;
}

ContextManager::~ContextManager() {
    std::unique_lock<std::mutex> lock(m_contextRequesterMutex);
    m_shutdown = true;
    lock.unlock();
    m_contextLoopNotifier.notify_one();

    if (m_updateStatesThread.joinable()) {
        m_updateStatesThread.join();
    }
}

void ContextManager::setStateProvider(const NamespaceAndName& stateProviderName,
        std::shared_ptr<StateProviderInterface> stateProvider) {
    std::lock_guard<std::mutex> stateProviderLock(m_stateProviderMutex);
    if (!stateProvider) {
        m_namespaceNameToStateInfo.erase(stateProviderName);
        Logger::log("Removed the stateProvider of namespace:"+ stateProviderName.nameSpace + " and name:" +
                stateProviderName.name + " from the ContextManager");
        return;
    }
    auto stateInfoMappingIt = m_namespaceNameToStateInfo.find(stateProviderName);
    if (m_namespaceNameToStateInfo.end() == stateInfoMappingIt) {
        m_namespaceNameToStateInfo[stateProviderName] = std::make_shared<StateInfo>(stateProvider);
    } else {
        stateInfoMappingIt->second->stateProvider = stateProvider;
    }
}

SetStateResult ContextManager::setState(const NamespaceAndName& stateProviderName, const std::string& jsonState,
        const StateRefreshPolicy& refreshPolicy, const unsigned int stateRequestToken) {
    std::lock_guard<std::mutex> stateProviderLock(m_stateProviderMutex);
    if (0 == stateRequestToken) {
        return updateStateLocked(stateProviderName, jsonState, refreshPolicy);
    }
    if (stateRequestToken != m_stateRequestToken) {
        Logger::log("State token outdated for the stateProvider of namespace:"+ stateProviderName.nameSpace
                + " and name:" + stateProviderName.name);
        return SetStateResult::STATE_TOKEN_OUTDATED;
    }
    SetStateResult status = updateStateLocked(stateProviderName, jsonState, refreshPolicy);
    if (SetStateResult::SUCCESS == status) {
        auto it = m_pendingOnStateProviders.find(stateProviderName);
        if (it != m_pendingOnStateProviders.end()) {
            m_pendingOnStateProviders.erase(it);
        }
        /*
         * Once the m_pendingOnStateProviders is empty, it implies that the ContextManager has received the state
         * from all the StateProviderInterfaces it sent provideState requests to, and updateStatesLoop should be
         * notified.
         */
        if (m_pendingOnStateProviders.empty()) {
            m_setStateCompleteNotifier.notify_one();
        }
    }
    return status;
}

void ContextManager::getContext(std::shared_ptr<ContextRequesterInterface> contextRequester) {
    std::lock_guard<std::mutex> contextRequesterLock(m_contextRequesterMutex);
    m_contextRequesterQueue.push(contextRequester);
    /*
     * The updateStatesLoop needs to be notified only when the first entry is added to the m_contextRequesterQueue.
     * Once the states are updated, all the context requesters are updated with the latest context.
     */
    if (m_contextRequesterQueue.size() > 0) {
        m_contextLoopNotifier.notify_one();
    }
}

ContextManager::StateInfo::StateInfo(
        std::shared_ptr<avsCommon::sdkInterfaces::StateProviderInterface> initStateProvider,
        std::string initJsonState, avsCommon::sdkInterfaces::StateRefreshPolicy initRefreshPolicy):
                stateProvider{initStateProvider}, jsonState{initJsonState}, refreshPolicy{initRefreshPolicy} {

}

ContextManager::ContextManager() :
        m_stateRequestToken{0},
        m_shutdown{false} {
}

void ContextManager::init() {
    m_updateStatesThread = std::thread(&ContextManager::updateStatesLoop, this);
}

SetStateResult ContextManager::updateStateLocked(const NamespaceAndName& stateProviderName,
        const std::string& jsonState, const StateRefreshPolicy& refreshPolicy) {
    auto stateInfoMappingIt = m_namespaceNameToStateInfo.find(stateProviderName);
    if (m_namespaceNameToStateInfo.end() == stateInfoMappingIt) {
        if (StateRefreshPolicy::ALWAYS == refreshPolicy) {
            Logger::log("State provider of namespace:"+ stateProviderName.nameSpace + " and name:" +
                stateProviderName.name + " not registered with the Context Manager");
            return SetStateResult::STATE_PROVIDER_NOT_REGISTERED;
        }
        m_namespaceNameToStateInfo[stateProviderName] = std::make_shared<StateInfo>(nullptr, jsonState, refreshPolicy);
    } else {
        stateInfoMappingIt->second->jsonState = jsonState;
        stateInfoMappingIt->second->refreshPolicy = refreshPolicy;
        Logger::log("Updated state for state provider of namespace:"+ stateProviderName.nameSpace + " and name:" +
                stateProviderName.name + " to " + jsonState);
    }
    return SetStateResult::SUCCESS;
}

void ContextManager::requestStatesLocked(std::unique_lock<std::mutex>& stateProviderLock) {
    m_stateRequestToken++;
    /*
     * If the token has wrapped around and token is 0, increment again. 0 is reserved for when the
     * stateProviderInterface does not send a token.
     */
    if (0 == m_stateRequestToken) {
        m_stateRequestToken++;
    }
    unsigned int curStateReqToken(m_stateRequestToken);

    for (auto it = m_namespaceNameToStateInfo.begin(); it != m_namespaceNameToStateInfo.end(); ++it) {
        auto & stateInfo = it->second;
        if (StateRefreshPolicy::ALWAYS == stateInfo->refreshPolicy) {
            m_pendingOnStateProviders.insert(it->first);
            stateProviderLock.unlock();
            stateInfo->stateProvider->provideState(curStateReqToken);
            stateProviderLock.lock();
        }
    }
}

void ContextManager::sendContextAndClearQueue(const std::string& context,
        const ContextRequestError& contextRequestError) {
    std::unique_lock<std::mutex> contextRequesterLock(m_contextRequesterMutex);
    while (!m_contextRequesterQueue.empty()) {
        auto currentContextRequester = m_contextRequesterQueue.front();
        contextRequesterLock.unlock();
        if (!context.empty()) {
            currentContextRequester->onContextAvailable(context);
        } else {
            currentContextRequester->onContextFailure(contextRequestError);
        }
        contextRequesterLock.lock();
        m_contextRequesterQueue.pop();
    }
}

void ContextManager::updateStatesLoop() {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(m_contextRequesterMutex);
            m_contextLoopNotifier.wait(lock, [this]() { return (!m_contextRequesterQueue.empty() || m_shutdown); });
            if (m_shutdown) {
                return;
            }
        }

        std::unique_lock<std::mutex> stateProviderLock(m_stateProviderMutex);
        requestStatesLocked(stateProviderLock);

        if (!m_pendingOnStateProviders.empty()){
            if (!m_setStateCompleteNotifier.wait_for(stateProviderLock, PROVIDE_STATE_DEFAULT_TIMEOUT,
                    [this]() { return m_pendingOnStateProviders.empty(); })) {
                stateProviderLock.unlock();
                Logger::log("State Update Failed: State Provider timed out");
                sendContextAndClearQueue("", ContextRequestError::STATE_PROVIDER_TIMEDOUT);
                continue;
            }
        }
        stateProviderLock.unlock();

        sendContextToRequesters();
    }
}

Value ContextManager::buildHeader(const NamespaceAndName& namespaceAndName,
        Document::AllocatorType& allocator) {
    Value header(kObjectType);

    header.AddMember(StringRef(NAMESPACE_JSON_KEY), namespaceAndName.nameSpace, allocator );
    header.AddMember(StringRef(NAME_JSON_KEY), namespaceAndName.name, allocator);

    return header;
}

Value ContextManager::buildState(const NamespaceAndName& namespaceAndName, const std::string jsonPayloadValue,
        Document::AllocatorType& allocator) {
    Document payload;
    Value state(kObjectType);
    Value header = buildHeader(namespaceAndName, allocator);

    // If the header is an empty object or during parsing the payload, an error occurs, return an empty event value.
    if (header.ObjectEmpty()){
        Logger::log("BuildStateFailed: Empty header");
        return state;
    }

    if (payload.Parse(jsonPayloadValue).HasParseError()) {
        Logger::log("BuildStateFailed: Unable to parse payload:" + jsonPayloadValue);
        return state;
    }

    state.AddMember(StringRef(HEADER_JSON_KEY), header, allocator);
    state.AddMember(StringRef(PAYLOAD_JSON_KEY), payload, allocator);

    return state;
}

void ContextManager::sendContextToRequesters() {
    bool errorBuildingContext = false;
    Document jsonContext;
    StringBuffer jsonContextBuf;

    jsonContext.SetObject();
    Value statesArray(kArrayType);
    Document::AllocatorType& allocator = jsonContext.GetAllocator();

    std::unique_lock<std::mutex> stateProviderLock(m_stateProviderMutex);
    for (auto it = m_namespaceNameToStateInfo.begin(); it != m_namespaceNameToStateInfo.end(); ++it) {
        auto& stateInfo = it->second;
        Value jsonState = buildState(it->first, stateInfo->jsonState, allocator);
        if (jsonState.ObjectEmpty()){
            Logger::log("Build Context Failed: Empty jsonState");
            errorBuildingContext = true;
            break;
        }
        statesArray.PushBack(Value(jsonState, allocator).Move(), allocator);
    }
    stateProviderLock.unlock();

    if (!errorBuildingContext) {
        jsonContext.AddMember(StringRef(CONTEXT_JSON_KEY), statesArray, allocator);
        Writer<StringBuffer> writer(jsonContextBuf);
        if (!jsonContext.Accept(writer)) {
            Logger::log("Build Context Failed: Accept writer failed");
            errorBuildingContext = true;
        }
    }

    if (errorBuildingContext) {
        sendContextAndClearQueue("", ContextRequestError::BUILD_CONTEXT_ERROR);
    } else {
        Logger::log("Context built successfully:" + std::string(jsonContextBuf.GetString()));
        sendContextAndClearQueue(jsonContextBuf.GetString());
    }
}

} // namespace contextManager
} // namespace alexaClientSDK
