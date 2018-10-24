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

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h"
#include "AVSCommon/AVS/CapabilityAgent.h"
#include "AVSCommon/AVS/EventBuilder.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace rapidjson;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("CapabilityAgent");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<CapabilityAgent::DirectiveInfo> CapabilityAgent::createDirectiveInfo(
    std::shared_ptr<AVSDirective> directive,
    std::unique_ptr<sdkInterfaces::DirectiveHandlerResultInterface> result) {
    return std::make_shared<DirectiveInfo>(directive, std::move(result));
}

CapabilityAgent::CapabilityAgent(
    const std::string& nameSpace,
    std::shared_ptr<sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender) :
        m_namespace{nameSpace},
        m_exceptionEncounteredSender{exceptionEncounteredSender} {
}

CapabilityAgent::DirectiveInfo::DirectiveInfo(
    std::shared_ptr<AVSDirective> directiveIn,
    std::unique_ptr<sdkInterfaces::DirectiveHandlerResultInterface> resultIn) :
        directive{directiveIn},
        result{std::move(resultIn)},
        isCancelled{false} {
}

void CapabilityAgent::preHandleDirective(
    std::shared_ptr<AVSDirective> directive,
    std::unique_ptr<DirectiveHandlerResultInterface> result) {
    std::string messageId = directive->getMessageId();
    auto info = getDirectiveInfo(messageId);
    if (info) {
        static const std::string error{"messageIdIsAlreadyInUse"};
        ACSDK_ERROR(LX("preHandleDirectiveFailed").d("reason", error).d("messageId", messageId));
        result->setFailed(error);
        if (m_exceptionEncounteredSender) {
            m_exceptionEncounteredSender->sendExceptionEncountered(
                directive->getUnparsedDirective(), ExceptionErrorType::INTERNAL_ERROR, error);
        }
        return;
    }
    ACSDK_DEBUG(LX("addingMessageIdToMap").d("messageId", messageId));
    info = createDirectiveInfo(directive, std::move(result));
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_directiveInfoMap[messageId] = info;
    }
    preHandleDirective(info);
}

bool CapabilityAgent::handleDirective(const std::string& messageId) {
    auto info = getDirectiveInfo(messageId);
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "messageIdNotFound").d("messageId", messageId));
        return false;
    }
    handleDirective(info);
    return true;
}

void CapabilityAgent::cancelDirective(const std::string& messageId) {
    auto info = getDirectiveInfo(messageId);
    if (!info) {
        ACSDK_ERROR(LX("cancelDirectiveFailed").d("reason", "messageIdNotFound").d("messageId", messageId));
        return;
    }
    /*
     * We mark this directive as cancelled so the @c CapabilityAgent can use this flag to handle directives that've
     * already been handled, but are not yet complete.
     */
    info->isCancelled = true;
    cancelDirective(info);
}

void CapabilityAgent::sendExceptionEncounteredAndReportFailed(
    std::shared_ptr<DirectiveInfo> info,
    const std::string& message,
    avsCommon::avs::ExceptionErrorType type) {
    if (info) {
        if (info->directive) {
            m_exceptionEncounteredSender->sendExceptionEncountered(
                info->directive->getUnparsedDirective(), type, message);
            removeDirective(info->directive->getMessageId());
        } else {
            ACSDK_ERROR(LX("sendExceptionEncounteredAndReportFailed").d("reason", "infoHasNoDirective"));
        }
        if (info->result) {
            info->result->setFailed(message);
        } else {
            ACSDK_ERROR(LX("sendExceptionEncounteredAndReportFailed").d("reason", "infoHasNoResult"));
        }
    } else {
        ACSDK_ERROR(LX("sendExceptionEncounteredAndReportFailed").d("reason", "infoNotFound"));
    }
}

void CapabilityAgent::onDeregistered() {
    // default no op
}

void CapabilityAgent::removeDirective(const std::string& messageId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ACSDK_DEBUG(LX("removingMessageIdFromMap").d("messageId", messageId));
    m_directiveInfoMap.erase(messageId);
}

void CapabilityAgent::onFocusChanged(FocusState) {
    // default no-op
}

void CapabilityAgent::provideState(const avsCommon::avs::NamespaceAndName& stateProviderName, const unsigned int) {
    // default no-op
}

void CapabilityAgent::onContextAvailable(const std::string&) {
    // default no-op
}

void CapabilityAgent::onContextFailure(const sdkInterfaces::ContextRequestError) {
    // default no-op
}

const std::pair<std::string, std::string> CapabilityAgent::buildJsonEventString(
    const std::string& eventName,
    const std::string& dialogRequestIdString,
    const std::string& payload,
    const std::string& context) {
    return avs::buildJsonEventString(m_namespace, eventName, dialogRequestIdString, payload, context);
}

std::shared_ptr<CapabilityAgent::DirectiveInfo> CapabilityAgent::getDirectiveInfo(const std::string& messageId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_directiveInfoMap.find(messageId);
    if (it != m_directiveInfoMap.end()) {
        return it->second;
    }
    return nullptr;
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
