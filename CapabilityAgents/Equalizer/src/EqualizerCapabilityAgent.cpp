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

#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>

#include "Equalizer/EqualizerCapabilityAgent.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace equalizer {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::audio;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::error;
using namespace avsCommon::utils::json::jsonUtils;
using namespace avsCommon::utils::logger;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG{"EqualizerController"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The EqualizerController interface namespace.
static const std::string NAMESPACE = "EqualizerController";

/// The EqualizerController state portion of the Context.
static const NamespaceAndName EQUALIZER_STATE{NAMESPACE, "EqualizerState"};

/// The @c SetBands directive identifier
static const NamespaceAndName DIRECTIVE_SETBANDS{NAMESPACE, "SetBands"};

/// The @c AdjustBands directive identifier
static const NamespaceAndName DIRECTIVE_ADJUSTBANDS{NAMESPACE, "AdjustBands"};

/// The @c ResetBands directive identifier
static const NamespaceAndName DIRECTIVE_RESETBANDS{NAMESPACE, "ResetBands"};

/// The @c SetMode directive identifier
static const NamespaceAndName DIRECTIVE_SETMODE{NAMESPACE, "SetMode"};

/// The @c EqualizerChanged event identifier
static const NamespaceAndName EVENT_EQUALIZERCHANGED{NAMESPACE, "EqualizerChanged"};

/// Equalizer capability constants
/// Equalizer interface type
static const std::string EQUALIZER_JSON_INTERFACE_TYPE = "AlexaInterface";

/// Equalizer interface name
static const std::string EQUALIZER_JSON_INTERFACE_NAME = "EqualizerController";

/// Equalizer interface version
static const std::string EQUALIZER_JSON_INTERFACE_VERSION = "1.0";

/// Name for "bands" JSON branch.
static constexpr char JSON_KEY_BANDS[] = "bands";
/// Name for "supported" JSON branch.
static constexpr char JSON_KEY_SUPPORTED[] = "supported";
/// Name for "name" JSON value.
static constexpr char JSON_KEY_NAME[] = "name";
/// Name for "level" JSON value.
static constexpr char JSON_KEY_LEVEL[] = "level";
/// Name for "range" JSON branch.
static constexpr char JSON_KEY_RANGE[] = "range";
/// Name for "minimum" JSON value.
static constexpr char JSON_KEY_MINIMUM[] = "minimum";
/// Name for "maximum" JSON value.
static constexpr char JSON_KEY_MAXIMUM[] = "maximum";
/// Name for "modes" JSON branch.
static constexpr char JSON_KEY_MODES[] = "modes";
/// Name for "mode" JSON value.
static constexpr char JSON_KEY_MODE[] = "mode";
/// Name for "levelDelta" JSON value.
static constexpr char JSON_KEY_LEVELDELTA[] = "levelDelta";
/// Name for "levelDirection" JSON value.
static constexpr char JSON_KEY_LEVELDIRECTION[] = "levelDirection";
/// String representing positive level adjustment.
static constexpr char LEVEL_DIRECTION_UP[] = "UP";
/// String representing negative level adjustment.
static constexpr char LEVEL_DIRECTION_DOWN[] = "DOWN";

/// Adjustment value used by AVS by default, in dB, when you, for example, say "Alexa, raise the bass".
static constexpr int64_t AVS_DEFAULT_ADJUST_DELTA = 1;

std::shared_ptr<EqualizerCapabilityAgent> EqualizerCapabilityAgent::create(
    std::shared_ptr<alexaClientSDK::equalizer::EqualizerController> equalizerController,
    std::shared_ptr<CapabilitiesDelegateInterface> capabilitiesDelegate,
    std::shared_ptr<EqualizerStorageInterface> equalizerStorage,
    std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<MessageSenderInterface> messageSender) {
    if (nullptr == equalizerController) {
        ACSDK_ERROR(LX("createFailed").d("reason", "equalizerControllerNull"));
        return nullptr;
    }

    if (nullptr == capabilitiesDelegate) {
        ACSDK_ERROR(LX("createFailed").d("reason", "capabilitiesDelegateNull"));
        return nullptr;
    }

    if (nullptr == equalizerStorage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "equalizerStoragelerNull"));
        return nullptr;
    }

    if (nullptr == contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "contextManagerNull"));
        return nullptr;
    }

    if (nullptr == messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "messageSenderNull"));
        return nullptr;
    }

    if (nullptr == customerDataManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "customerDataManagerNull"));
        return nullptr;
    }

    if (nullptr == exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "exceptionEncounteredSenderNull"));
        return nullptr;
    }

    auto equalizerCA = std::shared_ptr<EqualizerCapabilityAgent>(new EqualizerCapabilityAgent(
        equalizerController,
        capabilitiesDelegate,
        equalizerStorage,
        customerDataManager,
        exceptionEncounteredSender,
        contextManager,
        messageSender));

    equalizerController->addListener(equalizerCA);

    return equalizerCA;
}

/**
 * Builds a JSON string containing an Equalizer state.
 *
 * @param state @c EqualizerState to be serialized to JSON.
 * @return JSON string with serialized Equalizer State.
 */
static std::string buildEQStateJson(const EqualizerState& state) {
    rapidjson::Document payload(rapidjson::kObjectType);
    auto& allocator = payload.GetAllocator();
    rapidjson::Value bandsBranch(rapidjson::kArrayType);

    for (const auto& bandIter : state.bandLevels) {
        EqualizerBand band = bandIter.first;
        int bandLevel = bandIter.second;

        rapidjson::Value bandDesc(rapidjson::kObjectType);
        bandDesc.AddMember(JSON_KEY_NAME, equalizerBandToString(band), allocator);
        bandDesc.AddMember(JSON_KEY_LEVEL, bandLevel, allocator);
        bandsBranch.PushBack(bandDesc, allocator);
    }

    payload.AddMember(JSON_KEY_BANDS, bandsBranch, allocator);

    if (EqualizerMode::NONE != state.mode) {
        payload.AddMember(JSON_KEY_MODE, equalizerModeToString(state.mode), allocator);
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("buildEQStateJsonFailed").d("reason", "writerRefusedJsonObject"));
        return "";
    }

    return buffer.GetString();
}

EqualizerCapabilityAgent::EqualizerCapabilityAgent(
    std::shared_ptr<alexaClientSDK::equalizer::EqualizerController> equalizerController,
    std::shared_ptr<CapabilitiesDelegateInterface> capabilitiesDelegate,
    std::shared_ptr<EqualizerStorageInterface> equalizerStorage,
    std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<MessageSenderInterface> messageSender) :
        CapabilityAgent{NAMESPACE, exceptionEncounteredSender},
        RequiresShutdown{"Equalizer"},
        CustomerDataHandler{customerDataManager},
        m_equalizerController{equalizerController},
        m_capabilitiesDelegate{capabilitiesDelegate},
        m_equalizerStorage{equalizerStorage},
        m_messageSender{messageSender},
        m_contextManager{contextManager} {
    generateCapabilityConfiguration();

    auto payload = buildEQStateJson(equalizerController->getCurrentState());
    if (0 == payload.length()) {
        ACSDK_ERROR(LX("EqualizerCapabilityAgentFailed").d("reason", "Failed to serialize equalizer state."));
        return;
    }
    m_contextManager->setState(EQUALIZER_STATE, payload, avsCommon::avs::StateRefreshPolicy::NEVER);
}

void EqualizerCapabilityAgent::generateCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, EQUALIZER_JSON_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, EQUALIZER_JSON_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, EQUALIZER_JSON_INTERFACE_VERSION});

    // Build configuration capabilities
    auto equalizerConfiguration = m_equalizerController->getConfiguration();

    rapidjson::Document payload(rapidjson::kObjectType);
    auto& allocator = payload.GetAllocator();

    rapidjson::Value bandsBranch(rapidjson::kObjectType);
    rapidjson::Value modesBranch(rapidjson::kObjectType);
    rapidjson::Value bandsSupportedBranch(rapidjson::kArrayType);
    rapidjson::Value bandsRangeBranch(rapidjson::kObjectType);
    rapidjson::Value modesSupportedBranch(rapidjson::kArrayType);

    // Build supported bands
    for (auto band : equalizerConfiguration->getSupportedBands()) {
        rapidjson::Value bandDesc(rapidjson::kObjectType);
        bandDesc.AddMember(JSON_KEY_NAME, equalizerBandToString(band), allocator);
        bandsSupportedBranch.PushBack(bandDesc, allocator);
    }

    bandsRangeBranch.AddMember(JSON_KEY_MINIMUM, equalizerConfiguration->getMinBandLevel(), allocator);
    bandsRangeBranch.AddMember(JSON_KEY_MAXIMUM, equalizerConfiguration->getMaxBandLevel(), allocator);

    bandsBranch.AddMember(JSON_KEY_SUPPORTED, bandsSupportedBranch, allocator);
    bandsBranch.AddMember(JSON_KEY_RANGE, bandsRangeBranch, allocator);

    if (!equalizerConfiguration->getSupportedModes().empty()) {
        // Building modes
        for (auto mode : equalizerConfiguration->getSupportedModes()) {
            rapidjson::Value modeDesc(rapidjson::kObjectType);
            modeDesc.AddMember(JSON_KEY_NAME, equalizerModeToString(mode), allocator);
            modesSupportedBranch.PushBack(modeDesc, allocator);
        }

        modesBranch.AddMember(JSON_KEY_SUPPORTED, modesSupportedBranch, allocator);
        payload.AddMember(JSON_KEY_MODES, modesBranch, allocator);
    }

    // Finish up the doc
    payload.AddMember(JSON_KEY_BANDS, bandsBranch, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("generateCapabilityConfigurationFailed").d("reason", "writerRefusedJsonObject"));
        return;
    }

    configMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, buffer.GetString()});

    m_capabilityConfigurations.insert(std::make_shared<CapabilityConfiguration>(configMap));
}

avsCommon::avs::DirectiveHandlerConfiguration EqualizerCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));

    DirectiveHandlerConfiguration configuration;
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);

    configuration[DIRECTIVE_SETBANDS] = neitherNonBlockingPolicy;
    configuration[DIRECTIVE_ADJUSTBANDS] = neitherNonBlockingPolicy;
    configuration[DIRECTIVE_RESETBANDS] = neitherNonBlockingPolicy;
    configuration[DIRECTIVE_SETMODE] = neitherNonBlockingPolicy;

    return configuration;
}

void EqualizerCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void EqualizerCapabilityAgent::preHandleDirective(
    std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
    // No-op
}

/**
 * Parses a directive payload JSON and returns a parsed document object.
 *
 * @param payload JSON string to parse.
 * @param[out] document Pointer to a parsed document.
 * @return True if parsing was successful, false otherwise.
 */
bool parseDirectivePayload(const std::string& payload, Document* document) {
    ACSDK_DEBUG5(LX(__func__));
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

void EqualizerCapabilityAgent::handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    m_executor.submit([this, info] {
        const std::string directiveName = info->directive->getName();

        Document payload(kObjectType);
        if (!parseDirectivePayload(info->directive->getPayload(), &payload)) {
            sendExceptionEncounteredAndReportFailed(
                info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        if (directiveName == DIRECTIVE_SETBANDS.name) {
            if (!handleSetBandsDirective(info, payload)) {
                return;
            }
        } else if (directiveName == DIRECTIVE_ADJUSTBANDS.name) {
            if (!handleAdjustBandsDirective(info, payload)) {
                return;
            }
        } else if (directiveName == DIRECTIVE_RESETBANDS.name) {
            if (!handleResetBandsDirective(info, payload)) {
                return;
            }
        } else if (directiveName == DIRECTIVE_SETMODE.name) {
            if (!handleSetModeDirective(info, payload)) {
                return;
            }
        } else {
            sendExceptionEncounteredAndReportFailed(
                info, "Unexpected Directive", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        if (info->result) {
            info->result->setCompleted();
        }
        removeDirective(info->directive->getMessageId());
    });
}

void EqualizerCapabilityAgent::cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
    removeDirective(info->directive->getMessageId());
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> EqualizerCapabilityAgent::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

void EqualizerCapabilityAgent::doShutdown() {
    m_equalizerController->removeListener(shared_from_this());
}

void EqualizerCapabilityAgent::clearData() {
    m_equalizerStorage->clear();
}

void EqualizerCapabilityAgent::onEqualizerStateChanged(const EqualizerState& state) {
    auto payload = buildEQStateJson(state);
    if (0 == payload.length()) {
        ACSDK_ERROR(LX("onEqualizerStateChangedFailed").d("reason", "Failed to serialize equalizer state."));
        return;
    }
    m_contextManager->setState(EQUALIZER_STATE, payload, avsCommon::avs::StateRefreshPolicy::NEVER);
    auto eventJson = buildJsonEventString(EVENT_EQUALIZERCHANGED.name, "", payload);
    auto request = std::make_shared<MessageRequest>(eventJson.second);

    m_messageSender->sendMessage(request);
}

bool EqualizerCapabilityAgent::handleSetBandsDirective(
    std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
    Document& payload) {
    rapidjson::Value::ConstMemberIterator it;

    if (!findNode(payload, JSON_KEY_BANDS, &it) || !it->value.IsArray()) {
        sendExceptionEncounteredAndReportFailed(
            info, "'bands' node not found or is not an array.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    bool shouldFixConfiguration = false;

    auto eqConfig = m_equalizerController->getConfiguration();

    EqualizerBandLevelMap bandLevelMap;
    for (const auto& bandDesc : it->value.GetArray()) {
        std::string bandName;
        if (!retrieveValue(bandDesc, JSON_KEY_NAME, &bandName)) {
            sendExceptionEncounteredAndReportFailed(
                info, "Invalid 'bands[].name' value.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return false;
        }
        SuccessResult<EqualizerBand> bandResult = stringToEqualizerBand(bandName);
        if (!bandResult.isSucceeded() || !eqConfig->isBandSupported(bandResult.value())) {
            ACSDK_WARN(LX(__func__).d("band", bandName).m("Unsupported band"));
            shouldFixConfiguration = true;
            continue;
        }

        EqualizerBand band = bandResult.value();

        int64_t bandLevel = 0;
        if (!retrieveValue(bandDesc, JSON_KEY_LEVEL, &bandLevel)) {
            sendExceptionEncounteredAndReportFailed(
                info, "Invalid 'bands[].level' value.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return false;
        }

        int minValue = eqConfig->getMinBandLevel();
        int maxValue = eqConfig->getMaxBandLevel();

        if (bandLevel < minValue) {
            ACSDK_WARN(LX(__func__).d("level", bandLevel).d("minimum", minValue).m("Band level below minimum"));
            shouldFixConfiguration = true;
            bandLevel = minValue;
        } else if (bandLevel > maxValue) {
            ACSDK_WARN(LX(__func__).d("level", bandLevel).d("maximum", maxValue).m("Band level above maximum"));
            shouldFixConfiguration = true;
            bandLevel = maxValue;
        }
        bandLevelMap[band] = bandLevel;
    }  // For all bands

    m_equalizerController->setBandLevels(bandLevelMap);

    if (shouldFixConfiguration) {
        m_exceptionEncounteredSender->sendExceptionEncountered(
            info->directive->getUnparsedDirective(),
            ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED,
            "Unsupported EQ band or level values recieved.");
        fixConfigurationDesynchronization();
    }

    return true;
}

bool EqualizerCapabilityAgent::handleAdjustBandsDirective(
    std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
    Document& payload) {
    rapidjson::Value::ConstMemberIterator it;

    if (!findNode(payload, JSON_KEY_BANDS, &it) || !it->value.IsArray()) {
        sendExceptionEncounteredAndReportFailed(
            info, "'bands' node not found or is not an array.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    bool shouldFixConfiguration = false;

    auto eqConfig = m_equalizerController->getConfiguration();

    EqualizerBandLevelMap bandLevelMap;
    for (const auto& bandDesc : it->value.GetArray()) {
        std::string bandName;
        if (!retrieveValue(bandDesc, JSON_KEY_NAME, &bandName)) {
            sendExceptionEncounteredAndReportFailed(
                info, "Invalid 'bands[].name' value.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return false;
        }
        SuccessResult<EqualizerBand> bandResult = stringToEqualizerBand(bandName);
        if (!bandResult.isSucceeded() || !eqConfig->isBandSupported(bandResult.value())) {
            ACSDK_WARN(LX(__func__).d("band", bandName).m("Unsupported band"));
            shouldFixConfiguration = true;
            continue;
        }

        EqualizerBand band = bandResult.value();

        // Assume default delta if none provided.
        int64_t bandLevelDelta = AVS_DEFAULT_ADJUST_DELTA;
        retrieveValue(bandDesc, JSON_KEY_LEVELDELTA, &bandLevelDelta);

        std::string direction;
        if (!retrieveValue(bandDesc, JSON_KEY_LEVELDIRECTION, &direction)) {
            sendExceptionEncounteredAndReportFailed(
                info, "Invalid 'bands[].levelDelta' value.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return false;
        }

        bool isDirectionUp;
        if (direction == LEVEL_DIRECTION_UP) {
            isDirectionUp = true;
        } else if (direction == LEVEL_DIRECTION_DOWN) {
            isDirectionUp = false;
        } else {
            sendExceptionEncounteredAndReportFailed(
                info,
                "Invalid 'bands[].levelDirection', expected 'UP' or 'DOWN'.",
                ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return false;
        }

        int minValue = eqConfig->getMinBandLevel();
        int maxValue = eqConfig->getMaxBandLevel();

        auto bandLevelResult = m_equalizerController->getBandLevel(band);
        /// Must always succeed since band is validated above.
        int bandLevel = bandLevelResult.value();

        bandLevel += isDirectionUp ? bandLevelDelta : -bandLevelDelta;

        if (bandLevel < minValue) {
            ACSDK_WARN(
                LX(__func__).d("level", bandLevel).d("minimum", minValue).m("Adjusted band level below minimum"));
            shouldFixConfiguration = true;
            bandLevel = minValue;
        } else if (bandLevel > maxValue) {
            ACSDK_WARN(
                LX(__func__).d("level", bandLevel).d("maximum", maxValue).m("Adjusted band level above maximum"));
            shouldFixConfiguration = true;
            bandLevel = maxValue;
        }
        bandLevelMap[band] = bandLevel;
    }  // For all bands

    m_equalizerController->setBandLevels(bandLevelMap);

    if (shouldFixConfiguration) {
        m_exceptionEncounteredSender->sendExceptionEncountered(
            info->directive->getUnparsedDirective(),
            ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED,
            "Unsupported EQ band or level values recieved.");
        fixConfigurationDesynchronization();
    }

    return true;
}

bool EqualizerCapabilityAgent::handleResetBandsDirective(
    std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
    Document& payload) {
    rapidjson::Value::ConstMemberIterator it;

    if (!findNode(payload, JSON_KEY_BANDS, &it) || !it->value.IsArray()) {
        sendExceptionEncounteredAndReportFailed(
            info, "'bands' node not found or is not an array.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    bool shouldFixConfiguration = false;

    auto eqConfig = m_equalizerController->getConfiguration();

    std::set<EqualizerBand> bandsToReset;
    for (const auto& bandDesc : it->value.GetArray()) {
        std::string bandName;
        if (!retrieveValue(bandDesc, JSON_KEY_NAME, &bandName)) {
            sendExceptionEncounteredAndReportFailed(
                info, "Invalid 'bands[].name' value.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return false;
        }
        SuccessResult<EqualizerBand> bandResult = stringToEqualizerBand(bandName);
        if (!bandResult.isSucceeded() || !eqConfig->isBandSupported(bandResult.value())) {
            ACSDK_WARN(LX(__func__).d("band", bandName).m("Unsupported band"));
            shouldFixConfiguration = true;
            continue;
        }
        bandsToReset.insert(bandResult.value());
    }  // For all bands

    if (!bandsToReset.empty()) {
        m_equalizerController->resetBands(bandsToReset);
    }

    if (shouldFixConfiguration) {
        m_exceptionEncounteredSender->sendExceptionEncountered(
            info->directive->getUnparsedDirective(),
            ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED,
            "Unsupported EQ band recieved.");
        fixConfigurationDesynchronization();
    }

    return true;
}

bool EqualizerCapabilityAgent::handleSetModeDirective(
    std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
    Document& payload) {
    auto eqConfig = m_equalizerController->getConfiguration();

    std::string modeName;
    if (!retrieveValue(payload, JSON_KEY_MODE, &modeName)) {
        sendExceptionEncounteredAndReportFailed(
            info, "Invalid or missing 'mode' value.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    SuccessResult<EqualizerMode> modeResult = stringToEqualizerMode(modeName);
    if (!modeResult.isSucceeded() || !eqConfig->isModeSupported(modeResult.value())) {
        ACSDK_WARN(LX(__func__).d("mode", modeName).m("Unsupported mode"));
        m_exceptionEncounteredSender->sendExceptionEncountered(
            info->directive->getUnparsedDirective(),
            ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED,
            "Unsupported EQ band recieved.");
        fixConfigurationDesynchronization();
    } else {
        m_equalizerController->setCurrentMode(modeResult.value());
    }

    return true;
}

void EqualizerCapabilityAgent::fixConfigurationDesynchronization() {
    m_capabilitiesDelegate->invalidateCapabilities();
}

}  // namespace equalizer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
