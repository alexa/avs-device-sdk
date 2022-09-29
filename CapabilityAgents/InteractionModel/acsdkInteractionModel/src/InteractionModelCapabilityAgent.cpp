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
#include "acsdkInteractionModel/InteractionModelCapabilityAgent.h"

#include <AVSCommon/AVS/NamespaceAndName.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace alexaClientSDK {
namespace acsdkInteractionModel {

using namespace rapidjson;
using namespace acsdkInteractionModelInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace json::jsonUtils;

/// String to identify log entries originating from this file.
#define TAG "InteractionModel"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const std::string NAMESPACE = "InteractionModel";

/// The NewDialogRequest directive signature.
static const NamespaceAndName NEW_DIALOG_REQUEST{NAMESPACE, "NewDialogRequest"};

/// The RequestProcessingStarted (RPS) directive signature.
static const NamespaceAndName REQUEST_PROCESS_STARTED{NAMESPACE, "RequestProcessingStarted"};

/// The RequestProcessingCompleted (RPC) directive signature.
static const NamespaceAndName REQUEST_PROCESS_COMPLETED{NAMESPACE, "RequestProcessingCompleted"};

/// Interaction Model capability constants
/// Interaction Model interface type
static const std::string INTERACTION_MODEL_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";

/// Interaction Model interface name
static const std::string INTERACTION_MODEL_CAPABILITY_INTERFACE_NAME = "InteractionModel";

/// Interaction Model interface version.
static const std::string INTERACTION_MODEL_CAPABILITY_INTERFACE_VERSION = "1.2";

/// NewDialogRequestID payload key.
static const std::string PAYLOAD_KEY_DIALOG_REQUEST_ID = "dialogRequestId";

/**
 * Creates the Interaction Model capability configuration.
 *
 * @return The Interaction Model capability configuration.
 */
static std::shared_ptr<CapabilityConfiguration> getInteractionModelCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, INTERACTION_MODEL_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, INTERACTION_MODEL_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, INTERACTION_MODEL_CAPABILITY_INTERFACE_VERSION});

    return std::make_shared<CapabilityConfiguration>(configMap);
}

std::shared_ptr<InteractionModelCapabilityAgent> InteractionModelCapabilityAgent::create(
    std::shared_ptr<DirectiveSequencerInterface> directiveSequencer,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<InteractionModelNotifierInterface> interactionModelNotifier,
    acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface> endpointCapabilitiesRegistrar) {
    if (!directiveSequencer) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullDirectiveSequencer"));
        return nullptr;
    }
    if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionEncounteredSender"));
        return nullptr;
    }
    if (!interactionModelNotifier) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullInteractionModelNotifier"));
        return nullptr;
    }
    if (!endpointCapabilitiesRegistrar) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullEndpointCapabilitiesRegistrar"));
        return nullptr;
    }

    auto interactionModelCapabilityAgent = std::shared_ptr<InteractionModelCapabilityAgent>(
        new InteractionModelCapabilityAgent(directiveSequencer, exceptionEncounteredSender, interactionModelNotifier));

    endpointCapabilitiesRegistrar->withCapability(interactionModelCapabilityAgent, interactionModelCapabilityAgent);

    return interactionModelCapabilityAgent;
}

InteractionModelCapabilityAgent::InteractionModelCapabilityAgent(
    std::shared_ptr<DirectiveSequencerInterface> directiveSequencer,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<InteractionModelNotifierInterface> interactionModelNotifier) :
        CapabilityAgent{NAMESPACE, exceptionEncounteredSender},
        m_directiveSequencer{directiveSequencer},
        m_interactionModelNotifier{interactionModelNotifier} {
    ACSDK_DEBUG5(LX(__func__));
    m_capabilityConfigurations.insert(getInteractionModelCapabilityConfiguration());
}

InteractionModelCapabilityAgent::~InteractionModelCapabilityAgent() {
    ACSDK_DEBUG5(LX(__func__));
}

DirectiveHandlerConfiguration InteractionModelCapabilityAgent::getConfiguration() const {
    DirectiveHandlerConfiguration configuration;
    // TODO: ARC-227 Verify default values.
    configuration[NEW_DIALOG_REQUEST] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[REQUEST_PROCESS_STARTED] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[REQUEST_PROCESS_COMPLETED] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    return configuration;
}
void InteractionModelCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));

    // The logic for handleDirectiveImmediately and preHandleDirective should be same,
    // which we only process directive without dialogRequestId
    preHandleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}
void InteractionModelCapabilityAgent::preHandleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info) {
        ACSDK_ERROR(LX("preHandleDirectiveFailed").d("reason", "nullInfo"));
        return;
    }

    if (!info->directive) {
        ACSDK_ERROR(LX("preHandleDirectiveFailed").d("reason", "nullDirective"));
        return;
    }

    // Both preHandleDirective and handleDirectiveImmediately interfaces only handle message
    // with empty dialogRequestId. Other messages (having dialogRequestId in header) will be
    // queued to handle sequentially.
    if (info->directive->getDialogRequestId().empty()) {
        std::string errMessage;
        ExceptionErrorType errType;

        if (handleDirectiveHelper(info, &errMessage, &errType)) {
            if (info->result) {
                info->result->setCompleted();
            }
        } else {
            ACSDK_ERROR(LX("preHandleDirectiveFailed").d("reason", errMessage));
            m_exceptionEncounteredSender->sendExceptionEncountered(
                info->directive->getUnparsedDirective(), errType, errMessage);
            if (info->result) {
                info->result->setFailed(errMessage);
            }
        }
        removeDirective(info->directive->getMessageId());
    }
}

bool InteractionModelCapabilityAgent::handleDirectiveHelper(
    std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    std::string* errMessage,
    ExceptionErrorType* type) {
    ACSDK_DEBUG5(LX(__func__));
    if (!errMessage) {
        ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "nullErrMessage"));
        return false;
    }

    if (!type) {
        ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "nullType"));
        return false;
    }

    if (!info) {
        ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "nullInfo"));
        return false;
    }

    if (!info->directive) {
        ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "nullDirective"));
        return false;
    }

    const std::string directiveName = info->directive->getName();

    Document payload(kObjectType);
    ParseResult result = payload.Parse(info->directive->getPayload());
    if (!result) {
        ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "directiveParseFailed"));
        *errMessage = "Parse failure";
        *type = ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED;
        return false;
    }

    if (NEW_DIALOG_REQUEST.name == directiveName) {
        rapidjson::Value::ConstMemberIterator it;
        std::string uuid;
        if (findNode(payload, PAYLOAD_KEY_DIALOG_REQUEST_ID, &it)) {
            if (!retrieveValue(payload, PAYLOAD_KEY_DIALOG_REQUEST_ID, &uuid)) {
                ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "dialogRequestIDNotAccessible"));
                *errMessage = "Dialog Request ID not accessible";
                *type = ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED;
                return false;
            }
            if (uuid.empty()) {
                ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "dialogRequestIDIsAnEmptyString"));
                *errMessage = "Dialog Request ID is an Empty String";
                *type = ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED;
                return false;
            }
            m_directiveSequencer->setDialogRequestId(uuid);
            ACSDK_DEBUG(LX(__func__).d("processDirective", directiveName).d("dialogRequestId", uuid));
        } else {
            *errMessage = "Dialog Request ID not specified";
            *type = ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED;
            return false;
        }
    } else if (REQUEST_PROCESS_STARTED.name == directiveName) {
        m_interactionModelNotifier->notifyObservers(
            [](std::shared_ptr<acsdkInteractionModelInterfaces::InteractionModelRequestProcessingObserverInterface>
                   observer) { observer->onRequestProcessingStarted(); });
        return true;
    } else if (REQUEST_PROCESS_COMPLETED.name == directiveName) {
        m_interactionModelNotifier->notifyObservers(
            [](std::shared_ptr<acsdkInteractionModelInterfaces::InteractionModelRequestProcessingObserverInterface>
                   observer) { observer->onRequestProcessingCompleted(); });
        return true;
    } else {
        *errMessage = directiveName + " not supported";
        *type = ExceptionErrorType::UNSUPPORTED_OPERATION;
        return false;
    }

    return true;
}
void InteractionModelCapabilityAgent::handleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullInfo"));
        return;
    }

    if (!info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirective"));
        return;
    }

    std::string errMessage = "emptyDialogRequestId";
    ExceptionErrorType errType = ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED;

    // Directives without dialogRequestId must be handled in preHandle or handleImmediately
    // which happen on receiving thread.
    if (!info->directive->getDialogRequestId().empty() && handleDirectiveHelper(info, &errMessage, &errType)) {
        if (info->result) {
            info->result->setCompleted();
        }
    } else {
        ACSDK_ERROR(LX("processDirectiveFailed").d("reason", errMessage));
        m_exceptionEncounteredSender->sendExceptionEncountered(
            info->directive->getUnparsedDirective(), errType, errMessage);
        if (info->result) {
            info->result->setFailed(errMessage);
        }
    }

    removeDirective(info->directive->getMessageId());
}
void InteractionModelCapabilityAgent::cancelDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    // No-op
}
std::unordered_set<std::shared_ptr<CapabilityConfiguration>> InteractionModelCapabilityAgent::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

}  // namespace acsdkInteractionModel
}  // namespace alexaClientSDK
