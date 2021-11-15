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
#include <unordered_map>

#include "InputControllerCapabilityAgent.h"

#include <AVSCommon/AVS/NamespaceAndName.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace acsdkInputController {

using namespace rapidjson;
using namespace acsdkInputControllerInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace json::jsonUtils;

/// String to identify log entries originating from this file.
static const std::string TAG{"InputController"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const std::string NAMESPACE = "Alexa.InputController";

/// The SelectInput directive signature.
static const NamespaceAndName SELECT_INPUT{NAMESPACE, "SelectInput"};

/// InputController capability constants
/// The AlexaInterface constant type.
static const std::string ALEXA_INTERFACE_TYPE = "AlexaInterface";

/// Interface name
static const std::string INPUT_CONTROLLER_CAPABILITY_INTERFACE_NAME = "Alexa.InputController";

/// Interface version.
static const std::string INPUT_CONTROLLER_CAPABILITY_INTERFACE_VERSION = "3.0";

/// The configuration key
static const std::string CAPABILITY_CONFIGURATION_KEY{"configurations"};

/// Payload input key
static const std::string INPUT_CONTROLLER_INPUT_KEY = "input";

/// Payload inputs key
static const std::string INPUT_CONTROLLER_CONFIGURATION_KEY = "inputs";

/// Payload name key
static const std::string INPUT_CONTROLLER_CONFIGURATION_NAME_KEY = "name";

/// Payload friendlyNames key
static const std::string INPUT_CONTROLLER_CONFIGURATION_FRIENDLY_NAMES_KEY = "friendlyNames";

/**
 * A helper function to check if the input configurations from the handler is valid.
 *
 * @param inputConfigurations The @c inputs and its friendly names.
 * @return true if valid, false otherwise.
 */
static bool checkInputs(const InputControllerCapabilityAgent::InputFriendlyNameConfigurations& inputConfigurations) {
    std::unordered_map<std::string, std::string> friendlyNamesCheck;
    if (inputConfigurations.empty()) {
        ACSDK_ERROR(LX("checkInputsFailed").d("reason", "emptyInputConfig"));
        return false;
    }
    for (const auto& input : inputConfigurations) {
        for (const auto& friendlyName : input.second) {
            if (0 == friendlyNamesCheck.count(friendlyName)) {
                friendlyNamesCheck[friendlyName] = input.first;
            } else {
                ACSDK_ERROR(LX("checkInputsFailed")
                                .d("reason", "friendlyNameExistsInTwoInputs")
                                .d("friendlyName", friendlyName)
                                .d("input1", friendlyNamesCheck[friendlyName])
                                .d("input2", input.first));
                return false;
            }
        }
    }
    return true;
}

/**
 * A helper function to generate the @c CapabilityConfiguration based on the @c InputConfigurations.
 *
 * @param inputConfigurations The @c InputConfigurations to read from.
 * @return A @c std::shared_ptr to a @c CapabilityConfiguration.
 */
static std::shared_ptr<CapabilityConfiguration> getInputControllerCapabilityConfiguration(
    const InputControllerCapabilityAgent::InputFriendlyNameConfigurations& inputConfigurations) {
    avsCommon::utils::json::JsonGenerator jsonGenerator;

    jsonGenerator.startArray(INPUT_CONTROLLER_CONFIGURATION_KEY);
    for (const auto& input : inputConfigurations) {
        jsonGenerator.startArrayElement();
        jsonGenerator.addMember(INPUT_CONTROLLER_CONFIGURATION_NAME_KEY, input.first);
        jsonGenerator.addStringArray(INPUT_CONTROLLER_CONFIGURATION_FRIENDLY_NAMES_KEY, input.second);
        jsonGenerator.finishArrayElement();
    }
    jsonGenerator.finishArray();

    auto additionalConfigurations = CapabilityConfiguration::AdditionalConfigurations();
    additionalConfigurations[CAPABILITY_CONFIGURATION_KEY] = jsonGenerator.toString();

    auto configuration = std::make_shared<CapabilityConfiguration>(
        ALEXA_INTERFACE_TYPE,
        INPUT_CONTROLLER_CAPABILITY_INTERFACE_NAME,
        INPUT_CONTROLLER_CAPABILITY_INTERFACE_VERSION,
        avsCommon::utils::Optional<std::string>(),
        avsCommon::utils::Optional<CapabilityConfiguration::Properties>(),
        additionalConfigurations);

    return configuration;
}

std::shared_ptr<InputControllerCapabilityAgent> InputControllerCapabilityAgent::create(
    const std::shared_ptr<acsdkInputControllerInterfaces::InputControllerHandlerInterface>& handler,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender) {
    if (!handler) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullHandler"));
        return nullptr;
    }
    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    }
    if (!checkInputs(handler->getConfiguration().inputs)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "invalidInputs"));
        return nullptr;
    }
    auto inputControllerCapabilityAgent =
        std::shared_ptr<InputControllerCapabilityAgent>(new InputControllerCapabilityAgent(handler, exceptionSender));
    return inputControllerCapabilityAgent;
}

InputControllerCapabilityAgent::InputControllerCapabilityAgent(
    const std::shared_ptr<acsdkInputControllerInterfaces::InputControllerHandlerInterface>& handler,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        m_inputControllerHandler{handler} {
    ACSDK_DEBUG5(LX(__func__));
    m_inputConfigurations = handler->getConfiguration().inputs;
    m_capabilityConfigurations.insert(getInputControllerCapabilityConfiguration(m_inputConfigurations));
}

DirectiveHandlerConfiguration InputControllerCapabilityAgent::getConfiguration() const {
    DirectiveHandlerConfiguration configuration;
    configuration[SELECT_INPUT] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    return configuration;
}

void InputControllerCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void InputControllerCapabilityAgent::preHandleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    // No-op
}

bool InputControllerCapabilityAgent::executeHandleDirectiveHelper(
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

    if (SELECT_INPUT.name == directiveName) {
        rapidjson::Value::ConstMemberIterator it;
        std::string input;
        if (!retrieveValue(payload, INPUT_CONTROLLER_INPUT_KEY, &input)) {
            ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "missingInputField"));
            *errMessage = "Input field is not accessible";
            *type = ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED;
            return false;
        }
        if (input.empty()) {
            ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "inputIsEmptyString"));
            *errMessage = "Input is an Empty String";
            *type = ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED;
            return false;
        }
        if (0 == m_inputConfigurations.count(input)) {
            ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "InvalidInputReceived"));
            *errMessage = "Input is invalid";
            *type = ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED;
            return false;
        }

        ACSDK_INFO(LX("inputControllerNotifier").d("input", input));
        if (!m_inputControllerHandler->onInputChange(input)) {
            ACSDK_ERROR(LX("processDirectiveFailed").d("reason", "onInputChangeFailed"));
            *errMessage = "Input change failed";
            *type = ExceptionErrorType::INTERNAL_ERROR;
            return false;
        }
    } else {
        *errMessage = directiveName + " not supported";
        *type = ExceptionErrorType::UNSUPPORTED_OPERATION;
        return false;
    }

    return true;
}

void InputControllerCapabilityAgent::handleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullInfo"));
        return;
    }

    if (!info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirective"));
        return;
    }

    m_executor.submit([this, info] {
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

void InputControllerCapabilityAgent::cancelDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    // No-op
}

std::unordered_set<std::shared_ptr<CapabilityConfiguration>> InputControllerCapabilityAgent::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

}  // namespace acsdkInputController
}  // namespace alexaClientSDK
