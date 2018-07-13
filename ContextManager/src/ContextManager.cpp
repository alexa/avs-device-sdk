/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "ContextManager/ContextManager.h"

/// A state provider is expected to respond to a @c provideState request within this timeout period.
static const std::chrono::seconds PROVIDE_STATE_DEFAULT_TIMEOUT = std::chrono::seconds(2);

namespace alexaClientSDK {
namespace contextManager {

using namespace rapidjson;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("ContextManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

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

void ContextManager::setStateProvider(
    const NamespaceAndName& stateProviderName,
    std::shared_ptr<StateProviderInterface> stateProvider) {
    std::lock_guard<std::mutex> stateProviderLock(m_stateProviderMutex);
    if (!stateProvider) {
        m_namespaceNameToStateInfo.erase(stateProviderName);
        ACSDK_DEBUG(LX("setStateProvider")
                        .d("action", "removedStateProvider")
                        .d("namespace", stateProviderName.nameSpace)
                        .d("name", stateProviderName.name));
        return;
    }
    auto stateInfoMappingIt = m_namespaceNameToStateInfo.find(stateProviderName);
    if (m_namespaceNameToStateInfo.end() == stateInfoMappingIt) {
        m_namespaceNameToStateInfo[stateProviderName] = std::make_shared<StateInfo>(stateProvider);
    } else {
        stateInfoMappingIt->second->stateProvider = stateProvider;
    }
}

SetStateResult ContextManager::setState(
    const NamespaceAndName& stateProviderName,
    const std::string& jsonState,
    const StateRefreshPolicy& refreshPolicy,
    const unsigned int stateRequestToken) {
    std::lock_guard<std::mutex> stateProviderLock(m_stateProviderMutex);
    if (0 == stateRequestToken) {
        return updateStateLocked(stateProviderName, jsonState, refreshPolicy);
    }
    if (stateRequestToken != m_stateRequestToken) {
        ACSDK_ERROR(LX("setStateFailed")
                        .d("reason", "outdatedStateToken")
                        .d("namespace", stateProviderName.nameSpace)
                        .d("name", stateProviderName.name)
                        .sensitive("suppliedToken", stateRequestToken)
                        .sensitive("expectedToken", m_stateRequestToken));

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
    std::string initJsonState,
    avsCommon::avs::StateRefreshPolicy initRefreshPolicy) :
        stateProvider{initStateProvider},
        jsonState{initJsonState},
        refreshPolicy{initRefreshPolicy} {
}

ContextManager::ContextManager() : m_stateRequestToken{0}, m_shutdown{false} {
}

void ContextManager::init() {
    m_updateStatesThread = std::thread(&ContextManager::updateStatesLoop, this);
}

SetStateResult ContextManager::updateStateLocked(
    const NamespaceAndName& stateProviderName,
    const std::string& jsonState,
    const StateRefreshPolicy& refreshPolicy) {
    auto stateInfoMappingIt = m_namespaceNameToStateInfo.find(stateProviderName);
    if (m_namespaceNameToStateInfo.end() == stateInfoMappingIt) {
        if (StateRefreshPolicy::ALWAYS == refreshPolicy || StateRefreshPolicy::SOMETIMES == refreshPolicy) {
            ACSDK_ERROR(LX("updateStateLockedFailed")
                            .d("reason", "unregisteredStateProvider")
                            .d("namespace", stateProviderName.nameSpace)
                            .d("name", stateProviderName.name));
            return SetStateResult::STATE_PROVIDER_NOT_REGISTERED;
        }
        m_namespaceNameToStateInfo[stateProviderName] = std::make_shared<StateInfo>(nullptr, jsonState, refreshPolicy);
    } else {
        stateInfoMappingIt->second->jsonState = jsonState;
        stateInfoMappingIt->second->refreshPolicy = refreshPolicy;
        ACSDK_DEBUG(LX("updateStateLocked")
                        .d("action", "updatedState")
                        .sensitive("state", jsonState)
                        .d("namespace", stateProviderName.nameSpace)
                        .d("name", stateProviderName.name));
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
        auto& stateInfo = it->second;
        if (StateRefreshPolicy::ALWAYS == stateInfo->refreshPolicy ||
            StateRefreshPolicy::SOMETIMES == stateInfo->refreshPolicy) {
            m_pendingOnStateProviders.insert(it->first);
            stateProviderLock.unlock();
            stateInfo->stateProvider->provideState(it->first, curStateReqToken);
            stateProviderLock.lock();
        }
    }
}

void ContextManager::sendContextAndClearQueue(
    const std::string& context,
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

        if (!m_pendingOnStateProviders.empty()) {
            if (!m_setStateCompleteNotifier.wait_for(stateProviderLock, PROVIDE_STATE_DEFAULT_TIMEOUT, [this]() {
                    return m_pendingOnStateProviders.empty();
                })) {
                stateProviderLock.unlock();
                ACSDK_ERROR(LX("updateStatesLoopFailed").d("reason", "stateProviderTimedOut"));
                sendContextAndClearQueue("", ContextRequestError::STATE_PROVIDER_TIMEDOUT);
                continue;
            }
        }
        stateProviderLock.unlock();

        sendContextToRequesters();
    }
}

Value ContextManager::buildHeader(const NamespaceAndName& namespaceAndName, Document::AllocatorType& allocator) {
    Value header(kObjectType);

    header.AddMember(StringRef(NAMESPACE_JSON_KEY), namespaceAndName.nameSpace, allocator);
    header.AddMember(StringRef(NAME_JSON_KEY), namespaceAndName.name, allocator);

    return header;
}

Value ContextManager::buildState(
    const NamespaceAndName& namespaceAndName,
    const std::string jsonPayloadValue,
    Document::AllocatorType& allocator) {
    Document payload;
    Value state(kObjectType);
    Value header = buildHeader(namespaceAndName, allocator);

    // If the header is an empty object or during parsing the payload, an error occurs, return an empty event value.
    if (header.ObjectEmpty()) {
        ACSDK_ERROR(LX("buildStateFailed").d("reason", "emptyHeader"));
        return state;
    }

    if (payload.Parse(jsonPayloadValue).HasParseError()) {
        ACSDK_ERROR(LX("buildStateFailed").d("reason", "parseError").d("payload", jsonPayloadValue));
        return state;
    }

    state.AddMember(StringRef(HEADER_JSON_KEY), header, allocator);
    state.AddMember(StringRef(PAYLOAD_JSON_KEY), Value(payload, allocator), allocator);

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
        if (stateInfo->jsonState.empty() && StateRefreshPolicy::SOMETIMES == stateInfo->refreshPolicy) {
            /*
             * If jsonState supplied by the state provider is empty and it has a refreshPolicy of SOMETIMES, it means
             * that it doesn't want to provide state.
             */
            ACSDK_DEBUG9(LX("buildContextIgnored").d("namespace", it->first.nameSpace).d("name", it->first.name));
            continue;
        }
        Value jsonState = buildState(it->first, stateInfo->jsonState, allocator);
        if (jsonState.ObjectEmpty()) {
            ACSDK_ERROR(LX("buildContextFailed").d("reason", "buildStateFailed"));
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
            ACSDK_ERROR(LX("buildContextFailed").d("reason", "convertingJsonToStringFailed"));
            errorBuildingContext = true;
        }
    }

    if (errorBuildingContext) {
        sendContextAndClearQueue("", ContextRequestError::BUILD_CONTEXT_ERROR);
    } else {
        ACSDK_DEBUG(LX("buildContextSuccessful").sensitive("context", jsonContextBuf.GetString()));
        sendContextAndClearQueue(jsonContextBuf.GetString());
    }
}

}  // namespace contextManager
}  // namespace alexaClientSDK
