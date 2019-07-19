/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "System/RevokeAuthorizationHandler.h"

#include <string>
#include <rapidjson/document.h>

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG{"RevokeAuthorizationHandler"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// This string holds the namespace for AVS revoke.
static const std::string REVOKE_NAMESPACE = "System";

/// This string holds the name of the directive that's being sent for revoking.
static const std::string REVOKE_DIRECTIVE_NAME = "RevokeAuthorization";

void RevokeAuthorizationHandler::removeDirectiveGracefully(
    std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
    bool isFailure,
    const std::string& report) {
    if (info) {
        if (info->result) {
            if (isFailure) {
                info->result->setFailed(report);
            } else {
                info->result->setCompleted();
            }
            if (info->directive) {
                CapabilityAgent::removeDirective(info->directive->getMessageId());
            }
        }
    }
}

std::shared_ptr<RevokeAuthorizationHandler> RevokeAuthorizationHandler::create(
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender) {
    ACSDK_DEBUG5(LX(__func__));
    if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionEncounteredSender"));
        return nullptr;
    }
    return std::shared_ptr<RevokeAuthorizationHandler>(new RevokeAuthorizationHandler(exceptionEncounteredSender));
}

RevokeAuthorizationHandler::RevokeAuthorizationHandler(
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender) :
        CapabilityAgent(REVOKE_NAMESPACE, exceptionEncounteredSender) {
}

avsCommon::avs::DirectiveHandlerConfiguration RevokeAuthorizationHandler::getConfiguration() const {
    return avsCommon::avs::DirectiveHandlerConfiguration{
        {NamespaceAndName{REVOKE_NAMESPACE, REVOKE_DIRECTIVE_NAME},
         avsCommon::avs::BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false)}};
}

void RevokeAuthorizationHandler::preHandleDirective(
    std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
}

void RevokeAuthorizationHandler::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    handleDirective(std::make_shared<avsCommon::avs::CapabilityAgent::DirectiveInfo>(directive, nullptr));
}

void RevokeAuthorizationHandler::handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveOrDirectiveInfo"));
        return;
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    auto observers = m_revokeObservers;
    lock.unlock();

    for (auto observer : observers) {
        observer->onRevokeAuthorization();
    }

    removeDirectiveGracefully(info);
}

void RevokeAuthorizationHandler::cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
    if (!info || !info->directive) {
        removeDirectiveGracefully(info, true, "nullDirective");
        ACSDK_ERROR(LX("cancelDirectiveFailed").d("reason", "nullDirectiveOrDirectiveInfo"));
        return;
    }
    CapabilityAgent::removeDirective(info->directive->getMessageId());
}

bool RevokeAuthorizationHandler::addObserver(std::shared_ptr<RevokeAuthorizationObserverInterface> observer) {
    ACSDK_DEBUG5(LX(__func__));
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_revokeObservers.insert(observer).second;
}

bool RevokeAuthorizationHandler::removeObserver(std::shared_ptr<RevokeAuthorizationObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_revokeObservers.erase(observer) == 1;
}

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
