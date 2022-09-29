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

#include "acsdk/AlexaRemoteVideoPlayer/private/AlexaRemoteVideoPlayerCapabilityAgent.h"

#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Timing/TimeUtils.h>

namespace alexaClientSDK {
namespace alexaRemoteVideoPlayer {

using namespace acsdkAlexaVideoCommon;
using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::configuration;
using namespace avsCommon::utils::timing;

using namespace alexaRemoteVideoPlayerInterfaces;

/// String to identify log entries originating from this file.
#define TAG "AlexaRemoteVideoPlayerCapabilityAgent"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const std::string NAMESPACE{"Alexa.RemoteVideoPlayer"};

/// The namespace for Alexa.Video.ErrorResponse.
static const std::string NAMESPACE_ALEXA_VIDEO_ERRORRESPONSE{"Alexa.Video"};

/// The supported version
static const std::string INTERFACE_VERSION{"3.1"};

/// The name for SearchAndPlay directive
static const std::string NAME_SEARCHANDPLAY{"SearchAndPlay"};

/// The name for SearchAndDisplayResults directive
static const std::string NAME_SEARCHANDDISPLAYRESULTS{"SearchAndDisplayResults"};

/// Name of the @c ConfigurationNode for RemoteVideoController.
static const std::string REMOTE_VIDEO_PLAYER_ROOT_KEY{"remoteVideoPlayer"};

/// The namespace for this capability agent's response event.
static const std::string NAMESPACE_PLAYBACK_STATE_REPORTER{"Alexa.PlaybackStateReporter"};

/// Property name for Alexa.PlaybackStateReporter
static const std::string PLAYBACK_STATE_REPORTER_PROPERTY{"playbackState"};

/// PLAYING State for Alexa.PlaybackStateReporter
static const std::string PLAYBACK_STATE_REPORTER_PLAYING_STATE{"PLAYING"};

/// STOPPED State for Alexa.PlaybackStateReporter
static const std::string PLAYBACK_STATE_REPORTER_STOPPED_STATE{"STOPPED"};

/// The configuration key
static const std::string CAPABILITY_CONFIGURATION_KEY{"configurations"};

/// Operations key
static const std::string CONFIGURATION_OPERATIONS_KEY{"operations"};

/// EntityTypes key
static const std::string CONFIGURATION_ENTITY_TYPES_KEY{"entityTypes"};

/// Catalogs key
static const std::string CONFIGURATION_CATALOGS_KEY{"catalogs"};

/// Catalogs Type key
static const std::string CONFIGURATION_CATALOGS_TYPE_KEY{"type"};

/// Catalogs SourceId key
static const std::string CONFIGURATION_CATALOGS_SOURCE_ID_KEY{"sourceId"};

/// Json key for parsing entities.
static const std::string ENTITIES("entities");

/// Json key for parsing search text.
static const std::string SEARCHTEXT("searchText");

/// Json key for parsing transcribed searchText.
static const std::string SEARCHTEXT_TRANSCRIBED("transcribed");

/// Json key for parsing timeWindow.
static const std::string TIME_WINDOW("timeWindow");

/// Json key for parsing start.
static const std::string START("start");

/// Json key for parsing end.
static const std::string END("end");

/// Helper function to convert a directive type to its corresponding string
static std::string convertOperationToString(const Configuration::SupportedOperations& directive) {
    switch (directive) {
        case Configuration::SupportedOperations::PLAY_VIDEO:
            return "SearchAndPlay";
        case Configuration::SupportedOperations::DISPLAY_SEARCH_RESULTS:
            return "SearchAndDisplayResults";
    }
    return "";
}

/// Helper function to convert a catalog type to its corresponding string
static std::string convertCatalogTypeToString(const Configuration::Catalog::Type& type) {
    switch (type) {
        case Configuration::Catalog::Type::PRIVATE_CATALOG:
            return "VIDEO_INGESTION_IDENTIFIER";
        case Configuration::Catalog::Type::PUBLIC_CATALOG:
            return "VIDEO_PUBLIC_CATALOG_IDENTIFIER";
    }
    return "";
}

std::shared_ptr<AlexaRemoteVideoPlayerCapabilityAgent> AlexaRemoteVideoPlayerCapabilityAgent::create(
    EndpointIdentifier endpointId,
    std::shared_ptr<RemoteVideoPlayerInterface> remoteVideoPlayer,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender) {
    if (endpointId.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyEndpointId"));
        return nullptr;
    }
    if (!remoteVideoPlayer) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullRemoteVideoPlayer"));
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

    auto remoteVideoPlayerCapabilityAgent =
        std::shared_ptr<AlexaRemoteVideoPlayerCapabilityAgent>(new AlexaRemoteVideoPlayerCapabilityAgent(
            endpointId,
            std::move(remoteVideoPlayer),
            std::move(contextManager),
            std::move(responseSender),
            std::move(exceptionSender)));

    if (!remoteVideoPlayerCapabilityAgent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "instantiationFailed"));
        return nullptr;
    }

    return remoteVideoPlayerCapabilityAgent;
}

AlexaRemoteVideoPlayerCapabilityAgent::AlexaRemoteVideoPlayerCapabilityAgent(
    EndpointIdentifier endpointId,
    std::shared_ptr<RemoteVideoPlayerInterface> remoteVideoPlayer,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"AlexaRemoteVideoPlayerCapabilityAgent"},
        m_endpointId{std::move(endpointId)},
        m_remoteVideoPlayer{std::move(remoteVideoPlayer)},
        m_contextManager{std::move(contextManager)},
        m_responseSender{std::move(responseSender)} {
}

void AlexaRemoteVideoPlayerCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG9(LX(__func__));
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void AlexaRemoteVideoPlayerCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX("preHandleDirective"));
    // do nothing.
}

void AlexaRemoteVideoPlayerCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    m_executor.submit([this, info] {
        ACSDK_DEBUG9(LX("handleDirectiveInExecutor"));
        const std::string directiveName = info->directive->getName();
        if (!info->directive->getEndpoint().hasValue() ||
            (info->directive->getEndpoint().value().endpointId != m_endpointId)) {
            executeUnknownDirective(info, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        auto payload = info->directive->getPayload();
        std::unique_ptr<RemoteVideoPlayerRequest> remoteVideoPlayerRequestContext = parseDirectivePayload(payload);
        if (!remoteVideoPlayerRequestContext) {
            ACSDK_ERROR(
                LX("handleDirectiveFailed").d("reason", "unableToParseDirectivePayload").sensitive("payload", payload));
            executeUnknownDirective(info, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        RemoteVideoPlayerInterface::Response result;
        if (directiveName == NAME_SEARCHANDPLAY) {
            result = m_remoteVideoPlayer->playVideo(std::move(remoteVideoPlayerRequestContext));
            executeSetHandlingCompleted(info);
            executeSendResponseEvent(info, result, PLAYBACK_STATE_REPORTER_PLAYING_STATE);
        } else if (directiveName == NAME_SEARCHANDDISPLAYRESULTS) {
            result = m_remoteVideoPlayer->displaySearchResults(std::move(remoteVideoPlayerRequestContext));
            executeSetHandlingCompleted(info);
            executeSendResponseEvent(info, result, PLAYBACK_STATE_REPORTER_STOPPED_STATE);
        } else {
            ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "unexpectedDirective").d("name", directiveName));
            executeUnknownDirective(info, ExceptionErrorType::UNSUPPORTED_OPERATION);
            return;
        }
    });
}

void AlexaRemoteVideoPlayerCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
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

DirectiveHandlerConfiguration AlexaRemoteVideoPlayerCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG9(LX(__func__));
    DirectiveHandlerConfiguration configuration;
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[{NAMESPACE, NAME_SEARCHANDPLAY, m_endpointId}] = neitherNonBlockingPolicy;
    configuration[{NAMESPACE, NAME_SEARCHANDDISPLAYRESULTS, m_endpointId}] = neitherNonBlockingPolicy;

    return configuration;
}

std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>>
AlexaRemoteVideoPlayerCapabilityAgent::getCapabilityConfigurations() {
    auto additionalConfigurations = CapabilityConfiguration::AdditionalConfigurations();
    auto config = m_remoteVideoPlayer->getConfiguration();
    auto configurationJson = buildRemoteVideoPlayerConfigurationJson(config);
    if (!configurationJson.empty()) {
        additionalConfigurations[CAPABILITY_CONFIGURATION_KEY] = configurationJson;
    }
    CapabilityConfiguration configuration{CapabilityConfiguration::ALEXA_INTERFACE_TYPE,
                                          NAMESPACE,
                                          INTERFACE_VERSION,
                                          Optional<std::string>(),  // instance
                                          Optional<CapabilityConfiguration::Properties>(),
                                          additionalConfigurations};
    return {std::make_shared<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>(configuration)};
}

void AlexaRemoteVideoPlayerCapabilityAgent::doShutdown() {
    ACSDK_DEBUG9(LX(__func__));
    if (!m_executor.isShutdown()) {
        m_executor.shutdown();
    }

    m_remoteVideoPlayer.reset();
    m_responseSender.reset();
    m_contextManager.reset();
}

void AlexaRemoteVideoPlayerCapabilityAgent::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void AlexaRemoteVideoPlayerCapabilityAgent::executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void AlexaRemoteVideoPlayerCapabilityAgent::executeUnknownDirective(
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

void AlexaRemoteVideoPlayerCapabilityAgent::executeSendResponseEvent(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    const RemoteVideoPlayerInterface::Response& result,
    const std::string& playbackState) {
    switch (result.type) {
        case RemoteVideoPlayerInterface::Response::Type::SUCCESS: {
            std::string responsePayload = R"({"state":")" + playbackState + R"("})";
            m_responseSender->sendResponseEvent(
                info->directive->getInstance(),
                info->directive->getCorrelationToken(),
                AVSMessageEndpoint(m_endpointId),
                NAMESPACE_PLAYBACK_STATE_REPORTER,
                PLAYBACK_STATE_REPORTER_PROPERTY,
                responsePayload);
            break;
        }
        case RemoteVideoPlayerInterface::Response::Type::FAILED_TOO_MANY_FAILED_ATTEMPTS:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, result.errorMessage);
            break;
        case RemoteVideoPlayerInterface::Response::Type::FAILED_ENDPOINT_UNREACHABLE:
            sendAlexaErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::ErrorResponseType::ENDPOINT_UNREACHABLE,
                result.errorMessage);
            break;
        case RemoteVideoPlayerInterface::Response::Type::FAILED_NOT_SUBSCRIBED:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::NOT_SUBSCRIBED,
                result.errorMessage);
            break;
        case RemoteVideoPlayerInterface::Response::Type::FAILED_INTERNAL_ERROR:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, result.errorMessage);
            break;
    }
}

void AlexaRemoteVideoPlayerCapabilityAgent::sendAlexaErrorResponse(
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

void AlexaRemoteVideoPlayerCapabilityAgent::sendAlexaVideoErrorResponse(
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

/**
 * Helper function to convert the supported directives to strings and add it to the Configuration json.
 *
 * @param @c operations The set of supported directives.
 * @param @c jsonGenerator JsonGenerator object used to build the json string
 */
static void addOperationsArrayToConfiguration(
    const std::set<Configuration::SupportedOperations>& operations,
    alexaClientSDK::avsCommon::utils::json::JsonGenerator* jsonGenerator) {
    std::set<std::string> operationsArray;
    for (const auto& operation : operations) {
        operationsArray.insert(convertOperationToString(operation));
    }
    jsonGenerator->addStringArray(CONFIGURATION_OPERATIONS_KEY, operationsArray);
}

/**
 * Helper function to convert the supported entity types to strings and add it to the Configuration json.
 *
 * @param @c entityTypes The set of supported entity types.
 * @param @c jsonGenerator JsonGenerator object used to build the json string
 */
static void addEntityTypesArrayToConfiguration(
    const std::set<acsdkAlexaVideoCommon::VideoEntity::EntityType>& entityTypes,
    alexaClientSDK::avsCommon::utils::json::JsonGenerator* jsonGenerator) {
    std::set<std::string> entityTypesArray;
    for (const auto& entityType : entityTypes) {
        entityTypesArray.insert(VideoEntity::convertEntityTypeToString(entityType));
    }
    jsonGenerator->addStringArray(CONFIGURATION_ENTITY_TYPES_KEY, entityTypesArray);
}

/**
 * Helper function to convert the supported catalog types to strings and add it to the Configuration json.
 *
 * @param @c catalogs The set of supported catalogs.
 * @param @c jsonGenerator JsonGenerator object used to build the json string
 */
static void addCatalogsArraytoConfiguration(
    const std::list<Configuration::Catalog>& catalogs,
    alexaClientSDK::avsCommon::utils::json::JsonGenerator* jsonGenerator) {
    jsonGenerator->startArray(CONFIGURATION_CATALOGS_KEY);
    for (const auto& catalog : catalogs) {
        jsonGenerator->startArrayElement();
        jsonGenerator->addMember(CONFIGURATION_CATALOGS_TYPE_KEY, convertCatalogTypeToString(catalog.type));
        jsonGenerator->addMember(CONFIGURATION_CATALOGS_SOURCE_ID_KEY, catalog.sourceId);
        jsonGenerator->finishArrayElement();
    }
    jsonGenerator->finishArray();
}

std::string AlexaRemoteVideoPlayerCapabilityAgent::buildRemoteVideoPlayerConfigurationJson(
    const Configuration& configuration) {
    alexaClientSDK::avsCommon::utils::json::JsonGenerator jsonGenerator;

    if (configuration.catalogs.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "EmptyCatalog"));
        return "";
    }
    addCatalogsArraytoConfiguration(configuration.catalogs, &jsonGenerator);

    if (!configuration.operations.hasValue()) {
        ACSDK_DEBUG9(LX(__func__).m("EmptyOperationsArray"));
    } else {
        addOperationsArrayToConfiguration(configuration.operations.value(), &jsonGenerator);
    }

    if (!configuration.entityTypes.hasValue()) {
        ACSDK_DEBUG9(LX(__func__).m("EmptyEntityTypesArray"));
    } else {
        addEntityTypesArrayToConfiguration(configuration.entityTypes.value(), &jsonGenerator);
    }

    ACSDK_DEBUG9(LX(__func__).sensitive("configuration", jsonGenerator.toString()));
    return jsonGenerator.toString();
}

std::unique_ptr<RemoteVideoPlayerRequest> AlexaRemoteVideoPlayerCapabilityAgent::parseDirectivePayload(
    const std::string& payload) {
    ACSDK_DEBUG9(LX(__func__));
    rapidjson::Document jsonPayload;
    if (jsonPayload.Parse(payload.c_str()).HasParseError()) {
        ACSDK_ERROR(LX("parseDirectivePayload").d("reason", "unableToParseJson"));
        return nullptr;
    }

    auto remoteVideoPlayerRequest = std::unique_ptr<RemoteVideoPlayerRequest>(new RemoteVideoPlayerRequest());
    const auto& entities = jsonPayload[ENTITIES];
    if (!entities.IsArray()) {
        ACSDK_ERROR(LX("parseDirectivePayload").d("reason", "unableToParseEntities"));
        return nullptr;
    }

    for (auto& entityJson : entities.GetArray()) {
        if (!entityJson.IsObject()) {
            continue;
        }

        if (!parseEntityJson(entityJson, *remoteVideoPlayerRequest)) {
            ACSDK_ERROR(LX("parseDirectivePayload").d("reason", "unableToParseEntity"));
        }
    }

    if (jsonPayload.HasMember(SEARCHTEXT) && jsonPayload[SEARCHTEXT].IsObject()) {
        const auto& searchTextJson = jsonPayload[SEARCHTEXT];
        if (searchTextJson.HasMember(SEARCHTEXT_TRANSCRIBED) && searchTextJson[SEARCHTEXT_TRANSCRIBED].IsString()) {
            remoteVideoPlayerRequest->searchTextTranscribed.set(searchTextJson[SEARCHTEXT_TRANSCRIBED].GetString());
        }
    }

    if (jsonPayload.HasMember(TIME_WINDOW) && jsonPayload[TIME_WINDOW].IsObject()) {
        TimeUtils timeUtils;
        const auto& timeWindowJson = jsonPayload[TIME_WINDOW];

        std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
        if (!timeUtils.convert8601TimeStringToUtcTimePoint(timeWindowJson[START].GetString(), &startTime)) {
            ACSDK_ERROR(LX("parseDirectivePayload").d("reason", "unableToParseTimeWindow"));
            return nullptr;
        }
        if (!timeUtils.convert8601TimeStringToUtcTimePoint(timeWindowJson[END].GetString(), &endTime)) {
            ACSDK_ERROR(LX("parseDirectivePayload").d("reason", "unableToParseTimeWindow"));
            return nullptr;
        }

        RemoteVideoPlayerRequest::TimeWindow timeWindow;
        timeWindow.startTime = startTime;
        timeWindow.endTime = endTime;
        remoteVideoPlayerRequest->timeWindow.set(timeWindow);
    }

    return remoteVideoPlayerRequest;
}

bool AlexaRemoteVideoPlayerCapabilityAgent::parseEntityJson(
    const rapidjson::Value& entityJson,
    RemoteVideoPlayerRequest& remoteVideoPlayerRequest) {
    ACSDK_DEBUG9(LX(__func__));
    auto retVal = false;

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

                remoteVideoPlayerRequest.mediaList.emplace_back(media);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::ACTOR: {
                VideoEntity::Actor actor;
                if (!videoEntity.parseActorEntity(entityJson, actor)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseActor"));
                    break;
                }

                remoteVideoPlayerRequest.actorList.emplace_back(actor);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::APP: {
                VideoEntity::App app;
                if (!videoEntity.parseAppEntity(entityJson, app)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseApp"));
                    break;
                }

                remoteVideoPlayerRequest.appList.emplace_back(app);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::CHANNEL: {
                VideoEntity::Channel channel(0);
                if (!videoEntity.parseChannelEntity(entityJson, channel)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseChannel"));
                    break;
                }

                remoteVideoPlayerRequest.channelList.emplace_back(channel);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::CHARACTER: {
                VideoEntity::Character character;
                if (!videoEntity.parseCharacterEntity(entityJson, character)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseCharacter"));
                    break;
                }

                remoteVideoPlayerRequest.characterList.emplace_back(character);
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

                remoteVideoPlayerRequest.directorList.emplace_back(director);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::EPISODE: {
                VideoEntity::Episode episode;
                if (!videoEntity.parseEpisodeEntity(entityJson, episode)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseEpisode"));
                    break;
                }

                remoteVideoPlayerRequest.episodeList.emplace_back(episode);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::EVENT: {
                VideoEntity::Event event;
                if (!videoEntity.parseEventEntity(entityJson, event)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseEvent"));
                    break;
                }

                remoteVideoPlayerRequest.eventList.emplace_back(event);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::FRANCHISE: {
                VideoEntity::Franchise franchise;
                if (!videoEntity.parseFranchiseEntity(entityJson, franchise)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseFranchise"));
                    break;
                }

                remoteVideoPlayerRequest.franchiseList.emplace_back(franchise);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::GENRE: {
                VideoEntity::Genre genre;
                if (!videoEntity.parseGenreEntity(entityJson, genre)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseGenre"));
                    break;
                }

                remoteVideoPlayerRequest.genreList.emplace_back(genre);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::LEAGUE: {
                VideoEntity::League league;
                if (!videoEntity.parseLeagueEntity(entityJson, league)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseLeague"));
                    break;
                }

                remoteVideoPlayerRequest.leagueList.emplace_back(league);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::POPULARITY: {
                VideoEntity::Popularity popularity;
                if (!videoEntity.parsePopularityEntity(entityJson, popularity)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParsePopularity"));
                    break;
                }

                remoteVideoPlayerRequest.popularityList.emplace_back(popularity);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::PRODUCTION_COMPANY: {
                VideoEntity::ProductionCompany productionCompany;
                if (!videoEntity.parseProductionCompanyEntity(entityJson, productionCompany)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseProductionCompany"));
                    break;
                }

                remoteVideoPlayerRequest.productionCompanyList.emplace_back(productionCompany);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::RECENCY: {
                VideoEntity::Recency recency(VideoEntity::Recency::Type::NEW);
                if (!videoEntity.parseRecencyEntity(entityJson, recency)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseRecency"));
                    break;
                }

                remoteVideoPlayerRequest.recencyList.emplace_back(recency);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::SEASON: {
                VideoEntity::Season season;
                if (!videoEntity.parseSeasonEntity(entityJson, season)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseSeason"));
                    break;
                }

                remoteVideoPlayerRequest.seasonList.emplace_back(season);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::SPORT: {
                VideoEntity::Sport sport;
                if (!videoEntity.parseSportEntity(entityJson, sport)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseSport"));
                    break;
                }

                remoteVideoPlayerRequest.sportList.emplace_back(sport);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::SPORTS_TEAM: {
                VideoEntity::SportsTeam sportsTeam;
                if (!videoEntity.parseSportsTeamEntity(entityJson, sportsTeam)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseSportsTeam"));
                    break;
                }

                remoteVideoPlayerRequest.sportsTeamList.emplace_back(sportsTeam);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::VIDEO: {
                VideoEntity::Video video;
                if (!videoEntity.parseVideoEntity(entityJson, video)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseVideo"));
                    break;
                }

                remoteVideoPlayerRequest.videoList.emplace_back(video);
                retVal = true;
                break;
            }
            case VideoEntity::EntityType::VIDEO_RESOLUTION: {
                VideoEntity::VideoResolution videoResolution(VideoEntity::VideoResolution::Type::HD);
                if (!videoEntity.parseVideoResolutionEntity(entityJson, videoResolution)) {
                    ACSDK_ERROR(LX(__func__).d("reason", "unableToParseVideoResolution"));
                    break;
                }

                remoteVideoPlayerRequest.videoResolutionList.emplace_back(videoResolution);
                retVal = true;
                break;
            }
        }
    } while (0);

    return retVal;
}

}  // namespace alexaRemoteVideoPlayer
}  // namespace alexaClientSDK