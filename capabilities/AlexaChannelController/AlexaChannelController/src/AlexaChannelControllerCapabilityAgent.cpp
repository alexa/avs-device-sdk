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

#include <acsdk/AlexaChannelController/private/AlexaChannelControllerCapabilityAgent.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace alexaChannelController {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::json;
using namespace rapidjson;

using namespace alexaChannelControllerInterfaces;
using namespace alexaChannelControllerTypes;

/// String to identify log entries originating from this file.
#define TAG "AlexaChannelControllerCapabilityAgent"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const std::string NAMESPACE{"Alexa.ChannelController"};

/// The namespace for Alexa.Video.ErrorResponse.
static const std::string NAMESPACE_ALEXA_VIDEO_ERRORRESPONSE{"Alexa.Video"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for ChangeChannel directive
static const std::string NAME_CHANGECHANNEL{"ChangeChannel"};

/// The name for SkipChannels directive
static const std::string NAME_SKIPCHANNELS{"SkipChannels"};

/// The name for SkipChannels channel count property
static const std::string NAME_SKIPCHANNELS_CHANNELCOUNT{"channelCount"};

/// The name of channel property
static const std::string CHANNELSTATE_PROPERTY_NAME{"channel"};

/// The name of channel property
static const std::string CHANNELSTATE_PROPERTY_METADATA_NAME{"channelMetadata"};

/// Name of the @c ConfigurationNode for ChannelController.
static const std::string CHANNEL_CONTROLLER_ROOT_KEY{"channelController"};

/// The name of the key for channel lineups in configuration
static const std::string CAPABILITY_CONFIGURATION_LINEUP_KEY{"lineup"};

/// (Required) The name of the key for operator name.
static const std::string CAPABILITY_CONFIGURATION_LINEUP_OPERATOR_NAME_KEY{"operatorName"};

/// (Optional) The name of the key for lineup name.
static const std::string CAPABILITY_CONFIGURATION_LINEUP_NAME_KEY{"lineupName"};

/// (Optional) The name of the key for postal code.
static const std::string CAPABILITY_CONFIGURATION_LINEUP_POSTAL_CODE_KEY{"postalCode"};

/// (Required) The name of the key for type.
static const std::string CAPABILITY_CONFIGURATION_LINEUP_TYPE_KEY{"type"};

/// Json key for parsing channelNumber.
static const std::string NUMBER{"number"};

/// Json key for parsing channelCallSign.
static const std::string CALL_SIGN{"callSign"};

/// Json key for parsing uri.
static const std::string URI{"uri"};

/// Json key for parsing Affiliate CallSign.
static const std::string AFFILIATE_CALL_SIGN{"affiliateCallSign"};

/// Json key for parsing name.
static const std::string NAME{"name"};

/// Json key for parsing image.
static const std::string IMAGE{"image"};

std::shared_ptr<AlexaChannelControllerCapabilityAgent> AlexaChannelControllerCapabilityAgent::create(
    avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
    std::shared_ptr<ChannelControllerInterface> channelController,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable) {
    if (endpointId.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyEndpointId"));
        return nullptr;
    }
    if (!channelController) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullChannelController"));
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

    auto channelControllerCapabilityAgent =
        std::shared_ptr<AlexaChannelControllerCapabilityAgent>(new AlexaChannelControllerCapabilityAgent(
            std::move(endpointId),
            std::move(channelController),
            std::move(contextManager),
            std::move(responseSender),
            std::move(exceptionSender),
            isProactivelyReported,
            isRetrievable));

    if (!channelControllerCapabilityAgent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "instantiationFailed"));
        return nullptr;
    }

    if (!channelControllerCapabilityAgent->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initializationFailed"));
        return nullptr;
    }

    return channelControllerCapabilityAgent;
}

AlexaChannelControllerCapabilityAgent::AlexaChannelControllerCapabilityAgent(
    avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
    std::shared_ptr<ChannelControllerInterface> channelController,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"AlexaChannelControllerCapabilityAgent"},
        m_endpointId{std::move(endpointId)},
        m_isProactivelyReported{isProactivelyReported},
        m_isRetrievable{isRetrievable},
        m_channelController{std::move(channelController)},
        m_contextManager{std::move(contextManager)},
        m_responseSender{std::move(responseSender)} {
}

bool AlexaChannelControllerCapabilityAgent::initialize() {
    ACSDK_DEBUG9(LX(__func__));
    if (m_isRetrievable) {
        m_contextManager->addStateProvider({NAMESPACE, CHANNELSTATE_PROPERTY_NAME, m_endpointId}, shared_from_this());
    }
    if (m_isProactivelyReported) {
        if (!m_channelController->addObserver(shared_from_this())) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "addObserverFailed"));
            return false;
        }
    }

    return true;
}

void AlexaChannelControllerCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG9(LX(__func__));
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    handleDirective(std::make_shared<CapabilityAgent::DirectiveInfo>(directive, nullptr));
}

void AlexaChannelControllerCapabilityAgent::preHandleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    ACSDK_DEBUG9(LX("preHandleDirective"));
    // do nothing.
}

/**
 * Parses a directive payload JSON and returns a parsed document object.
 *
 * @param payload JSON string to parse.
 * @param[out] document Pointer to a parsed document.
 * @return @c true if parsing was successful, @c false otherwise.
 */
static bool parseDirectivePayload(const std::string& payload, Document* document) {
    ACSDK_DEBUG9(LX(__func__));
    if (!document) {
        ACSDK_ERROR(LX("parseDirectivePayloadFailed").d("reason", "nullDocument"));
        return false;
    }

    ParseResult result = document->Parse(payload);
    if (!result) {
        ACSDK_ERROR(LX("parseDirectivePayloadFailed")
                        .d("reason", "parseFailed")
                        .d("error", GetParseError_En(result.Code()))
                        .d("offset", result.Offset()));
        return false;
    }

    return true;
}

std::unique_ptr<Channel> AlexaChannelControllerCapabilityAgent::parseChannel(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    const Document& payloadDocument) {
    if (payloadDocument.HasMember(CHANNELSTATE_PROPERTY_NAME) &&
        payloadDocument[CHANNELSTATE_PROPERTY_NAME].IsObject()) {
        const auto& channelJson = payloadDocument[CHANNELSTATE_PROPERTY_NAME];
        std::string number;
        if (channelJson.HasMember(NUMBER) && channelJson[NUMBER].IsString()) {
            number = channelJson[NUMBER].GetString();
        }
        std::string callSign;
        if (channelJson.HasMember(CALL_SIGN) && channelJson[CALL_SIGN].IsString()) {
            callSign = channelJson[CALL_SIGN].GetString();
        }
        std::string affiliateCallSign;
        if (channelJson.HasMember(AFFILIATE_CALL_SIGN) && channelJson[AFFILIATE_CALL_SIGN].IsString()) {
            affiliateCallSign = channelJson[AFFILIATE_CALL_SIGN].GetString();
        }
        std::string uri;
        if (channelJson.HasMember(URI) && channelJson[URI].IsString()) {
            uri = channelJson[URI].GetString();
        }
        std::string name;
        std::string image;
        if (payloadDocument.HasMember(CHANNELSTATE_PROPERTY_METADATA_NAME) &&
            payloadDocument[CHANNELSTATE_PROPERTY_METADATA_NAME].IsObject()) {
            const auto& channelMetadataJson = payloadDocument[CHANNELSTATE_PROPERTY_METADATA_NAME];
            if (channelMetadataJson.HasMember(NAME) && channelMetadataJson[NAME].IsString()) {
                name = channelMetadataJson[NAME].GetString();
            }
            if (channelMetadataJson.HasMember(IMAGE) && channelMetadataJson[IMAGE].IsString()) {
                image = channelMetadataJson[IMAGE].GetString();
            }
        } else {
            std::string errorMessage = CHANNELSTATE_PROPERTY_METADATA_NAME + " not found";
            ACSDK_ERROR(LX("readPayloadFailed").m(errorMessage));
            sendExceptionEncounteredAndReportFailed(info, errorMessage);
            return nullptr;
        }
        return Channel::Create(number, callSign, affiliateCallSign, uri, name, image);
    }

    std::string errorMessage = CHANNELSTATE_PROPERTY_NAME + " not found";
    ACSDK_ERROR(LX("readPayloadFailed").m(errorMessage));
    sendExceptionEncounteredAndReportFailed(info, errorMessage);
    return nullptr;
}

bool AlexaChannelControllerCapabilityAgent::parseChannelCount(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    const Document& payloadDocument,
    int64_t& channelCount) {
    if (payloadDocument.HasMember(NAME_SKIPCHANNELS_CHANNELCOUNT) &&
        payloadDocument[NAME_SKIPCHANNELS_CHANNELCOUNT].IsInt()) {
        channelCount = payloadDocument[NAME_SKIPCHANNELS_CHANNELCOUNT].GetInt();
        return true;
    }
    ACSDK_ERROR(LX("readPayloadFailed").m(NAME_SKIPCHANNELS_CHANNELCOUNT + " not found"));
    return false;
}

void AlexaChannelControllerCapabilityAgent::handleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    m_executor.execute([this, info] {
        ACSDK_DEBUG9(LX("handleDirectiveInExecutor"));
        const std::string directiveName = info->directive->getName();
        if (!info->directive->getEndpoint().hasValue() ||
            (info->directive->getEndpoint().value().endpointId != m_endpointId)) {
            executeUnknownDirective(info, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        Document payloadDocument(kObjectType);
        if (!parseDirectivePayload(info->directive->getPayload(), &payloadDocument)) {
            sendExceptionEncounteredAndReportFailed(
                info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        ChannelControllerInterface::Response response;
        if (directiveName == NAME_CHANGECHANNEL) {
            std::unique_ptr<Channel> channel = parseChannel(info, payloadDocument);
            if (channel == nullptr) {
                ACSDK_ERROR(LX("readChangeChannelPayloadFailed"));
                return;
            }
            response = m_channelController->change(std::move(channel));
        } else if (directiveName == NAME_SKIPCHANNELS) {
            int64_t channelCount;
            auto successfullyParsed = parseChannelCount(info, payloadDocument, channelCount);
            if (!successfullyParsed) {
                std::string errorMessage = NAME_SKIPCHANNELS_CHANNELCOUNT + " not found";
                ACSDK_ERROR(LX("readPayloadFailed").m(errorMessage));
                sendExceptionEncounteredAndReportFailed(info, errorMessage);
                return;
            }
            if (channelCount == 1) {
                response = m_channelController->incrementChannel();
            } else if (channelCount == -1) {
                response = m_channelController->decrementChannel();
            } else {
                std::string errorMessage = "Payload value: " + std::to_string(channelCount);
                ACSDK_ERROR(LX("Unexpected Value of payload").m(errorMessage));
                sendExceptionEncounteredAndReportFailed(info, errorMessage);
                return;
            }

        } else {
            ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "unexpectedDirective").d("name", directiveName));
            executeUnknownDirective(info, ExceptionErrorType::UNSUPPORTED_OPERATION);
            return;
        }
        executeSetHandlingCompleted(info);
        std::unique_ptr<Channel> channel = m_channelController->getCurrentChannel();
        if (channel == nullptr) {
            ACSDK_ERROR(LX("Empty Channel State"));
            return;
        }
        sendResponseEvent(info, response, std::move(channel));
    });
}

void AlexaChannelControllerCapabilityAgent::provideState(
    const CapabilityTag& stateProviderName,
    const ContextRequestToken contextRequestToken) {
    ACSDK_DEBUG9(
        LX(__func__).d("contextRequestToken", contextRequestToken).sensitive("stateProviderName", stateProviderName));

    m_executor.execute([this, stateProviderName, contextRequestToken] {
        ACSDK_DEBUG9(LX("provideStateInExecutor"));
        executeProvideState(stateProviderName, contextRequestToken);
    });
    return;
}

bool AlexaChannelControllerCapabilityAgent::canStateBeRetrieved() {
    ACSDK_DEBUG9(LX(__func__));
    return m_isRetrievable;
}

bool AlexaChannelControllerCapabilityAgent::hasReportableStateProperties() {
    ACSDK_DEBUG9(LX(__func__));
    return m_isRetrievable || m_isProactivelyReported;
}

void AlexaChannelControllerCapabilityAgent::cancelDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("cancelDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    if (!info->directive->getEndpoint().hasValue() ||
        info->directive->getEndpoint().value().endpointId != m_endpointId) {
        ACSDK_WARN(LX("cancelDirective").d("reason", "notExpectedEndpointId"));
    }
    removeDirective(info);
}

DirectiveHandlerConfiguration AlexaChannelControllerCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG9(LX(__func__));
    DirectiveHandlerConfiguration configuration;
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[{NAMESPACE, NAME_CHANGECHANNEL, m_endpointId}] = neitherNonBlockingPolicy;
    configuration[{NAMESPACE, NAME_SKIPCHANNELS, m_endpointId}] = neitherNonBlockingPolicy;

    return configuration;
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AlexaChannelControllerCapabilityAgent::
    getCapabilityConfigurations() {
    auto additionalConfigurations = CapabilityConfiguration::AdditionalConfigurations();
    auto lineupConfigurationJson = buildLineupConfigurationJson();
    if (!lineupConfigurationJson.empty())
        additionalConfigurations[CAPABILITY_CONFIGURATION_LINEUP_KEY] = lineupConfigurationJson;

    CapabilityConfiguration configuration{CapabilityConfiguration::ALEXA_INTERFACE_TYPE,
                                          NAMESPACE,
                                          INTERFACE_VERSION,
                                          Optional<std::string>(),  // instance
                                          Optional<CapabilityConfiguration::Properties>(
                                              {m_isRetrievable, m_isProactivelyReported, {CHANNELSTATE_PROPERTY_NAME}}),
                                          additionalConfigurations};
    return {std::make_shared<avsCommon::avs::CapabilityConfiguration>(configuration)};
}

std::string AlexaChannelControllerCapabilityAgent::buildLineupConfigurationJson() {
    auto configRoot =
        avsCommon::utils::configuration::ConfigurationNode::getRoot()[CHANNEL_CONTROLLER_ROOT_KEY]
                                                                     [CAPABILITY_CONFIGURATION_LINEUP_KEY];
    if (!configRoot) {
        ACSDK_DEBUG9(LX(__func__).m("noLineupConfigurationRoot"));
        return "";
    }

    const std::string errorEvent = "buildingLineupFailed";
    const std::string errorReasonKey = "reason";
    const std::string errorValueKey = "key";

    std::string operatorName;
    if (!configRoot.getString(CAPABILITY_CONFIGURATION_LINEUP_OPERATOR_NAME_KEY, &operatorName)) {
        ACSDK_ERROR(LX(errorEvent)
                        .d(errorReasonKey, "missingLineupOperatorName")
                        .d(errorValueKey, CAPABILITY_CONFIGURATION_LINEUP_OPERATOR_NAME_KEY));
        return "";
    }

    std::string lineupName;
    if (!configRoot.getString(CAPABILITY_CONFIGURATION_LINEUP_NAME_KEY, &lineupName)) {
        ACSDK_DEBUG9(LX(__func__).m("missingLineupName"));
    }

    std::string postalCode;
    if (!configRoot.getString(CAPABILITY_CONFIGURATION_LINEUP_POSTAL_CODE_KEY, &postalCode)) {
        ACSDK_DEBUG9(LX(__func__).m("missingLineupPostalCode"));
    }

    std::string type;
    if (!configRoot.getString(CAPABILITY_CONFIGURATION_LINEUP_TYPE_KEY, &type)) {
        ACSDK_ERROR(LX(errorEvent)
                        .d(errorReasonKey, "missingLineupType")
                        .d(errorValueKey, CAPABILITY_CONFIGURATION_LINEUP_TYPE_KEY));
        return "";
    }

    JsonGenerator jsonGenerator;

    if (!operatorName.empty()) {
        jsonGenerator.addMember(CAPABILITY_CONFIGURATION_LINEUP_OPERATOR_NAME_KEY, operatorName);
    }

    if (!lineupName.empty()) {
        jsonGenerator.addMember(CAPABILITY_CONFIGURATION_LINEUP_NAME_KEY, lineupName);
    }

    if (!postalCode.empty()) {
        jsonGenerator.addMember(CAPABILITY_CONFIGURATION_LINEUP_POSTAL_CODE_KEY, postalCode);
    }

    if (!type.empty()) {
        jsonGenerator.addMember(CAPABILITY_CONFIGURATION_LINEUP_TYPE_KEY, type);
    }

    ACSDK_DEBUG9(LX(__func__).sensitive("configuration", jsonGenerator.toString()));
    return jsonGenerator.toString();
}

void AlexaChannelControllerCapabilityAgent::doShutdown() {
    if (m_isProactivelyReported) {
        m_channelController->removeObserver(shared_from_this());
    }
    m_executor.shutdown();
    m_channelController.reset();
    m_responseSender.reset();
    if (m_isRetrievable) {
        m_contextManager->removeStateProvider({NAMESPACE, CHANNELSTATE_PROPERTY_NAME, m_endpointId});
    }
    m_contextManager.reset();
}

void AlexaChannelControllerCapabilityAgent::removeDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void AlexaChannelControllerCapabilityAgent::executeSetHandlingCompleted(
    std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void AlexaChannelControllerCapabilityAgent::executeUnknownDirective(
    std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    ExceptionErrorType type) {
    ACSDK_ERROR(LX("executeUnknownDirectiveFailed")
                    .d("reason", "unknownDirective")
                    .d("namespace", info->directive->getNamespace())
                    .d("name", info->directive->getName()));

    const std::string exceptionMessage =
        "unexpected directive " + info->directive->getNamespace() + ":" + info->directive->getName();

    sendExceptionEncounteredAndReportFailed(info, exceptionMessage, type);
}

void AlexaChannelControllerCapabilityAgent::executeProvideState(
    const CapabilityTag& stateProviderName,
    const ContextRequestToken contextRequestToken) {
    ACSDK_DEBUG3(LX("executeProvideState"));
    auto isError = false;
    if (stateProviderName.endpointId != m_endpointId) {
        ACSDK_ERROR(LX("provideStateFailed")
                        .d("reason", "notExpectedEndpointId")
                        .sensitive("endpointId", stateProviderName.endpointId));
        isError = true;
    }
    if (stateProviderName.name != CHANNELSTATE_PROPERTY_NAME) {
        ACSDK_ERROR(LX("provideStateFailed").d("reason", "notExpectedName").d("name", stateProviderName.name));
        isError = true;
    }
    if (!m_isRetrievable) {
        ACSDK_ERROR(LX("provideStateFailed").d("reason", "provideStateOnNotRetrievableProperty"));
        isError = true;
    }

    if (isError) {
        m_contextManager->provideStateUnavailableResponse(stateProviderName, contextRequestToken, false);
        return;
    }

    std::unique_ptr<Channel> channel = m_channelController->getCurrentChannel();
    if (channel == nullptr) {
        ACSDK_ERROR(LX("provideStateFailed").d("reason", "EmptyChannelState"));
        m_contextManager->provideStateUnavailableResponse(stateProviderName, contextRequestToken, false);
        return;
    }
    m_contextManager->provideStateResponse(
        stateProviderName, CapabilityState(buildCapabilityStateString(*channel)), contextRequestToken);
}

void AlexaChannelControllerCapabilityAgent::sendResponseEvent(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    const ChannelControllerInterface::Response& result,
    std::unique_ptr<Channel> currentChannel) {
    switch (result.type) {
        case ChannelControllerInterface::Response::Type::SUCCESS: {
            m_responseSender->sendResponseEvent(
                info->directive->getInstance(),
                info->directive->getCorrelationToken(),
                AVSMessageEndpoint(m_endpointId),
                buildCapabilityStateString(*currentChannel));
            break;
        }
        case ChannelControllerInterface::Response::Type::FAILED_TOO_MANY_FAILED_ATTEMPTS:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, result.errorMessage);
            break;
        case ChannelControllerInterface::Response::Type::FAILED_ENDPOINT_UNREACHABLE:
            sendAlexaErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::ErrorResponseType::ENDPOINT_UNREACHABLE,
                result.errorMessage);
            break;
        case ChannelControllerInterface::Response::Type::FAILED_INVALID_VALUE:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INVALID_VALUE, result.errorMessage);
            break;
        case ChannelControllerInterface::Response::Type::FAILED_ACTION_NOT_PERMITTED_FOR_CONTENT:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::ACTION_NOT_PERMITTED_FOR_CONTENT,
                result.errorMessage);
            break;
        case ChannelControllerInterface::Response::Type::FAILED_NOT_SUBSCRIBED:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::NOT_SUBSCRIBED,
                result.errorMessage);
            break;
        case ChannelControllerInterface::Response::Type::FAILED_INTERNAL_ERROR:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, result.errorMessage);
            break;
    }
}

void AlexaChannelControllerCapabilityAgent::sendAlexaErrorResponse(
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

void AlexaChannelControllerCapabilityAgent::sendAlexaVideoErrorResponse(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType alexaVideoErrorResponseType,
    const std::string& responseMessage) {
    std::string payload =
        R"({"type":")" +
        AlexaInterfaceMessageSenderInterface::alexaVideoErrorResponseToString(alexaVideoErrorResponseType) +
        R"(", "message":")" + responseMessage + R"("})";

    m_responseSender->sendErrorResponseEvent(
        info->directive->getInstance(),
        info->directive->getCorrelationToken(),
        AVSMessageEndpoint(m_endpointId),
        NAMESPACE_ALEXA_VIDEO_ERRORRESPONSE,
        payload);
}

void AlexaChannelControllerCapabilityAgent::onChannelChanged(std::unique_ptr<Channel> channel) {
    ACSDK_DEBUG9(LX(__func__));
    if (!m_isProactivelyReported) {
        ACSDK_ERROR(LX("onChannelChangeFailed").d("reason", "invalidOnChannelChangedCall"));
        return;
    }
    std::string capabilityStateString = buildCapabilityStateString(*channel);
    m_executor.execute([this, capabilityStateString] {
        ACSDK_DEBUG9(LX("onChannelChangedInExecutor"));
        m_contextManager->reportStateChange(
            CapabilityTag(NAMESPACE, CHANNELSTATE_PROPERTY_NAME, m_endpointId),
            CapabilityState(capabilityStateString),
            AlexaStateChangeCauseType::VOICE_INTERACTION);
    });
}

std::string AlexaChannelControllerCapabilityAgent::buildCapabilityStateString(const Channel& channel) {
    rapidjson::Document payloadJson(rapidjson::kObjectType);
    auto& alloc = payloadJson.GetAllocator();

    payloadJson.AddMember("number", channel.getNumber(), alloc);
    payloadJson.AddMember("callSign", channel.getCallSign(), alloc);
    payloadJson.AddMember("affiliateCallsign", channel.getAffiliateCallSign(), alloc);
    payloadJson.AddMember("uri", channel.getURI(), alloc);

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    payloadJson.Accept(writer);

    return sb.GetString();
}

}  // namespace alexaChannelController
}  // namespace alexaClientSDK
