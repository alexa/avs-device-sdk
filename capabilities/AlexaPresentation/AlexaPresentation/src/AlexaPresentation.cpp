/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     https://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <ostream>

#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdk/AlexaPresentation/private/AlexaPresentation.h"

namespace alexaClientSDK {
namespace alexaPresentation {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::json;

/// String to identify log entries originating from this file.
#define TAG "AlexaPresentation"
/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The name for Presentation Dismissed event.
static const char* PRESENTATION_DISMISSED = "Dismissed";

/// AlexaPresentation interface type
static const char* ALEXAPRESENTATION_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";

/// AlexaPresentation interface name
static const char* ALEXAPRESENTATION_CAPABILITY_INTERFACE_NAME = "Alexa.Presentation";

/// AlexaPresentation interface version
static const char* ALEXAPRESENTATION_CAPABILITY_INTERFACE_VERSION = "1.0";

/// Namespace supported by Alexa presentation capability agent.
static const char* ALEXA_PRESENTATION_NAMESPACE = "Alexa.Presentation";

std::shared_ptr<AlexaPresentation> AlexaPresentation::create(
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) {
    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    }

    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    }

    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManagerInterface"));
        return nullptr;
    }

    std::shared_ptr<AlexaPresentation> alexaPresentation(
        new AlexaPresentation(exceptionSender, messageSender, contextManager));

    return alexaPresentation;
}

void AlexaPresentation::setExecutor(
    const std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor>& executor) {
    ACSDK_WARN(LX(__func__).d("reason", "should be called in test only"));
    m_executor = executor;
}

void AlexaPresentation::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void AlexaPresentation::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("preHandleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
}

void AlexaPresentation::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    handleUnknownDirective(info);
}

void AlexaPresentation::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    removeDirective(info);
}

DirectiveHandlerConfiguration AlexaPresentation::getConfiguration() const {
    DirectiveHandlerConfiguration configuration;
    return configuration;
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AlexaPresentation::
    getCapabilityConfigurations() {
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>>
        capabilityConfigurations;
    std::unordered_map<std::string, std::string> configMap;

    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, ALEXAPRESENTATION_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, ALEXAPRESENTATION_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, ALEXAPRESENTATION_CAPABILITY_INTERFACE_VERSION});

    capabilityConfigurations.insert(std::make_shared<CapabilityConfiguration>(configMap));
    return capabilityConfigurations;
}

void AlexaPresentation::onPresentationDismissed(const aplCapabilityCommonInterfaces::PresentationToken& token) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this, token]() {
        ACSDK_DEBUG5(LX("onPresentationDismissedExecutor"));
        // Assemble the event payload.
        std::ostringstream payload;
        payload << R"({"presentationToken":")" << token << R"("})";
        executeSendEvent(ALEXA_PRESENTATION_NAMESPACE, PRESENTATION_DISMISSED, payload.str());
    });
}

AlexaPresentation::AlexaPresentation(
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) :
        CapabilityAgent{ALEXA_PRESENTATION_NAMESPACE, exceptionSender},
        RequiresShutdown{ALEXA_PRESENTATION_NAMESPACE},
        m_messageSender{messageSender},
        m_contextManager{contextManager} {
    m_executor = std::make_shared<alexaClientSDK::avsCommon::utils::threading::Executor>();
}

void AlexaPresentation::doShutdown() {
    m_executor->shutdown();
    m_messageSender.reset();
    m_contextManager.reset();
}

void AlexaPresentation::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    /*
     * Check result too, to catch cases where DirectiveInfo was created locally, without a nullptr result.
     * In those cases there is no messageId to remove because no result was expected.
     */
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void AlexaPresentation::setHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (info) {
        if (info->result) {
            info->result->setCompleted();
        } else {
            sendExceptionEncounteredAndReportFailed(
                info, "Unable to complete directive", ExceptionErrorType::INTERNAL_ERROR);
        }
        removeDirective(info);
    }
}

void AlexaPresentation::handleUnknownDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info) {
        ACSDK_ERROR(LX("handleUnknownDirective").m("nullDirectiveInfo"));
        sendExceptionEncounteredAndReportFailed(
            info, "nullDirectiveInfo", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return;
    }
    ACSDK_ERROR(LX("requestedToHandleUnknownDirective")
                    .d("reason", "unknownDirective")
                    .d("namespace", info->directive->getNamespace())
                    .d("name", info->directive->getName()));
    m_executor->submit([this, info] {
        const std::string exceptionMessage =
            "unexpected directive " + info->directive->getNamespace() + ":" + info->directive->getName();

        sendExceptionEncounteredAndReportFailed(
            info, exceptionMessage, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
    });
}

void AlexaPresentation::executeSendEvent(
    const std::string& avsNamespace,
    const std::string& name,
    const std::string& payload) {
    ACSDK_DEBUG9(LX(__func__));
    m_events.push(std::make_tuple(avsNamespace, name, payload));
    m_contextManager->getContext(shared_from_this());
}

void AlexaPresentation::onContextAvailable(const std::string& jsonContext) {
    auto task = [this, jsonContext]() {
        ACSDK_DEBUG9(LX("onContextAvailableExecutor"));

        if (!m_events.empty()) {
            auto event = m_events.front();

            auto msgIdAndJsonEvent = avsCommon::avs::buildJsonEventString(
                std::get<0>(event), std::get<1>(event), "", std::get<2>(event), jsonContext);
            auto userEventMessage = std::make_shared<avsCommon::avs::MessageRequest>(msgIdAndJsonEvent.second);
            ACSDK_DEBUG9(LX("Sending event to AVS").d("namespace", std::get<0>(event)).d("name", std::get<1>(event)));
            m_messageSender->sendMessage(userEventMessage);

            m_events.pop();
        }
    };

    m_executor->submit(task);
}

void AlexaPresentation::onContextFailure(const ContextRequestError error) {
    ACSDK_ERROR(LX(__func__).d("reason", "contextRequestErrorOccurred").d("error", error));
}

}  // namespace alexaPresentation
}  // namespace alexaClientSDK
