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

#include "acsdkAlexaVideoRecorder/AlexaVideoRecorderCapabilityAgent.h"

#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Timing/TimeUtils.h>

namespace alexaClientSDK {
namespace acsdkAlexaVideoRecorder {

using namespace acsdkAlexaVideoCommon;
using namespace avsCommon::avs;
using namespace avsCommon;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::timing;

using namespace acsdkAlexaVideoRecorderInterfaces;

/// String to identify log entries originating from this file.
#define TAG "AlexaVideoRecorderCapabilityAgent"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const std::string NAMESPACE{"Alexa.VideoRecorder"};

/// The namespace for Alexa.Video.ErrorResponse.
static const std::string NAMESPACE_ALEXA_VIDEO_ERRORRESPONSE{"Alexa.Video"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for SearchAndRecord directive
static const std::string NAME_SEARCH_AND_RECORD{"SearchAndRecord"};

/// The name for SearchAndRecord response event
static const std::string NAME_SEARCH_AND_RECORD_RESPONSE{"SearchAndRecord.Response"};

/// The name for CancelRecording directive
static const std::string NAME_CANCEL_RECORDING{"CancelRecording"};

/// The name for CancelRecording directive
static const std::string NAME_DELETE_RECORDING{"DeleteRecording"};

/// The name for state property for extended GUI
static const std::string EXTENDED_GUI_STATE_NAME{"isExtendedRecordingGUIShown"};

/// The name for state property for storage level
static const std::string STORAGE_LEVEL_STATE_NAME{"storageLevel"};

/// Json key for parsing entities.
static const std::string ENTITIES("entities");

/// Json key for parsing type.
static const std::string TYPE("type");

/// Json key for parsing quantifier.
static const std::string QUANTIFIER("quantifier");

/// Json key for parsing name.
static const std::string NAME("name");

/// Json key for parsing timeWindow.
static const std::string TIME_WINDOW("timeWindow");

/// Json key for parsing start.
static const std::string START("start");

/// Json key for parsing end.
static const std::string END("end");

/// A map to convert string from json to Quantifier enum
const std::unordered_map<std::string, VideoRecorderRequest::Quantifier> stringToQuantifierMap = {
    {"ALL", VideoRecorderRequest::Quantifier::ALL},
    {"NEXT", VideoRecorderRequest::Quantifier::NEXT},
    {"NEW", VideoRecorderRequest::Quantifier::NEW},
    {"WATCHED", VideoRecorderRequest::Quantifier::WATCHED}};

std::shared_ptr<AlexaVideoRecorderCapabilityAgent> AlexaVideoRecorderCapabilityAgent::create(
    EndpointIdentifier endpointId,
    std::shared_ptr<VideoRecorderInterface> videoRecorder,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender) {
    if (endpointId.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyEndpointId"));
        return nullptr;
    }
    if (!videoRecorder) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullVideoRecorder"));
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

    auto videoRecorderCapabilityAgent =
        std::shared_ptr<AlexaVideoRecorderCapabilityAgent>(new AlexaVideoRecorderCapabilityAgent(
            endpointId,
            std::move(videoRecorder),
            contextManager,
            std::move(responseSender),
            std::move(exceptionSender)));

    if (!videoRecorderCapabilityAgent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "instantiationFailed"));
        return nullptr;
    }

    const avs::CapabilityTag extendedGUICapabilityTag{NAMESPACE, EXTENDED_GUI_STATE_NAME, endpointId};
    contextManager->addStateProvider(extendedGUICapabilityTag, videoRecorderCapabilityAgent);

    const avs::CapabilityTag storageLevelCapabilityTag{NAMESPACE, STORAGE_LEVEL_STATE_NAME, endpointId};
    contextManager->addStateProvider(storageLevelCapabilityTag, videoRecorderCapabilityAgent);
    return videoRecorderCapabilityAgent;
}

AlexaVideoRecorderCapabilityAgent::AlexaVideoRecorderCapabilityAgent(
    EndpointIdentifier endpointId,
    std::shared_ptr<VideoRecorderInterface> videoRecorder,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"AlexaVideoRecorderCapabilityAgent"},
        m_endpointId{std::move(endpointId)},
        m_videoRecorder{std::move(videoRecorder)},
        m_contextManager{std::move(contextManager)},
        m_responseSender{std::move(responseSender)} {
}

void AlexaVideoRecorderCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void AlexaVideoRecorderCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("preHandleDirective"));
    // do nothing.
}

void AlexaVideoRecorderCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    m_executor.submit([this, info] {
        ACSDK_DEBUG5(LX("handleDirectiveInExecutor").d("Payload", info->directive->getPayload()));
        const std::string directiveName = info->directive->getName();
        if (!info->directive->getEndpoint().hasValue() ||
            (info->directive->getEndpoint().value().endpointId != m_endpointId)) {
            executeUnknownDirective(info, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        auto payload = info->directive->getPayload();
        std::unique_ptr<VideoRecorderRequest> videoRecorderRequestContext = parseDirectivePayload(payload);
        if (!videoRecorderRequestContext) {
            ACSDK_ERROR(
                LX("handleDirectiveFailed").d("reason", "unableToParseDirectivePayload").sensitive("payload", payload));
            executeUnknownDirective(info, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        VideoRecorderInterface::Response result;
        if (directiveName == NAME_SEARCH_AND_RECORD) {
            result = m_videoRecorder->searchAndRecord(std::move(videoRecorderRequestContext));
        } else if (directiveName == NAME_CANCEL_RECORDING) {
            result = m_videoRecorder->cancelRecording(std::move(videoRecorderRequestContext));
        } else if (directiveName == NAME_DELETE_RECORDING) {
            result = m_videoRecorder->deleteRecording(std::move(videoRecorderRequestContext));
        } else {
            ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "unexpectedDirective").d("name", directiveName));
            executeUnknownDirective(info, ExceptionErrorType::UNSUPPORTED_OPERATION);
            return;
        }

        executeSetHandlingCompleted(info);
        sendResponseEvent(info, result);
    });
}

void AlexaVideoRecorderCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
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

DirectiveHandlerConfiguration AlexaVideoRecorderCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));
    DirectiveHandlerConfiguration configuration;
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[{NAMESPACE, NAME_SEARCH_AND_RECORD, m_endpointId}] = neitherNonBlockingPolicy;
    configuration[{NAMESPACE, NAME_CANCEL_RECORDING, m_endpointId}] = neitherNonBlockingPolicy;
    configuration[{NAMESPACE, NAME_DELETE_RECORDING, m_endpointId}] = neitherNonBlockingPolicy;

    return configuration;
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AlexaVideoRecorderCapabilityAgent::
    getCapabilityConfigurations() {
    auto additionalConfigurations = CapabilityConfiguration::AdditionalConfigurations();
    CapabilityConfiguration configuration{CapabilityConfiguration::ALEXA_INTERFACE_TYPE,
                                          NAMESPACE,
                                          INTERFACE_VERSION,
                                          Optional<std::string>(),  // instance
                                          Optional<CapabilityConfiguration::Properties>(),
                                          additionalConfigurations};
    return {std::make_shared<avsCommon::avs::CapabilityConfiguration>(configuration)};
}

void AlexaVideoRecorderCapabilityAgent::doShutdown() {
    ACSDK_DEBUG9(LX(__func__));
    if (!m_executor.isShutdown()) {
        m_executor.shutdown();
    }

    const avs::CapabilityTag extendedGUICapabilityTag{NAMESPACE, EXTENDED_GUI_STATE_NAME, m_endpointId};
    m_contextManager->removeStateProvider(extendedGUICapabilityTag);

    const avs::CapabilityTag storageLevelCapabilityTag{NAMESPACE, STORAGE_LEVEL_STATE_NAME, m_endpointId};
    m_contextManager->removeStateProvider(storageLevelCapabilityTag);

    m_videoRecorder.reset();
    m_responseSender.reset();
    m_contextManager.reset();
}

void AlexaVideoRecorderCapabilityAgent::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void AlexaVideoRecorderCapabilityAgent::executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void AlexaVideoRecorderCapabilityAgent::executeUnknownDirective(
    std::shared_ptr<DirectiveInfo> info,
    ExceptionErrorType type) {
    ACSDK_ERROR(LX("executeUnknownDirectiveFailed")
                    .d("reason", "unknownDirective")
                    .d("namespace", info->directive->getNamespace())
                    .d("name", info->directive->getName()));

    const std::string exceptionMessage =
        "unexpected directive " + info->directive->getNamespace() + ":" + info->directive->getName();

    sendExceptionEncounteredAndReportFailed(info, exceptionMessage, type);
}

void AlexaVideoRecorderCapabilityAgent::sendResponseEvent(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    const VideoRecorderInterface::Response& result) {
    switch (result.type) {
        case VideoRecorderInterface::Response::Type::SUCCESS: {
            std::string responsePayload = R"({"recordingStatus":")" + result.message + R"("})";
            m_responseSender->sendResponseEvent(
                info->directive->getInstance(),
                info->directive->getCorrelationToken(),
                AVSMessageEndpoint(m_endpointId),
                NAMESPACE,
                NAME_SEARCH_AND_RECORD_RESPONSE,
                responsePayload);
            break;
        }
        case VideoRecorderInterface::Response::Type::FAILED_TOO_MANY_FAILED_ATTEMPTS:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, result.message);
            break;
        case VideoRecorderInterface::Response::Type::FAILED_ACTION_NOT_PERMITTED_FOR_CONTENT:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::ACTION_NOT_PERMITTED_FOR_CONTENT,
                result.message);
            break;
        case VideoRecorderInterface::Response::Type::FAILED_CONFIRMATION_REQUIRED:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::CONFIRMATION_REQUIRED,
                result.message);
            break;
        case VideoRecorderInterface::Response::Type::FAILED_CONTENT_NOT_RECORDABLE:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::CONTENT_NOT_RECORDABLE,
                result.message);
            break;
        case VideoRecorderInterface::Response::Type::FAILED_NOT_SUBSCRIBED:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::NOT_SUBSCRIBED,
                result.message);
            break;
        case VideoRecorderInterface::Response::Type::FAILED_RECORDING_EXISTS:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::RECORDING_EXISTS,
                result.message);
            break;
        case VideoRecorderInterface::Response::Type::FAILED_STORAGE_FULL:
            sendAlexaVideoErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::STORAGE_FULL, result.message);
            break;
        case VideoRecorderInterface::Response::Type::FAILED_VIDEO_TITLE_DISAMBIGUATION_REQUIRED:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::TITLE_DISAMBIGUATION_REQUIRED,
                result.message);
            break;
        case VideoRecorderInterface::Response::Type::FAILED_RECORDING_SCHEDULE_CONFLICT:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::TUNER_OCCUPIED,
                result.message);
            break;
    }
}

void AlexaVideoRecorderCapabilityAgent::sendAlexaErrorResponse(
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

void AlexaVideoRecorderCapabilityAgent::sendAlexaVideoErrorResponse(
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

void AlexaVideoRecorderCapabilityAgent::provideState(
    const avsCommon::avs::CapabilityTag& stateProviderName,
    const avsCommon::sdkInterfaces::ContextRequestToken stateRequestToken) {
    m_executor.submit([this, stateProviderName, stateRequestToken] {
        do {
            if (stateProviderName.endpointId != m_endpointId) {
                ACSDK_ERROR(LX("provideStateFailed")
                                .d("reason", "notExpectedEndpointId")
                                .sensitive("endpointId", stateProviderName.endpointId));
                break;
            }

            std::string value;
            if (stateProviderName.name == EXTENDED_GUI_STATE_NAME) {
                value = m_videoRecorder->isExtendedRecordingGUIShown() ? "true" : "false";
            } else if (stateProviderName.name == STORAGE_LEVEL_STATE_NAME) {
                value = std::to_string(m_videoRecorder->getStorageUsedPercentage());
            } else {
                ACSDK_ERROR(
                    LX("ProvideStateExecutor").d("reason", "unknownState").sensitive("state", stateProviderName.name));
                break;
            }

            m_contextManager->provideStateResponse(stateProviderName, value, stateRequestToken);
        } while (0);
    });
}

/**
 * Templatized method to convert string to given template type.
 *
 * @param mapping string to enum type mapping.
 * @param in String to be converted.
 * @param out [out] Reference to enum resolved.
 *
 * @return true if string is resolvable.
 */
template <typename T>
static bool stringToEnum(const std::unordered_map<std::string, T>& mapping, const std::string& in, T& out) {
    auto it = mapping.find(in);
    if (it != mapping.end()) {
        out = it->second;
        return true;
    }

    return false;
}

std::unique_ptr<VideoRecorderRequest> AlexaVideoRecorderCapabilityAgent::parseDirectivePayload(
    const std::string& payload) {
    ACSDK_DEBUG9(LX(__func__));
    rapidjson::Document jsonPayload;
    if (jsonPayload.Parse(payload.c_str()).HasParseError()) {
        ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseJson"));
        return nullptr;
    }

    auto videoRecorderRequest = std::unique_ptr<VideoRecorderRequest>(new VideoRecorderRequest());
    const auto& entities = jsonPayload[ENTITIES];
    if (!entities.IsArray()) {
        ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseEntities"));
        return nullptr;
    }

    for (auto& entityJson : entities.GetArray()) {
        if (!entityJson.IsObject()) {
            continue;
        }

        if (!parseEntityJson(entityJson, *videoRecorderRequest)) {
            ACSDK_WARN(LX("parseEntityJson").d("reason", "unableToParseEntity"));
        }
    }

    if (jsonPayload.HasMember(QUANTIFIER) && jsonPayload[QUANTIFIER].IsObject()) {
        const auto& quantifierJson = jsonPayload[QUANTIFIER];

        if (quantifierJson.HasMember(NAME) && quantifierJson[NAME].IsString()) {
            VideoRecorderRequest::Quantifier quantifier;
            if (!stringToEnum(stringToQuantifierMap, quantifierJson[NAME].GetString(), quantifier)) {
                ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseMediaType"));
                return nullptr;
            }

            videoRecorderRequest->quantifier.set(quantifier);
        }
    }

    if (jsonPayload.HasMember(TIME_WINDOW) && jsonPayload[TIME_WINDOW].IsObject()) {
        TimeUtils timeUtils;
        const auto& timeWindowJson = jsonPayload[TIME_WINDOW];

        std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
        if (!timeUtils.convert8601TimeStringToUtcTimePoint(timeWindowJson[START].GetString(), &startTime)) {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseTimeWindow"));
            return nullptr;
        }
        if (!timeUtils.convert8601TimeStringToUtcTimePoint(timeWindowJson[END].GetString(), &endTime)) {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseTimeWindow"));
            return nullptr;
        }

        VideoRecorderRequest::TimeWindow timeWindow;
        timeWindow.startTime = startTime;
        timeWindow.endTime = endTime;
        videoRecorderRequest->timeWindow.set(timeWindow);
    }

    return videoRecorderRequest;
}

bool AlexaVideoRecorderCapabilityAgent::parseEntityJson(
    const rapidjson::Value& entityJson,
    VideoRecorderRequest& videoRecorderRequest) {
    ACSDK_DEBUG9(LX(__func__));
    bool retVal = false;

    do {
        VideoEntity videoEntity;
        VideoEntity::EntityType entityType;
        if (!videoEntity.parseVideoEntityType(entityJson, entityType)) {
            ACSDK_ERROR(LX(__func__).d("reason", "unableToParseEntityType"));
            break;
        }

        switch (entityType) {
            case VideoEntity::EntityType::MEDIA_TYPE: {
                VideoEntity::Media media(VideoEntity::Media::Type::MOVIE);
                if (!videoEntity.parseMediaEntity(entityJson, media)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseMedia"));
                    break;
                }

                videoRecorderRequest.mediaList.emplace_back(media);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::ACTOR: {
                VideoEntity::Actor actor;
                if (!videoEntity.parseActorEntity(entityJson, actor)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseActor"));
                    break;
                }

                videoRecorderRequest.actorList.emplace_back(actor);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::APP: {
                VideoEntity::App app;
                if (!videoEntity.parseAppEntity(entityJson, app)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseApp"));
                    break;
                }

                videoRecorderRequest.appList.emplace_back(app);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::CHANNEL: {
                VideoEntity::Channel channel(0);
                if (!videoEntity.parseChannelEntity(entityJson, channel)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseChannel"));
                    break;
                }

                videoRecorderRequest.channelList.emplace_back(channel);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::CHARACTER: {
                VideoEntity::Character character;
                if (!videoEntity.parseCharacterEntity(entityJson, character)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseCharacter"));
                    break;
                }

                videoRecorderRequest.characterList.emplace_back(character);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::DIRECTOR: {
                retVal = true;
                VideoEntity::Director director;
                if (!videoEntity.parseDirectorEntity(entityJson, director)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseDirector"));
                    break;
                }

                videoRecorderRequest.directorList.emplace_back(director);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::EPISODE: {
                VideoEntity::Episode episode;
                if (!videoEntity.parseEpisodeEntity(entityJson, episode)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseEpisode"));
                    break;
                }

                videoRecorderRequest.episodeList.emplace_back(episode);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::EVENT: {
                VideoEntity::Event event;
                if (!videoEntity.parseEventEntity(entityJson, event)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseEvent"));
                    break;
                }

                videoRecorderRequest.eventList.emplace_back(event);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::FRANCHISE: {
                VideoEntity::Franchise franchise;
                if (!videoEntity.parseFranchiseEntity(entityJson, franchise)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseFranchise"));
                    break;
                }

                videoRecorderRequest.franchiseList.emplace_back(franchise);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::GENRE: {
                VideoEntity::Genre genre;
                if (!videoEntity.parseGenreEntity(entityJson, genre)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseGenre"));
                    break;
                }

                videoRecorderRequest.genreList.emplace_back(genre);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::LEAGUE: {
                VideoEntity::League league;
                if (!videoEntity.parseLeagueEntity(entityJson, league)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseLeague"));
                    break;
                }

                videoRecorderRequest.leagueList.emplace_back(league);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::POPULARITY: {
                VideoEntity::Popularity popularity;
                if (!videoEntity.parsePopularityEntity(entityJson, popularity)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParsePopularity"));
                    break;
                }

                videoRecorderRequest.popularityList.emplace_back(popularity);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::PRODUCTION_COMPANY: {
                VideoEntity::ProductionCompany productionCompany;
                if (!videoEntity.parseProductionCompanyEntity(entityJson, productionCompany)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseProductionCompany"));
                    break;
                }

                videoRecorderRequest.productionCompanyList.emplace_back(productionCompany);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::RECENCY: {
                VideoEntity::Recency recency(VideoEntity::Recency::Type::NEW);
                if (!videoEntity.parseRecencyEntity(entityJson, recency)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseRecency"));
                    break;
                }

                videoRecorderRequest.recencyList.emplace_back(recency);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::SEASON: {
                VideoEntity::Season season;
                if (!videoEntity.parseSeasonEntity(entityJson, season)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseSeason"));
                    break;
                }

                videoRecorderRequest.seasonList.emplace_back(season);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::SPORT: {
                VideoEntity::Sport sport;
                if (!videoEntity.parseSportEntity(entityJson, sport)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseSport"));
                    break;
                }

                videoRecorderRequest.sportList.emplace_back(sport);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::SPORTS_TEAM: {
                VideoEntity::SportsTeam sportsTeam;
                if (!videoEntity.parseSportsTeamEntity(entityJson, sportsTeam)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseSportsTeam"));
                    break;
                }

                videoRecorderRequest.sportsTeamList.emplace_back(sportsTeam);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::VIDEO: {
                VideoEntity::Video video;
                if (!videoEntity.parseVideoEntity(entityJson, video)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseVideo"));
                    break;
                }

                videoRecorderRequest.videoList.emplace_back(video);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::VIDEO_RESOLUTION: {
                VideoEntity::VideoResolution videoResolution(VideoEntity::VideoResolution::Type::HD);
                if (!videoEntity.parseVideoResolutionEntity(entityJson, videoResolution)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseVideoResolution"));
                    break;
                }

                videoRecorderRequest.videoResolutionList.emplace_back(videoResolution);
                retVal = true;
                break;
            }
        }
    } while (0);

    return retVal;
}

}  // namespace acsdkAlexaVideoRecorder
}  // namespace alexaClientSDK
