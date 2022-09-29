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

#include "acsdk/AlexaInputController/private/InputControllerCapabilityAgent.h"

#include <AVSCommon/AVS/NamespaceAndName.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Optional.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace alexaClientSDK {
namespace alexaInputController {

using namespace rapidjson;
using namespace alexaInputControllerInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::json;
using namespace json::jsonUtils;

/// String to identify log entries originating from this file.
#define TAG "AlexaInputControllerCapabilityAgent"

/*
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const std::string NAMESPACE = "Alexa.InputController";

/// The name for the SelectInput directive.
static const std::string NAME_SELECT_INPUT{"SelectInput"};

/// InputController capability constants
/// The AlexaInterface constant type.
static const std::string ALEXA_INTERFACE_TYPE = "AlexaInterface";

/// Interface version.
static const std::string INTERFACE_VERSION = "3";

/// The configuration key
static const std::string CAPABILITY_INPUTS_KEY{"inputs"};

/// Payload input key
static const std::string INPUT_CONTROLLER_INPUT_KEY{"input"};

/// Payload name key
static const std::string INPUT_CONTROLLER_CONFIGURATION_NAME_KEY{"name"};

/// The name of inputState property
static const std::string INPUT_PROPERTY_NAME{"input"};

/// The name of friendlyName property
static const std::string FRIENDLY_NAME_PROPERTY_NAME{"friendlyNames"};

std::shared_ptr<AlexaInputControllerCapabilityAgent> AlexaInputControllerCapabilityAgent::create(
    endpoints::EndpointIdentifier endpointId,
    std::shared_ptr<alexaInputControllerInterfaces::InputControllerInterface> inputController,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable) {
    if (endpointId.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyEndpointId"));
        return nullptr;
    }
    if (!inputController) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullInputController"));
        return nullptr;
    }
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    }
    if (!responseSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullResponseSender"));
        return nullptr;
    }
    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    }

    auto inputControllerCapabilityAgent =
        std::shared_ptr<AlexaInputControllerCapabilityAgent>(new AlexaInputControllerCapabilityAgent(
            std::move(endpointId),
            std::move(inputController),
            std::move(contextManager),
            std::move(responseSender),
            std::move(exceptionSender),
            isProactivelyReported,
            isRetrievable));

    if (!inputControllerCapabilityAgent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "instantiationFailed"));
        return nullptr;
    }

    if (!inputControllerCapabilityAgent->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initializationFailed"));
        return nullptr;
    }

    return inputControllerCapabilityAgent;
}

AlexaInputControllerCapabilityAgent::AlexaInputControllerCapabilityAgent(
    endpoints::EndpointIdentifier endpointId,
    std::shared_ptr<alexaInputControllerInterfaces::InputControllerInterface> inputController,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"AlexaInputControllerCapabilityAgent"},
        m_endpointId{std::move(endpointId)},
        m_isProactivelyReported{isProactivelyReported},
        m_isRetrievable{isRetrievable},
        m_inputController{std::move(inputController)},
        m_contextManager{std::move(contextManager)},
        m_responseSender{std::move(responseSender)} {
}

void AlexaInputControllerCapabilityAgent::doShutdown() {
    ACSDK_DEBUG9(LX(__func__));
    if (!m_executor.isShutdown()) {
        m_executor.shutdown();
    }

    if (m_isProactivelyReported) {
        m_inputController->removeObserver(shared_from_this());
    }
    m_inputController.reset();
    m_responseSender.reset();
    if (m_isRetrievable) {
        m_contextManager->removeStateProvider({NAMESPACE, INPUT_PROPERTY_NAME, m_endpointId});
    }
    m_contextManager.reset();
}

bool AlexaInputControllerCapabilityAgent::initialize() {
    ACSDK_DEBUG9(LX(__func__));
    m_supportedInputs = m_inputController->getSupportedInputs();
    if (m_supportedInputs.empty()) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "emptySupportedInputsSet"));
        return false;
    }

    if (m_isProactivelyReported) {
        if (!m_inputController->addObserver(shared_from_this())) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "addObserverFailed"));
            return false;
        }
    }
    if (m_isRetrievable) {
        m_contextManager->addStateProvider({NAMESPACE, INPUT_PROPERTY_NAME, m_endpointId}, shared_from_this());
    }
    return true;
}

AlexaInputControllerCapabilityAgent::~AlexaInputControllerCapabilityAgent() {
    ACSDK_DEBUG9(LX(__func__));
}

DirectiveHandlerConfiguration AlexaInputControllerCapabilityAgent::getConfiguration() const {
    DirectiveHandlerConfiguration configuration;
    configuration[{NAMESPACE, NAME_SELECT_INPUT, m_endpointId}] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    return configuration;
}

void AlexaInputControllerCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG9(LX(__func__));
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void AlexaInputControllerCapabilityAgent::preHandleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    // No-op
}

bool AlexaInputControllerCapabilityAgent::executeHandleDirectiveHelper(
    std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    std::string* errMessage,
    ExceptionErrorType* type) {
    ACSDK_DEBUG9(LX(__func__));
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

    if (!info->directive->getEndpoint().hasValue() ||
        (info->directive->getEndpoint().value().endpointId != m_endpointId)) {
        ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "endpointIdMismatch"));
        *errMessage = "EndpointId Mismatch";
        *type = ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED;
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

    if (NAME_SELECT_INPUT == directiveName) {
        rapidjson::Value::ConstMemberIterator it;
        std::string inputString;
        if (!retrieveValue(payload, INPUT_CONTROLLER_INPUT_KEY, &inputString)) {
            ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "missingInputField"));
            *errMessage = "Input field is not accessible";
            *type = ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED;
            return false;
        }
        if (inputString.empty()) {
            ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "inputIsEmptyString"));
            *errMessage = "Input is an Empty String";
            *type = ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED;
            return false;
        }
        auto input = alexaInputControllerInterfaces::stringToInput(inputString);
        if (!input.hasValue()) {
            ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "InvalidInputReceived"));
            *errMessage = "Input is invalid";
            *type = ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED;
            return false;
        }

        auto inputResult = m_inputController->setInput(input.value());
        sendResponseEvent(info, inputResult);
    } else {
        *errMessage = directiveName + " not supported";
        *type = ExceptionErrorType::UNSUPPORTED_OPERATION;
        return false;
    }

    return true;
}

void AlexaInputControllerCapabilityAgent::handleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    m_executor.execute([this, info] {
        std::string errMessage;
        ExceptionErrorType errType;

        if (executeHandleDirectiveHelper(info, &errMessage, &errType)) {
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
    });
}

void AlexaInputControllerCapabilityAgent::cancelDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("cancelDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    if (!info->directive->getEndpoint().hasValue() ||
        info->directive->getEndpoint().value().endpointId != m_endpointId) {
        ACSDK_WARN(LX("cancelDirective").d("reason", "notExpectedEndpointId"));
    }
    CapabilityAgent::removeDirective(info->directive->getMessageId());
}

/*
 * Helper function to create a json array based on @c SupportedInputsSet.
 *
 * The format of the json is this:
 *
 *
 * @code{.json}
 * [
 *     {
 *         "name": "{{STRING}}",
 *         "friendlyNames": ["{{STRING}}", ...]
 *     },
 *     {
 *         "name": "{{STRING}}",
 *         "friendlyNames": ["{{STRING}}", ...]
 *     }
 * ]
 * @endcode
 *
 * @param inputSet The inputsSet.
 * @return A json string.
 */
static std::string getInputArrayJson(const InputControllerInterface::SupportedInputs& inputSet) {
    std::string json;
    rapidjson::Document inputsJson(rapidjson::kArrayType);
    auto& alloc = inputsJson.GetAllocator();
    for (const auto& input : inputSet) {
        rapidjson::Document payloadJson(rapidjson::kObjectType);
        payloadJson.AddMember(
            rapidjson::StringRef(INPUT_CONTROLLER_CONFIGURATION_NAME_KEY),
            alexaInputControllerInterfaces::inputToString(input.first),
            alloc);

        rapidjson::Value friendlyNamesArrayJson(rapidjson::kArrayType);
        for (const auto& friendlyName : input.second) {
            rapidjson::Value value;
            value.SetString(friendlyName.c_str(), friendlyName.length(), alloc);
            friendlyNamesArrayJson.PushBack(value, alloc);
        }
        payloadJson.AddMember(rapidjson::StringRef(FRIENDLY_NAME_PROPERTY_NAME), friendlyNamesArrayJson, alloc);
        inputsJson.PushBack(payloadJson, alloc);
    }

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    inputsJson.Accept(writer);

    json = sb.GetString();
    return json;
}

std::unordered_set<std::shared_ptr<CapabilityConfiguration>> AlexaInputControllerCapabilityAgent::
    getCapabilityConfigurations() {
    auto additionalConfigurations = CapabilityConfiguration::AdditionalConfigurations();
    additionalConfigurations[CAPABILITY_INPUTS_KEY] = getInputArrayJson(m_supportedInputs);

    CapabilityConfiguration configuration{CapabilityConfiguration::ALEXA_INTERFACE_TYPE,
                                          NAMESPACE,
                                          INTERFACE_VERSION,
                                          avsCommon::utils::Optional<std::string>(),
                                          Optional<CapabilityConfiguration::Properties>(
                                              {m_isRetrievable, m_isProactivelyReported, {INPUT_PROPERTY_NAME}}),
                                          additionalConfigurations};

    return {std::make_shared<avsCommon::avs::CapabilityConfiguration>(configuration)};
}

void AlexaInputControllerCapabilityAgent::provideState(
    const CapabilityTag& stateProviderName,
    const avsCommon::sdkInterfaces::ContextRequestToken contextRequestToken) {
    ACSDK_DEBUG9(
        LX(__func__).d("contextRequestToken", contextRequestToken).sensitive("stateProviderName", stateProviderName));

    m_executor.execute([this, stateProviderName, contextRequestToken] {
        ACSDK_DEBUG9(LX("provideStateInExecutor"));
        executeProvideState(stateProviderName, contextRequestToken);
    });
}

bool AlexaInputControllerCapabilityAgent::canStateBeRetrieved() {
    ACSDK_DEBUG9(LX(__func__));
    return m_isRetrievable;
}

bool AlexaInputControllerCapabilityAgent::hasReportableStateProperties() {
    ACSDK_DEBUG9(LX(__func__));
    return m_isRetrievable || m_isProactivelyReported;
}

void AlexaInputControllerCapabilityAgent::onInputChanged(Input input) {
    ACSDK_DEBUG9(LX(__func__));
    if (!m_isProactivelyReported) {
        ACSDK_ERROR(LX("onInputChangedFailed").d("reason", "invalidOnInputChangedCall"));
        return;
    }

    m_executor.execute([this, input] {
        m_contextManager->reportStateChange(
            CapabilityTag(NAMESPACE, INPUT_PROPERTY_NAME, m_endpointId),
            buildCapabilityState(input),
            AlexaStateChangeCauseType::VOICE_INTERACTION);
    });
}

void AlexaInputControllerCapabilityAgent::executeProvideState(
    const CapabilityTag& stateProviderName,
    const ContextRequestToken contextRequestToken) {
    ACSDK_DEBUG9(LX("executeProvideState"));

    if (stateProviderName.endpointId != m_endpointId) {
        ACSDK_ERROR(LX("provideStateFailed")
                        .d("reason", "notExpectedEndpointId")
                        .sensitive("endpointId", stateProviderName.endpointId));
        m_contextManager->provideStateUnavailableResponse(stateProviderName, contextRequestToken, false);
        return;
    }
    if (stateProviderName.name != INPUT_PROPERTY_NAME) {
        ACSDK_ERROR(LX("provideStateFailed").d("reason", "notExpectedName").sensitive("name", stateProviderName.name));
        m_contextManager->provideStateUnavailableResponse(stateProviderName, contextRequestToken, false);
        return;
    }
    if (!m_isRetrievable) {
        ACSDK_ERROR(LX("provideStateFailed").d("reason", "provideStateOnNotRetrievableProperty"));
        m_contextManager->provideStateUnavailableResponse(stateProviderName, contextRequestToken, false);
        return;
    }

    Input input = m_inputController->getInput();
    m_contextManager->provideStateResponse(stateProviderName, buildCapabilityState(input), contextRequestToken);
}

void AlexaInputControllerCapabilityAgent::sendAlexaErrorResponse(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    AlexaInterfaceMessageSenderInterface::ErrorResponseType alexaErrorResponseType,
    const std::string& responseMessage) {
    m_responseSender->sendErrorResponseEvent(
        info->directive->getInstance(),
        info->directive->getCorrelationToken(),
        AVSMessageEndpoint(m_endpointId),
        alexaErrorResponseType,
        responseMessage);
}

void AlexaInputControllerCapabilityAgent::sendResponseEvent(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo>& info,
    const InputControllerInterface::Response& result) {
    switch (result.type) {
        case InputControllerInterface::Response::Type::SUCCESS: {
            m_responseSender->sendResponseEvent(
                info->directive->getInstance(),
                info->directive->getCorrelationToken(),
                AVSMessageEndpoint(m_endpointId));
            break;
        }
        case InputControllerInterface::Response::Type::FAILED_INVALID_VALUE:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INVALID_VALUE, result.errorMessage);
            break;
        case InputControllerInterface::Response::Type::FAILED_TOO_MANY_FAILED_ATTEMPTS:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, result.errorMessage);
            break;
        case InputControllerInterface::Response::Type::FAILED_ENDPOINT_UNREACHABLE:
            sendAlexaErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::ErrorResponseType::ENDPOINT_UNREACHABLE,
                result.errorMessage);
            break;
        case InputControllerInterface::Response::Type::FAILED_INTERNAL_ERROR:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, result.errorMessage);
            break;
    }
}

CapabilityState AlexaInputControllerCapabilityAgent::buildCapabilityState(Input input) {
    return CapabilityState("\"" + alexaInputControllerInterfaces::inputToString(input) + "\"");
}

}  // namespace alexaInputController
}  // namespace alexaClientSDK
