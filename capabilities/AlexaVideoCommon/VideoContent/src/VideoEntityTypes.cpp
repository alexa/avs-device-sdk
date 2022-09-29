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

#include "acsdk/VideoContent/VideoEntityTypes.h"

#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace acsdkAlexaVideoCommon {

using namespace avsCommon;
using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
#define TAG "VideoEntity"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// Json key for parsing type.
static const auto TYPE = "type";

/// Json key for parsing value.
static const auto VALUE = "value";

/// Json key for parsing externalIds.
static const auto EXTERNAL_IDS = "externalIds";

/// Json key for parsing entityMetadata.
static const auto ENTITY_METADATA = "entityMetadata";

/// Json key for parsing channelNumber.
static const auto CHANNEL_NUMBER = "channelNumber";

/// Json key for parsing channelCallSign.
static const auto CHANNEL_CALL_SIGN = "channelCallSign";

/// Json key for parsing uri.
static const auto URI = "uri";

/// A map to convert string from json to MediaType enum
static const std::unordered_map<std::string, VideoEntity::Media::Type> stringToMediaTypeMap = {
    {"MOVIE", VideoEntity::Media::Type::MOVIE},
    {"VIDEO", VideoEntity::Media::Type::VIDEO},
    {"TV_SHOW", VideoEntity::Media::Type::TV_SHOW}};

/// A map to convert string from json to RecencyType enum
static const std::unordered_map<std::string, VideoEntity::Recency::Type> stringToRecencyTypeMap = {
    {"NEW", VideoEntity::Recency::Type::NEW},
    {"OLD", VideoEntity::Recency::Type::OLD}};

/// A map to convert string from json to Resolution enum
static const std::unordered_map<std::string, VideoEntity::VideoResolution::Type> stringToResolutionMap = {
    {"SD", VideoEntity::VideoResolution::Type::SD},
    {"HD", VideoEntity::VideoResolution::Type::HD},
    {"4k", VideoEntity::VideoResolution::Type::FOUR_K},
    {"8k", VideoEntity::VideoResolution::Type::EIGHT_K}};

/// A map to convert string from json to EntityType enum
static const std::unordered_map<std::string, VideoEntity::EntityType> stringToEntityTypeMap = {
    {"Actor", VideoEntity::EntityType::ACTOR},
    {"App", VideoEntity::EntityType::APP},
    {"Channel", VideoEntity::EntityType::CHANNEL},
    {"Character", VideoEntity::EntityType::CHARACTER},
    {"Director", VideoEntity::EntityType::DIRECTOR},
    {"Episode", VideoEntity::EntityType::EPISODE},
    {"Event", VideoEntity::EntityType::EVENT},
    {"Franchise", VideoEntity::EntityType::FRANCHISE},
    {"Genre", VideoEntity::EntityType::GENRE},
    {"League", VideoEntity::EntityType::LEAGUE},
    {"MediaType", VideoEntity::EntityType::MEDIA_TYPE},
    {"Popularity", VideoEntity::EntityType::POPULARITY},
    {"ProductionCompany", VideoEntity::EntityType::PRODUCTION_COMPANY},
    {"Recency", VideoEntity::EntityType::RECENCY},
    {"Season", VideoEntity::EntityType::SEASON},
    {"Sport", VideoEntity::EntityType::SPORT},
    {"SportsTeam", VideoEntity::EntityType::SPORTS_TEAM},
    {"Video", VideoEntity::EntityType::VIDEO},
    {"VideoResolution", VideoEntity::EntityType::VIDEO_RESOLUTION}};

/**
 * Templatized method to convert string to given template type.
 * @tparam T type of Enum that needs a conversion.
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

bool VideoEntity::parseVideoEntityType(const rapidjson::Value& entityJson, VideoEntity::EntityType& entityType) {
    auto retVal = false;
    do {
        if (!stringToEnum(stringToEntityTypeMap, entityJson[TYPE].GetString(), entityType)) {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseVideoEntityType"));
            break;
        }
        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseActorEntity(const rapidjson::Value& entityJson, VideoEntity::Actor& actor) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(EXTERNAL_IDS) && entityJson[EXTERNAL_IDS].IsObject()) {
            const auto& externalIds = entityJson[EXTERNAL_IDS];
            for (rapidjson::Value::ConstMemberIterator it = externalIds.MemberBegin(); it != externalIds.MemberEnd();
                 ++it) {
                if (it->name.IsString() && it->value.IsString()) {
                    actor.externalIds.emplace(it->name.GetString(), it->value.GetString());
                }
            }
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseExternalId"));
            break;
        }

        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            actor.name = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseValue"));
            break;
        }
        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseAppEntity(const rapidjson::Value& entityJson, VideoEntity::App& app) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(EXTERNAL_IDS) && entityJson[EXTERNAL_IDS].IsObject()) {
            const auto& externalIds = entityJson[EXTERNAL_IDS];
            for (rapidjson::Value::ConstMemberIterator it = externalIds.MemberBegin(); it != externalIds.MemberEnd();
                 ++it) {
                if (it->name.IsString() && it->value.IsString()) {
                    app.externalIds.emplace(it->name.GetString(), it->value.GetString());
                }
            }
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseExternalId"));
            break;
        }

        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            app.name = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseChannelEntity(const rapidjson::Value& entityJson, VideoEntity::Channel& channel) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(EXTERNAL_IDS) && entityJson[EXTERNAL_IDS].IsObject()) {
            const auto& externalIds = entityJson[EXTERNAL_IDS];
            for (auto it = externalIds.MemberBegin(); it != externalIds.MemberEnd(); ++it) {
                if (it->name.IsString() && it->value.IsString()) {
                    channel.externalIds.emplace(it->name.GetString(), it->value.GetString());
                }
            }
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseExternalId"));
            break;
        }

        if (entityJson.HasMember(ENTITY_METADATA) && entityJson[ENTITY_METADATA].IsObject()) {
            const auto& entityMetaData = entityJson[ENTITY_METADATA];

            if (entityMetaData.HasMember(CHANNEL_NUMBER) && entityMetaData[CHANNEL_NUMBER].IsNumber()) {
                channel.channelNumber = entityMetaData[CHANNEL_NUMBER].GetInt();
            }

            if (entityMetaData.HasMember(CHANNEL_CALL_SIGN) && entityMetaData[CHANNEL_CALL_SIGN].IsString()) {
                channel.channelCallSign = entityMetaData[CHANNEL_CALL_SIGN].GetString();
            }
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseMetadata"));
            break;
        }

        if (entityJson.HasMember(URI) && entityJson[URI].IsString()) {
            channel.uri = entityJson[URI].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseURI"));
            break;
        }

        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            channel.name = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseCharacterEntity(const rapidjson::Value& entityJson, VideoEntity::Character& character) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(EXTERNAL_IDS) && entityJson[EXTERNAL_IDS].IsObject()) {
            const auto& externalIds = entityJson[EXTERNAL_IDS];
            for (auto it = externalIds.MemberBegin(); it != externalIds.MemberEnd(); ++it) {
                if (it->name.IsString() && it->value.IsString()) {
                    character.externalIds.emplace(it->name.GetString(), it->value.GetString());
                }
            }
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseExternalIds"));
            break;
        }

        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            character.name = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseDirectorEntity(const rapidjson::Value& entityJson, VideoEntity::Director& director) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(EXTERNAL_IDS) && entityJson[EXTERNAL_IDS].IsObject()) {
            const auto& externalIds = entityJson[EXTERNAL_IDS];
            for (auto it = externalIds.MemberBegin(); it != externalIds.MemberEnd(); ++it) {
                if (it->name.IsString() && it->value.IsString()) {
                    director.externalIds.emplace(it->name.GetString(), it->value.GetString());
                }
            }
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseExternalIds"));
            break;
        }

        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            director.name = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseEpisodeEntity(const rapidjson::Value& entityJson, VideoEntity::Episode& episode) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            episode.number = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseEventEntity(const rapidjson::Value& entityJson, VideoEntity::Event& event) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(EXTERNAL_IDS) && entityJson[EXTERNAL_IDS].IsObject()) {
            const auto& externalIds = entityJson[EXTERNAL_IDS];
            for (rapidjson::Value::ConstMemberIterator it = externalIds.MemberBegin(); it != externalIds.MemberEnd();
                 ++it) {
                if (it->name.IsString() && it->value.IsString()) {
                    event.externalIds.emplace(it->name.GetString(), it->value.GetString());
                }
            }
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseExternalIds"));
            break;
        }

        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            event.name = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseFranchiseEntity(const rapidjson::Value& entityJson, VideoEntity::Franchise& franchise) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            franchise.name = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseGenreEntity(const rapidjson::Value& entityJson, VideoEntity::Genre& genre) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(EXTERNAL_IDS) && entityJson[EXTERNAL_IDS].IsObject()) {
            const auto& externalIds = entityJson[EXTERNAL_IDS];
            for (rapidjson::Value::ConstMemberIterator it = externalIds.MemberBegin(); it != externalIds.MemberEnd();
                 ++it) {
                if (it->name.IsString() && it->value.IsString()) {
                    genre.externalIds.emplace(it->name.GetString(), it->value.GetString());
                }
            }
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseExternalIds"));
            break;
        }

        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            genre.name = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseLeagueEntity(const rapidjson::Value& entityJson, VideoEntity::League& league) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(EXTERNAL_IDS) && entityJson[EXTERNAL_IDS].IsObject()) {
            const auto& externalIds = entityJson[EXTERNAL_IDS];
            for (rapidjson::Value::ConstMemberIterator it = externalIds.MemberBegin(); it != externalIds.MemberEnd();
                 ++it) {
                if (it->name.IsString() && it->value.IsString()) {
                    league.externalIds.emplace(it->name.GetString(), it->value.GetString());
                }
            }
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseExternalIds"));
            break;
        }

        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            league.name = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseMediaEntity(const rapidjson::Value& entityJson, VideoEntity::Media& media) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            if (!stringToEnum(stringToMediaTypeMap, entityJson[VALUE].GetString(), media.type)) {
                ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseMediaType"));
                break;
            }
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parsePopularityEntity(const rapidjson::Value& entityJson, VideoEntity::Popularity& popularity) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            popularity.value = std::string(entityJson[VALUE].GetString()) == "TRUE" ? true : false;
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParsePopularityValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseProductionCompanyEntity(
    const rapidjson::Value& entityJson,
    VideoEntity::ProductionCompany& productionCompany) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            productionCompany.name = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseRecencyEntity(const rapidjson::Value& entityJson, VideoEntity::Recency& recency) {
    bool retVal = false;
    do {
        if (!stringToEnum(stringToRecencyTypeMap, entityJson[VALUE].GetString(), recency.type)) {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseRecencyType"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseSeasonEntity(const rapidjson::Value& entityJson, VideoEntity::Season& season) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            season.number = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseSportEntity(const rapidjson::Value& entityJson, VideoEntity::Sport& sport) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(EXTERNAL_IDS) && entityJson[EXTERNAL_IDS].IsObject()) {
            const auto& externalIds = entityJson[EXTERNAL_IDS];
            for (rapidjson::Value::ConstMemberIterator it = externalIds.MemberBegin(); it != externalIds.MemberEnd();
                 ++it) {
                if (it->name.IsString() && it->value.IsString()) {
                    sport.externalIds.emplace(it->name.GetString(), it->value.GetString());
                }
            }
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseExternalIds"));
            break;
        }

        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            sport.name = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseSportsTeamEntity(const rapidjson::Value& entityJson, VideoEntity::SportsTeam& sportsTeam) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(EXTERNAL_IDS) && entityJson[EXTERNAL_IDS].IsObject()) {
            const auto& externalIds = entityJson[EXTERNAL_IDS];
            for (rapidjson::Value::ConstMemberIterator it = externalIds.MemberBegin(); it != externalIds.MemberEnd();
                 ++it) {
                if (it->name.IsString() && it->value.IsString()) {
                    sportsTeam.externalIds.emplace(it->name.GetString(), it->value.GetString());
                }
            }
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseExternalIds"));
            break;
        }

        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            sportsTeam.name = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseVideoEntity(const rapidjson::Value& entityJson, VideoEntity::Video& video) {
    bool retVal = false;
    do {
        if (entityJson.HasMember(EXTERNAL_IDS) && entityJson[EXTERNAL_IDS].IsObject()) {
            const auto& externalIds = entityJson[EXTERNAL_IDS];
            for (rapidjson::Value::ConstMemberIterator it = externalIds.MemberBegin(); it != externalIds.MemberEnd();
                 ++it) {
                if (it->name.IsString() && it->value.IsString()) {
                    video.externalIds.emplace(it->name.GetString(), it->value.GetString());
                }
            }
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseExternalIds"));
            break;
        }

        if (entityJson.HasMember(VALUE) && entityJson[VALUE].IsString()) {
            video.name = entityJson[VALUE].GetString();
        } else {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseValue"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

bool VideoEntity::parseVideoResolutionEntity(
    const rapidjson::Value& entityJson,
    VideoEntity::VideoResolution& videoResolution) {
    bool retVal = false;
    do {
        if (!stringToEnum(stringToResolutionMap, entityJson[VALUE].GetString(), videoResolution.type)) {
            ACSDK_ERROR(LX("parseEntityJson").d("reason", "unableToParseVideoResolutionType"));
            break;
        }

        retVal = true;
    } while (0);

    return retVal;
}

std::string VideoEntity::convertEntityTypeToString(const VideoEntity::EntityType& entityType) {
    switch (entityType) {
        case VideoEntity::EntityType::ACTOR:
            return "Actor";
        case VideoEntity::EntityType::APP:
            return "App";
        case VideoEntity::EntityType::CHANNEL:
            return "Channel";
        case VideoEntity::EntityType::CHARACTER:
            return "Character";
        case VideoEntity::EntityType::DIRECTOR:
            return "Director";
        case VideoEntity::EntityType::EPISODE:
            return "Episode";
        case VideoEntity::EntityType::EVENT:
            return "Event";
        case VideoEntity::EntityType::FRANCHISE:
            return "Franchise";
        case VideoEntity::EntityType::GENRE:
            return "Genre";
        case VideoEntity::EntityType::LEAGUE:
            return "League";
        case VideoEntity::EntityType::MEDIA_TYPE:
            return "MediaType";
        case VideoEntity::EntityType::POPULARITY:
            return "Popularity";
        case VideoEntity::EntityType::PRODUCTION_COMPANY:
            return "ProductionCompany";
        case VideoEntity::EntityType::RECENCY:
            return "Recency";
        case VideoEntity::EntityType::SEASON:
            return "Season";
        case VideoEntity::EntityType::SPORT:
            return "Sport";
        case VideoEntity::EntityType::SPORTS_TEAM:
            return "SportsTeam";
        case VideoEntity::EntityType::VIDEO:
            return "Video";
        case VideoEntity::EntityType::VIDEO_RESOLUTION:
            return "VideoResolution";
    }
    return "";
}

}  // namespace acsdkAlexaVideoCommon
}  // namespace alexaClientSDK
