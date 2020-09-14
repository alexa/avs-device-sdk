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

#include "ApiGateway/ApiGatewayCapabilityAgent.h"

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <rapidjson/document.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace apiGateway {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json::jsonUtils;

/// String to identify log entries originating from this file.
static const std::string TAG{"ApiGateway"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The namespace for this CA.
static const std::string NAMESPACE = "Alexa.ApiGateway";

/// The @c SetGateway directive signature.
static const NamespaceAndName SET_GATEWAY_DIRECTIVE{NAMESPACE, "SetGateway"};

/// Gateway payload key.
static const std::string PAYLOAD_KEY_GATEWAY = "gateway";

/// ApiGateway capability constants
/// ApiGateway interface type
static const std::string APIGAETWAY_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";

/// AlexaInterface interface name
static const std::string APIGAETWAY_CAPABILITY_INTERFACE_NAME = "Alexa.ApiGateway";

/// AlexaInterface interface version.
static const std::string APIGAETWAY_CAPABILITY_INTERFACE_VERSION = "1.0";

/**
 * Creates the ApiGateway capability configuration.
 *
 * @return The ApiGateway capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getApiGatewayConfigurations() {
    return std::make_shared<CapabilityConfiguration>(
        APIGAETWAY_CAPABILITY_INTERFACE_TYPE,
        APIGAETWAY_CAPABILITY_INTERFACE_NAME,
        APIGAETWAY_CAPABILITY_INTERFACE_VERSION);
}

std::shared_ptr<ApiGatewayCapabilityAgent> ApiGatewayCapabilityAgent::create(
    std::shared_ptr<AVSGatewayManagerInterface> avsGatewayManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender) {
    ACSDK_DEBUG5(LX(__func__));
    if (!avsGatewayManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullavsGatewayManager"));
    } else if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionEncounteredSender"));
    } else {
        auto instance = std::shared_ptr<ApiGatewayCapabilityAgent>(
            new ApiGatewayCapabilityAgent(avsGatewayManager, exceptionEncounteredSender));
        return instance;
    }
    return nullptr;
}

ApiGatewayCapabilityAgent::ApiGatewayCapabilityAgent(
    std::shared_ptr<AVSGatewayManagerInterface> avsGatewayManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender) :
        CapabilityAgent{NAMESPACE, exceptionEncounteredSender},
        RequiresShutdown("ApiGateway"),
        m_avsGatewayManager{avsGatewayManager} {
    m_capabilityConfigurations.insert(getApiGatewayConfigurations());
}

DirectiveHandlerConfiguration ApiGatewayCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));
    DirectiveHandlerConfiguration configuration;
    configuration[SET_GATEWAY_DIRECTIVE] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    return configuration;
}

std::unordered_set<std::shared_ptr<CapabilityConfiguration>> ApiGatewayCapabilityAgent::getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

void ApiGatewayCapabilityAgent::preHandleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    // No-op
}

void ApiGatewayCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void ApiGatewayCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this, info] { executeHandleDirective(info); });
}

void ApiGatewayCapabilityAgent::executeHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirective"));
        return;
    }

    if (SET_GATEWAY_DIRECTIVE.name == info->directive->getName()) {
        std::string newGateway;
        if (!retrieveValue(info->directive->getPayload(), PAYLOAD_KEY_GATEWAY, &newGateway)) {
            executeSendExceptionEncountered(
                info, "unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        } else {
            if (!m_avsGatewayManager->setGatewayURL(newGateway)) {
                ACSDK_ERROR(LX("executeHandleDirectiveFailed").d("reason", "failure to set gateway URL"));
            }
        }
    } else {
        executeSendExceptionEncountered(info, "unknown directive", ExceptionErrorType::UNSUPPORTED_OPERATION);
        return;
    }
    executeSetHandlingCompleted(info);
}

void ApiGatewayCapabilityAgent::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (info && info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void ApiGatewayCapabilityAgent::executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (info && info->directive && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void ApiGatewayCapabilityAgent::executeSendExceptionEncountered(
    std::shared_ptr<DirectiveInfo> info,
    const std::string& message,
    alexaClientSDK::avsCommon::avs::ExceptionErrorType errorType) {
    ACSDK_ERROR(LX("handleDirectiveFailed")
                    .d("reason", message)
                    .d("namespace", info->directive->getNamespace())
                    .d("name", info->directive->getName()));

    const std::string exceptionMessage =
        message + " " + info->directive->getNamespace() + ":" + info->directive->getName();

    sendExceptionEncounteredAndReportFailed(info, exceptionMessage, errorType);
}

void ApiGatewayCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    removeDirective(info);
}
void ApiGatewayCapabilityAgent::doShutdown() {
    m_executor.shutdown();
}

}  // namespace apiGateway
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
