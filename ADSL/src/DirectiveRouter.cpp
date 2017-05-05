/*
 * DirectiveRouter.cpp
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

#include <iostream>
#include <set>
#include <sstream>
#include <vector>

#include <AVSUtils/Logger/LogEntry.h>
#include <AVSUtils/Logging/Logger.h>

#include "ADSL/DirectiveRouter.h"

/// String to identify log entries originating from this file.
static const std::string TAG("DirectiveRouter");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) ::alexaClientSDK::avsUtils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace adsl {

DirectiveRouter::~DirectiveRouter() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto item : m_handlerReferenceCounts) {
        item.first->onDeregistered();
    }
}

bool DirectiveRouter::addDirectiveHandlers(const DirectiveHandlerConfiguration& configuration) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto item : configuration) {
        if (!item.second) {
            ACSDK_ERROR(LX("addDirectiveHandlersFailed")
                    .d("reason", "emptyHandlerAndPolicy")
                    .d("namespace", item.first.nameSpace)
                    .d("name", item.first.name));
            return false;
        }
        auto it = m_configuration.find(item.first);
        if (m_configuration.end() != it) {
            ACSDK_ERROR(LX("addDirectiveHandlersFailed")
                    .d("reason", "alreadySet")
                    .d("namespace", item.first.nameSpace)
                    .d("name", item.first.name));
            return false;
        }
    }

    for (auto item : configuration) {
        m_configuration[item.first] = item.second;
        incrementHandlerReferenceCountLocked(item.second.handler);
        ACSDK_INFO(LX("addDirectiveHandlers")
                .d("action", "added")
                .d("namespace", item.first.nameSpace)
                .d("name", item.first.name)
                .d("handler", item.second.handler.get())
                .d("policy", item.second.policy));
    }

    return true;
}

bool DirectiveRouter::removeDirectiveHandlers(const DirectiveHandlerConfiguration& configuration) {
    std::unique_lock<std::mutex> lock(m_mutex);

    for (auto item : configuration) {
        auto it = m_configuration.find(item.first);
        if (m_configuration.end() == it || it->second != item.second) {
            ACSDK_ERROR(LX("removeDirectiveHandlersFailed")
                    .d("reason", "notFound")
                    .d("namespace", item.first.nameSpace)
                    .d("name", item.first.name)
                    .d("handler", item.second.handler.get())
                    .d("policy", item.second.policy));
            return false;
        }
    }

    /**
     * Decrement reference counts. Unfortunately, a simple loop calling @c decrementHandlerReferenceCountLocked()
     * would create a race condition because that function temporarily releases @c m_mutex when a count goes to zero.
     * Instead, the operation is expanded here with the lock released once we know which handlers to notify.
     */
    std::vector<std::shared_ptr<DirectiveHandlerInterface>> releasedHandlers;
    for (auto item : configuration) {
        m_configuration.erase(item.first);
        ACSDK_INFO(LX("removeDirectiveHandlers")
                .d("action", "removed")
                .d("namespace", item.first.nameSpace)
                .d("name", item.first.name)
                .d("handler", item.second.handler.get())
                .d("policy", item.second.policy));
        auto it = m_handlerReferenceCounts.find(item.second.handler);
        if (0 == --(it->second)) {
            releasedHandlers.push_back(item.second.handler);
            m_handlerReferenceCounts.erase(it);
        }
    }
    lock.unlock();
    for (auto handler : releasedHandlers) {
        ACSDK_INFO(LX("onDeregisteredCalled").d("handler", handler.get()));
        handler->onDeregistered();
    }

    return true;
}

bool DirectiveRouter::handleDirectiveImmediately(std::shared_ptr<avsCommon::AVSDirective> directive) {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto handlerAndPolicy = getHandlerAndPolicyLocked(directive);
    if (!handlerAndPolicy) {
        ACSDK_WARN(LX("handleDirectiveImmediatelyFailed")
                .d("messageId", directive->getMessageId()).d("reason", "noHandlerRegistered"));
        return false;
    }
    ACSDK_INFO(LX("handleDirectiveImmediately").d("messageId", directive->getMessageId()).d("action", "calling"));
    HandlerCallScope scope(lock, this, handlerAndPolicy.handler);
    handlerAndPolicy.handler->handleDirectiveImmediately(directive);
    return true;
}

bool DirectiveRouter::preHandleDirective(
        std::shared_ptr<avsCommon::AVSDirective> directive,
        std::unique_ptr<DirectiveHandlerResultInterface> result) {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto handlerAndPolicy = getHandlerAndPolicyLocked(directive);
    if (!handlerAndPolicy) {
        ACSDK_WARN(LX("preHandleDirectiveFailed")
                .d("messageId", directive->getMessageId()).d("reason", "noHandlerRegistered"));
        return false;
    }
    ACSDK_INFO(LX("preHandleDirective").d("messageId", directive->getMessageId()).d("action", "calling"));
    HandlerCallScope scope(lock, this, handlerAndPolicy.handler);
    handlerAndPolicy.handler->preHandleDirective(directive, std::move(result));
    return true;
}

bool DirectiveRouter::handleDirective(std::shared_ptr<avsCommon::AVSDirective> directive, BlockingPolicy* policyOut) {
    if (!policyOut) {
        ACSDK_ERROR(LX("handleDirectiveFailed")
                .d("messageId", directive->getMessageId()).d("reason", "nullptrPolicyOut"));
        return false;
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    auto handlerAndPolicy = getHandlerAndPolicyLocked(directive);
    if (!handlerAndPolicy) {
        ACSDK_WARN(LX("handleDirectiveFailed")
                .d("messageId", directive->getMessageId()).d("reason", "noHandlerRegistered"));
        return false;
    }
    ACSDK_INFO(LX("handleDirective").d("messageId", directive->getMessageId()).d("action", "calling"));
    HandlerCallScope scope(lock, this, handlerAndPolicy.handler);
    auto result = handlerAndPolicy.handler->handleDirective(directive->getMessageId());
    if (result) {
        *policyOut = handlerAndPolicy.policy;
    } else {
        ACSDK_WARN(LX("messageIdNotRecognized")
                .d("handler", handlerAndPolicy.handler.get())
                .d("messageId", directive->getMessageId())
                .d("reason", "handleDirectiveReturnedFalse"));
    }
    return result;
}


bool DirectiveRouter::cancelDirective(std::shared_ptr<avsCommon::AVSDirective> directive) {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto handlerAndPolicy = getHandlerAndPolicyLocked(directive);
    if (!handlerAndPolicy) {
        ACSDK_WARN(LX("cancelDirectiveFailed")
                .d("messageId", directive->getMessageId()).d("reason", "noHandlerRegistered"));
        return false;
    }
    ACSDK_INFO(LX("cancelDirective").d("messageId", directive->getMessageId()).d("action", "calling"));
    HandlerCallScope scope(lock, this, handlerAndPolicy.handler);
    handlerAndPolicy.handler->cancelDirective(directive->getMessageId());
    return true;
}

DirectiveRouter::HandlerCallScope::HandlerCallScope(
        std::unique_lock<std::mutex>& lock,
        DirectiveRouter* router,
        std::shared_ptr<DirectiveHandlerInterface> handler) :
        // Parenthesis are used for initializing @c m_lock to work-around a bug in the C++ specification.  see:
        // http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1288
        m_lock(lock), m_router{router}, m_handler{handler} {
    m_router->incrementHandlerReferenceCountLocked(m_handler);
    m_lock.unlock();
}

DirectiveRouter::HandlerCallScope::~HandlerCallScope() {
    m_lock.lock();
    m_router->decrementHandlerReferenceCountLocked(m_lock, m_handler);
}

HandlerAndPolicy DirectiveRouter::getHandlerAndPolicyLocked(std::shared_ptr<avsCommon::AVSDirective> directive) {
    if (!directive) {
        ACSDK_WARN(LX("getHandlerAndPolicyLockedFailed").d("reason", "nullptrDirective"));
        return HandlerAndPolicy();
    }
    auto it = m_configuration.find(NamespaceAndName(directive->getNamespace(), directive->getName()));
    if (m_configuration.end() == it) {
        return HandlerAndPolicy();
    }
    return it->second;
}

void DirectiveRouter::incrementHandlerReferenceCountLocked(std::shared_ptr<DirectiveHandlerInterface> handler){
    const auto it = m_handlerReferenceCounts.find(handler);
    if (it != m_handlerReferenceCounts.end()) {
        it->second++;
    } else {
        m_handlerReferenceCounts[handler] = 1;
    }
}

void DirectiveRouter::decrementHandlerReferenceCountLocked(
        std::unique_lock<std::mutex>& lock, std::shared_ptr<DirectiveHandlerInterface> handler) {
    const auto it = m_handlerReferenceCounts.find(handler);
    if (it != m_handlerReferenceCounts.end()) {
        if (0 == --(it->second)) {
            m_handlerReferenceCounts.erase(it);
            ACSDK_INFO(LX("onDeregisteredCalled").d("handler", handler.get()));
            lock.unlock();
            handler->onDeregistered();
            lock.lock();
        }
    } else {
        ACSDK_ERROR(LX("removeHandlerReferenceLockedFailed").d("reason", "handlerNotFound"));
    }
}

} // namespace adsl
} // namespace alexaClientSDK