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

#include <iostream>
#include <set>
#include <sstream>
#include <vector>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "ADSL/DirectiveRouter.h"

/// String to identify log entries originating from this file.
static const std::string TAG("DirectiveRouter");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace adsl {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;

DirectiveRouter::DirectiveRouter() : RequiresShutdown{"DirectiveRouter"} {
}

bool DirectiveRouter::addDirectiveHandler(std::shared_ptr<DirectiveHandlerInterface> handler) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (isShutdown()) {
        ACSDK_ERROR(LX("addDirectiveHandlersFailed").d("reason", "isShutdown"));
        return false;
    }

    if (!handler) {
        ACSDK_ERROR(LX("addDirectiveHandlersFailed").d("reason", "emptyHandler"));
        return false;
    }

    auto configuration = handler->getConfiguration();
    for (auto item : configuration) {
        if (!(item.second.isValid())) {
            ACSDK_ERROR(LX("addDirectiveHandlersFailed").d("reason", "nonePolicy"));
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
        HandlerAndPolicy handlerAndPolicy(handler, item.second);
        m_configuration[item.first] = handlerAndPolicy;
        incrementHandlerReferenceCountLocked(handler);
        ACSDK_DEBUG9(LX("addDirectiveHandlers")
                         .d("action", "added")
                         .d("namespace", item.first.nameSpace)
                         .d("name", item.first.name)
                         .d("handler", handler.get())
                         .d("policy", item.second));
    }

    return true;
}

bool DirectiveRouter::removeDirectiveHandlerLocked(std::shared_ptr<DirectiveHandlerInterface> handler) {
    if (!handler) {
        ACSDK_ERROR(LX("removeDirectiveHandlersFailed").d("reason", "nullptrHandler"));
        return false;
    }

    auto configuration = handler->getConfiguration();
    for (auto item : configuration) {
        auto it = m_configuration.find(item.first);
        if (m_configuration.end() == it || it->second != HandlerAndPolicy(handler, item.second)) {
            ACSDK_ERROR(LX("removeDirectiveHandlersFailed")
                            .d("reason", "notFound")
                            .d("namespace", item.first.nameSpace)
                            .d("name", item.first.name)
                            .d("handler", handler.get())
                            .d("policy", item.second));
            return false;
        }
    }

    /**
     * Decrement reference counts. Unfortunately, a simple loop calling @c decrementHandlerReferenceCountLocked()
     * would create a race condition because that function temporarily releases @c m_mutex when a count goes to zero.
     * Instead, the operation is expanded here with the lock released once we know which handlers to notify.
     */
    for (auto item : configuration) {
        m_configuration.erase(item.first);
        ACSDK_DEBUG9(LX("removeDirectiveHandlers")
                         .d("action", "removed")
                         .d("namespace", item.first.nameSpace)
                         .d("name", item.first.name)
                         .d("handler", handler.get())
                         .d("policy", item.second));
        auto it = m_handlerReferenceCounts.find(handler);
        if (0 == --(it->second)) {
            m_handlerReferenceCounts.erase(it);
        }
    }

    return true;
}

bool DirectiveRouter::removeDirectiveHandler(std::shared_ptr<DirectiveHandlerInterface> handler) {
    std::unique_lock<std::mutex> lock(m_mutex);

    if (!removeDirectiveHandlerLocked(handler)) {
        return false;
    }

    lock.unlock();
    ACSDK_DEBUG9(LX("onDeregisteredCalled").d("handler", handler.get()));
    handler->onDeregistered();

    return true;
}

bool DirectiveRouter::handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto handlerAndPolicy = getdHandlerAndPolicyLocked(directive);
    if (!handlerAndPolicy) {
        ACSDK_WARN(LX("handleDirectiveImmediatelyFailed")
                       .d("messageId", directive->getMessageId())
                       .d("reason", "noHandlerRegistered"));
        return false;
    }
    ACSDK_INFO(LX("handleDirectiveImmediately").d("messageId", directive->getMessageId()).d("action", "calling"));
    HandlerCallScope scope(lock, this, handlerAndPolicy.handler);
    handlerAndPolicy.handler->handleDirectiveImmediately(directive);
    return true;
}

bool DirectiveRouter::preHandleDirective(
    std::shared_ptr<avsCommon::avs::AVSDirective> directive,
    std::unique_ptr<DirectiveHandlerResultInterface> result) {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto handler = getHandlerLocked(directive);
    if (!handler) {
        ACSDK_WARN(LX("preHandleDirectiveFailed")
                       .d("messageId", directive->getMessageId())
                       .d("reason", "noHandlerRegistered"));
        return false;
    }
    ACSDK_INFO(LX("preHandleDirective").d("messageId", directive->getMessageId()).d("action", "calling"));
    HandlerCallScope scope(lock, this, handler);
    handler->preHandleDirective(directive, std::move(result));
    return true;
}

bool DirectiveRouter::handleDirective(const std::shared_ptr<AVSDirective>& directive) {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto handler = getHandlerLocked(directive);
    if (!handler) {
        ACSDK_WARN(
            LX("handleDirectiveFailed").d("messageId", directive->getMessageId()).d("reason", "noHandlerRegistered"));
        return false;
    }
    ACSDK_INFO(LX("handleDirective").d("messageId", directive->getMessageId()).d("action", "calling"));
    HandlerCallScope scope(lock, this, handler);
    auto result = handler->handleDirective(directive->getMessageId());
    if (!result) {
        ACSDK_WARN(LX("messageIdNotRecognized")
                       .d("handler", handler.get())
                       .d("messageId", directive->getMessageId())
                       .d("reason", "handleDirectiveReturnedFalse"));
    }
    return result;
}

bool DirectiveRouter::cancelDirective(std::shared_ptr<avsCommon::avs::AVSDirective> directive) {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto handler = getHandlerLocked(directive);
    if (!handler) {
        ACSDK_WARN(
            LX("cancelDirectiveFailed").d("messageId", directive->getMessageId()).d("reason", "noHandlerRegistered"));
        return false;
    }
    ACSDK_INFO(LX("cancelDirective").d("messageId", directive->getMessageId()).d("action", "calling"));
    HandlerCallScope scope(lock, this, handler);
    handler->cancelDirective(directive->getMessageId());
    return true;
}

void DirectiveRouter::doShutdown() {
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface>> releasedHandlers;
    std::unique_lock<std::mutex> lock(m_mutex);

    // Should remove all configurations cleanly.
    size_t numConfigurations = m_configuration.size();
    for (size_t i = 0; i < numConfigurations && !m_configuration.empty(); ++i) {
        auto handler = m_configuration.begin()->second.handler;

        if (removeDirectiveHandlerLocked(handler)) {
            releasedHandlers.push_back(handler);
        }
    }

    // For good measure
    m_configuration.clear();

    lock.unlock();

    for (auto releasedHandler : releasedHandlers) {
        ACSDK_DEBUG9(LX("onDeregisteredCalled").d("handler", releasedHandler.get()));
        releasedHandler->onDeregistered();
    }
}

DirectiveRouter::HandlerCallScope::HandlerCallScope(
    std::unique_lock<std::mutex>& lock,
    DirectiveRouter* router,
    std::shared_ptr<DirectiveHandlerInterface> handler) :
        // Parenthesis are used for initializing @c m_lock to work-around a bug in the C++ specification.  see:
        // http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1288
        m_lock(lock),
        m_router{router},
        m_handler{handler} {
    m_router->incrementHandlerReferenceCountLocked(m_handler);
    m_lock.unlock();
}

DirectiveRouter::HandlerCallScope::~HandlerCallScope() {
    m_lock.lock();
    m_router->decrementHandlerReferenceCountLocked(m_lock, m_handler);
}

BlockingPolicy DirectiveRouter::getPolicy(const std::shared_ptr<AVSDirective>& directive) {
    std::unique_lock<std::mutex> lock(m_mutex);

    return getdHandlerAndPolicyLocked(directive).policy;
}

HandlerAndPolicy DirectiveRouter::getdHandlerAndPolicyLocked(const std::shared_ptr<AVSDirective>& directive) {
    if (!directive) {
        ACSDK_ERROR(LX("getConfiguredHandlerAndPolicyLockedFailed").d("reason", "nullptrDirective"));
        return HandlerAndPolicy();
    }

    // First, look for an exact match.  If not found, then look for a wildcard handler for the AVS namespace.
    auto it = m_configuration.find(NamespaceAndName(directive->getNamespace(), directive->getName()));
    if (m_configuration.end() == it) {
        it = m_configuration.find(NamespaceAndName(directive->getNamespace(), "*"));
    }

    if (m_configuration.end() == it) {
        return HandlerAndPolicy();
    }

    return it->second;
}

std::shared_ptr<DirectiveHandlerInterface> DirectiveRouter::getHandlerLocked(std::shared_ptr<AVSDirective> directive) {
    auto handlerAndPolicy = getdHandlerAndPolicyLocked(directive);
    if (!handlerAndPolicy) {
        ACSDK_DEBUG0(
            LX("noHandlerFoundForDirective").d("namespace", directive->getNamespace()).d("name", directive->getName()));
        return nullptr;
    }

    return handlerAndPolicy.handler;
}

void DirectiveRouter::incrementHandlerReferenceCountLocked(std::shared_ptr<DirectiveHandlerInterface> handler) {
    const auto it = m_handlerReferenceCounts.find(handler);
    if (it != m_handlerReferenceCounts.end()) {
        it->second++;
    } else {
        m_handlerReferenceCounts[handler] = 1;
    }
}

void DirectiveRouter::decrementHandlerReferenceCountLocked(
    std::unique_lock<std::mutex>& lock,
    std::shared_ptr<DirectiveHandlerInterface> handler) {
    const auto it = m_handlerReferenceCounts.find(handler);
    if (it != m_handlerReferenceCounts.end()) {
        if (0 == --(it->second)) {
            m_handlerReferenceCounts.erase(it);
            ACSDK_DEBUG9(LX("onDeregisteredCalled").d("handler", handler.get()));
            lock.unlock();
            handler->onDeregistered();
            lock.lock();
        }
    } else {
        ACSDK_ERROR(LX("removeHandlerReferenceLockedFailed").d("reason", "handlerNotFound"));
    }
}

}  // namespace adsl
}  // namespace alexaClientSDK
