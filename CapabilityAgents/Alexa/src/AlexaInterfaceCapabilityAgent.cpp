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

#include "Alexa/AlexaInterfaceCapabilityAgent.h"
#include "Alexa/AlexaInterfaceConstants.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alexa {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG{"AlexaInterfaceCapabilityAgent"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// This string holds the namespace for the AlexaInterface directives.
static const std::string NAMESPACE = ALEXA_INTERFACE_NAME;

/// The Alexa.EventProcessed directive name.
static const std::string EVENT_PROCESSED_DIRECTIVE_NAME = "EventProcessed";

/// The Alexa.ReportState directive name.
static const std::string REPORT_STATE_DIRECTIVE_NAME = "ReportState";

std::shared_ptr<AlexaInterfaceCapabilityAgent> AlexaInterfaceCapabilityAgent::create(
    const avsCommon::utils::DeviceInfo& deviceInfo,
    const endpoints::EndpointIdentifier& endpointId,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<AlexaInterfaceMessageSenderInternalInterface> alexaMessageSender) {
    ACSDK_DEBUG5(LX(__func__));
    if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
    } else if (!alexaMessageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAlexaMessageSender"));
    } else {
        auto instance = std::shared_ptr<AlexaInterfaceCapabilityAgent>(
            new AlexaInterfaceCapabilityAgent(deviceInfo, endpointId, exceptionEncounteredSender, alexaMessageSender));
        return instance;
    }
    return nullptr;
}

AlexaInterfaceCapabilityAgent::AlexaInterfaceCapabilityAgent(
    const avsCommon::utils::DeviceInfo& deviceInfo,
    const endpoints::EndpointIdentifier& endpointId,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<AlexaInterfaceMessageSenderInternalInterface> alexaMessageSender) :
        CapabilityAgent{NAMESPACE, exceptionEncounteredSender},
        m_deviceInfo{deviceInfo},
        m_endpointId{endpointId},
        m_alexaMessageSender{alexaMessageSender} {
}

DirectiveHandlerConfiguration AlexaInterfaceCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));
    DirectiveHandlerConfiguration configuration;

    // If this is the default endpoint, register for Alexa.EventProcessed directives.
    if (m_endpointId == m_deviceInfo.getDefaultEndpointId()) {
        ACSDK_DEBUG5(LX("registeringEventProcessedDirective").d("reason", "defaultEndpoint"));
        configuration[NamespaceAndName{NAMESPACE, EVENT_PROCESSED_DIRECTIVE_NAME}] =
            BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    }

    /// Alexa.ReportState directives do arrive with an endpoint.
    configuration[{NAMESPACE, REPORT_STATE_DIRECTIVE_NAME, m_endpointId}] =
        BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);

    return configuration;
}

avsCommon::avs::CapabilityConfiguration AlexaInterfaceCapabilityAgent::getCapabilityConfiguration() {
    return CapabilityConfiguration(ALEXA_INTERFACE_TYPE, ALEXA_INTERFACE_NAME, ALEXA_INTERFACE_VERSION);
}

void AlexaInterfaceCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    // No-op.
}

void AlexaInterfaceCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void AlexaInterfaceCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this, info] {
        if (!info || !info->directive) {
            ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirective"));
            return;
        }
        if (EVENT_PROCESSED_DIRECTIVE_NAME == info->directive->getName()) {
            if (!executeHandleEventProcessed(info->directive)) {
                // Alexa.EventProcessed errors get an exception.
                executeSendExceptionEncounteredAndReportFailed(
                    info, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED, "empty event correlation token");
                return;
            }
        } else if (REPORT_STATE_DIRECTIVE_NAME == info->directive->getName()) {
            if (!info->directive->getEndpoint().hasValue() ||
                info->directive->getEndpoint().value().endpointId.empty()) {
                // Alexa.ReportState errors get an Alexa.ErrorReponse.
                executeSendErrorResponse(
                    info,
                    AlexaInterfaceMessageSenderInternalInterface::ErrorResponseType::INVALID_DIRECTIVE,
                    "missing endpoint");
            } else if (!m_alexaMessageSender->sendStateReportEvent(
                           info->directive->getInstance(),
                           info->directive->getCorrelationToken(),
                           info->directive->getEndpoint().value().endpointId)) {
                // Alexa.ReportState errors get an Alexa.ErrorReponse.
                executeSendErrorResponse(
                    info,
                    AlexaInterfaceMessageSenderInternalInterface::ErrorResponseType::INTERNAL_ERROR,
                    "failed to handle report state");
            }
        } else {
            // Unknown directives get an exception.
            executeSendExceptionEncounteredAndReportFailed(
                info, ExceptionErrorType::UNSUPPORTED_OPERATION, "unknown directive");
            return;
        }

        if (info->result) {
            info->result->setCompleted();
        }
        removeDirective(info);
    });
}

void AlexaInterfaceCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    removeDirective(info);
}

void AlexaInterfaceCapabilityAgent::removeDirective(const std::shared_ptr<DirectiveInfo>& info) {
    ACSDK_DEBUG5(LX(__func__));
    if (info && info->directive) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void AlexaInterfaceCapabilityAgent::executeSendExceptionEncounteredAndReportFailed(
    const std::shared_ptr<DirectiveInfo>& info,
    ExceptionErrorType errorType,
    const std::string& errorMessage) {
    ACSDK_ERROR(LX("handleDirectiveFailed")
                    .d("reason", errorMessage)
                    .d("namespace", info->directive->getNamespace())
                    .d("name", info->directive->getName()));

    const std::string exceptionMessage =
        errorMessage + " " + info->directive->getNamespace() + ":" + info->directive->getName();

    sendExceptionEncounteredAndReportFailed(info, exceptionMessage, errorType);
}

void AlexaInterfaceCapabilityAgent::executeSendErrorResponse(
    const std::shared_ptr<DirectiveInfo>& info,
    AlexaInterfaceMessageSenderInterface::ErrorResponseType errorType,
    const std::string& errorMessage) {
    if (!m_alexaMessageSender->sendErrorResponseEvent(
            info->directive->getInstance(),
            info->directive->getCorrelationToken(),
            info->directive->getEndpoint().value().endpointId,
            errorType,
            errorMessage)) {
        ACSDK_ERROR(LX("executeSendErrorResponseFailed").d("reason", "failedToSendEvent"));
    }
}

bool AlexaInterfaceCapabilityAgent::executeHandleEventProcessed(const std::shared_ptr<AVSDirective>& directive) {
    ACSDK_DEBUG5(LX(__func__));
    if (!directive) {
        ACSDK_ERROR(LX("executeHandleEventProcessedFailed").d("reason", "nullDirective"));
        return false;
    }

    std::string eventCorrelationToken = directive->getEventCorrelationToken();
    if (eventCorrelationToken.empty()) {
        ACSDK_ERROR(LX("executeHandleEventProcessedFailed").d("reason", "emptyEventCorrelationToken"));
        return false;
    }

    // Notify observers.
    std::lock_guard<std::mutex> lock{m_observerMutex};
    for (const auto& observer : m_observers) {
        observer->onAlexaEventProcessedReceived(eventCorrelationToken);
    }

    return true;
}

void AlexaInterfaceCapabilityAgent::addEventProcessedObserver(
    const std::shared_ptr<avsCommon::sdkInterfaces::AlexaEventProcessedObserverInterface>& observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addEventProcessedObserver").d("reason", "nullObserver"));
        return;
    }
    std::lock_guard<std::mutex> lock{m_observerMutex};
    m_observers.insert(observer);
}

void AlexaInterfaceCapabilityAgent::removeEventProcessedObserver(
    const std::shared_ptr<avsCommon::sdkInterfaces::AlexaEventProcessedObserverInterface>& observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeEventProcessedObserver").d("reason", "nullObserver"));
        return;
    }
    std::lock_guard<std::mutex> lock{m_observerMutex};
    m_observers.erase(observer);
}

}  // namespace alexa
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
