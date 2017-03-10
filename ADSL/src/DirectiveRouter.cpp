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

#include <AVSUtils/Logger/LogEntry.h>
#include <AVSUtils/Logging/Logger.h>
#include "ADSL/DirectiveRouter.h"

/// String used to identify log entries that originated from this file.
static const std::string TAG("DirectiveRouter");

/// Macro to create a LogEntry in-line using the TAG for this file.
#define LX(event) ::alexaClientSDK::avsUtils::logger::LogEntry(TAG, event)

// TODO: ACSDK-179 Remove this (and migrate invocations of this to ACSDK_<LEVEL> invocations.
#define ACSDK_LOG(expression)                                           \
    do {                                                                \
        ::alexaClientSDK::avsUtils::Logger::log(expression.c_str());    \
    } while (false)


namespace alexaClientSDK {
namespace adsl {

std::set<std::shared_ptr<DirectiveHandlerInterface>> DirectiveRouter::setDirectiveHandlers(
        const DirectiveHandlerConfiguration& configuration) {

    std::set<std::shared_ptr<DirectiveHandlerInterface>> removedHandlers;

    for (auto entry : configuration) {
        auto it = m_configuration.find(entry.first);
        if (!entry.second.first || entry.second.second == BlockingPolicy::NONE) {
            if (m_configuration.end() == it) {
                ACSDK_LOG(LX("setDirectiveHandlers").d("action", "ignoringRemovalOfDirectiveHandler")
                        .d("namespace", entry.first.first).d("name", entry.first.second).d("reason", "notFound"));
            } else {
                ACSDK_LOG(LX("setDirectiveHandlers").d("action", "removingDirectiveHandler")
                        .d("namespace", entry.first.first).d("name", entry.first.second));
                removedHandlers.insert(it->second.first);
                m_configuration.erase(it);
            }
        } else {
            if (m_configuration.end() == it) {
                ACSDK_LOG(LX("setDirectiveHandlers").d("action", "addingDirectiveHandler")
                        .d("namespace", entry.first.first).d("name", entry.first.second));
                m_configuration[entry.first] = entry.second;
            } else if (it->second != entry.second) {
                if (it->second.first != entry.second.first) {
                    removedHandlers.insert(it->second.first);
                }
                ACSDK_LOG(LX("setDirectiveHandlers").d("action","changingDirectiveHandler")
                        .d("namespace", entry.first.first).d("name", entry.first.second));
                it->second = entry.second;
            }
        }
    }

    // Reduce removedHandlers to just those that have been completely removed.  That is the set returned.
    for (auto entry : m_configuration) {
        auto it = removedHandlers.find(entry.second.first);
        if (removedHandlers.end() != it) {
            removedHandlers.erase(it);
        }
    }

    ACSDK_LOG(LX("setDirectiveHandlers").d("action","removedHandlers").d("count", removedHandlers.size()));
    return removedHandlers;
}

DirectiveHandlerAndBlockingPolicyPair DirectiveRouter::getDirectiveHandlerAndBlockingPolicy(
        std::shared_ptr<avsCommon::AVSDirective> directive) {
    if (!directive) {
        ACSDK_LOG(LX("getDirectiveHandlerAndBlockingPolicy() failed.").d("directive", directive));
        return DirectiveHandlerAndBlockingPolicyPair{nullptr, BlockingPolicy::NONE};
    }
    auto it = m_configuration.find(NamespaceAndNamePair(directive->getNamespace(), directive->getName()));
    if (m_configuration.end() == it) {
        return DirectiveHandlerAndBlockingPolicyPair{nullptr, BlockingPolicy::NONE};
    }
    return it->second;
}

std::set<std::shared_ptr<DirectiveHandlerInterface>> DirectiveRouter::clear() {
    ACSDK_LOG(LX("clear"));
    std::set<std::shared_ptr<DirectiveHandlerInterface>> removedHandlers;
    for (auto entry : m_configuration) {
        removedHandlers.insert(entry.second.first);
    }
    m_configuration.clear();
    return removedHandlers;
}

} // namespace adsl
} // namespace alexaClientSDK