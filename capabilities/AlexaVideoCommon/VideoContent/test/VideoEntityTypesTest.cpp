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

/// @file VideoEntityTest.cpp

#include "acsdk/VideoContent/VideoEntityTypes.h"

#include <gtest/gtest.h>

namespace alexaClientSDK {
namespace acsdkAlexaVideoCommon {
namespace test {
using namespace alexaClientSDK;
using namespace ::testing;

// Sample Entity payloads
static const std::string ACTOR_ENTITY = R"({
        "externalIds": {
            "gracenote": "P000000099001"
        },
        "type": "Actor",
        "value": "Actor Name"
})";
static const std::string APP_ENTITY = R"({
        "externalIds": {
            "ENTITY_ID": "amzn1.alexa-ask-target.app.72095"
        },
        "type": "App",
        "value": "Prime Video"
})";
static const std::string CHANNEL_ENTITY = R"({
        "externalIds": {
            "gracenote": "TV000000000001"
        },
        "entityMetadata": {
            "channelNumber": 15,
            "channelCallSign": "KBTC"
        },
        "type": "Channel",
        "value": "PBS",
        "uri": "entity://provider/channel/15"
})";
static const std::string CHARACTER_ENTITY = R"({
        "externalIds": {
            "gracenote": "ST0000000123456",
            "ASIN": "B00DT55P3K"
        },
        "type": "Character",
        "value": "Snow White"
})";
static const std::string DIRECTOR_ENTITY = R"({
        "externalIds": {
            "gracenote": "gracenote ID"
        },
        "type": "Director",
        "value": "Kenneth Lonergan"
})";
static const std::string EPISODE_ENTITY = R"({
    "value": "8",
    "type": "Episode"
})";
static const std::string EVENT_ENTITY = R"({
    "externalIds": {
        "gracenote": "SP0000000666661"
    },
    "type": "Event",
    "value": "Games"
})";
static const std::string FRANCHISE_ENTITY = R"({
    "type": "Franchise",
    "value": "Intergalactic Wars"
})";
static const std::string GENRE_ENTITY = R"({
    "externalIds": {
        "gracenote": "gracenote ID"
    },
    "type": "Genre",
    "value": "Action"
})";
static const std::string LEAGUE_ENTITY = R"({
    "externalIds": {
        "gracenote": "gracenote ID"
    },
    "type": "League",
    "value": "NCAA"
})";
static const std::string MEDIA_ENTITY = R"({
    "type": "MediaType",
    "value": "MOVIE"
})";
static const std::string POPULARITY_ENTITY = R"({
    "type": "Popularity",
    "value": "TRUE"
})";
static const std::string PRODUCTION_COMPANY_ENTITY = R"({
    "type": "ProductionCompany",
    "value": "Marvel"
})";
static const std::string RECENCY_ENTITY = R"({
    "type": "Recency",
    "value": "NEW"
})";
static const std::string SEASON_ENTITY = R"({
    "type": "Season",
    "value": "2"
})";
static const std::string SPORT_ENTITY = R"({
    "externalIds": {
        "gracenote": "gracenote ID"
    },
    "type": "Sport",
    "value": "Football"
})";
static const std::string SPORTS_TEAM_ENTITY = R"({
    "externalIds": {
        "gracenote": "gracenote ID"
    },
    "type": "SportsTeam",
    "value": "University of Washington Huskies"
})";
static const std::string VIDEO_ENTITY = R"({
    "externalIds": {
        "gracenote": "MV0000000666661",
        "asin": "B01M3X9T06"
    },
    "type": "Video",
    "value": "Manchester by the Sea"
})";
static const std::string VIDEO_RESOLUTION_ENTITY = R"({
    "type": "VideoResolution",
    "value": "HD"
})";

class VideoEntityTest : public ::testing::Test {
protected:
    /// An instance of the @c VideoEntity that will be used to test units.
    VideoEntity m_videoEntity;
};

/**
 * Test successful parsing of Actor entity payload.
 */
TEST_F(VideoEntityTest, test_parseActorEntity) {
    VideoEntity::Actor actor;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(ACTOR_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseActorEntity(jsonPayload, actor));

    ASSERT_FALSE(ACTOR_ENTITY.find(actor.name) == ACTOR_ENTITY.npos);
}

/**
 * Test successful parsing of App entity payload.
 */
TEST_F(VideoEntityTest, test_parseAppEntity) {
    VideoEntity::App app;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(APP_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseAppEntity(jsonPayload, app));

    ASSERT_FALSE(APP_ENTITY.find(app.name) == APP_ENTITY.npos);
}

/**
 * Test successful parsing of Channel entity payload.
 */
TEST_F(VideoEntityTest, test_parseChannelEntity) {
    VideoEntity::Channel channel(0);
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(CHANNEL_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseChannelEntity(jsonPayload, channel));

    ASSERT_FALSE(CHANNEL_ENTITY.find(channel.name) == CHANNEL_ENTITY.npos);
}

/**
 * Test successful parsing of Character entity payload.
 */
TEST_F(VideoEntityTest, test_parseCharacterEntity) {
    VideoEntity::Character character;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(CHARACTER_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseCharacterEntity(jsonPayload, character));

    ASSERT_FALSE(CHARACTER_ENTITY.find(character.name) == CHARACTER_ENTITY.npos);
}

/**
 * Test successful parsing of Director entity payload.
 */
TEST_F(VideoEntityTest, test_parseDirectorEntity) {
    VideoEntity::Director director;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(DIRECTOR_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseDirectorEntity(jsonPayload, director));

    ASSERT_FALSE(DIRECTOR_ENTITY.find(director.name) == DIRECTOR_ENTITY.npos);
}

/**
 * Test successful parsing of Event entity payload.
 */
TEST_F(VideoEntityTest, test_parseEventEntity) {
    VideoEntity::Event event;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(EVENT_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseEventEntity(jsonPayload, event));

    ASSERT_FALSE(EVENT_ENTITY.find(event.name) == EVENT_ENTITY.npos);
}

/**
 * Test successful parsing of Episode entity payload.
 */
TEST_F(VideoEntityTest, test_parseEpisodeEntity) {
    VideoEntity::Episode episode;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(EPISODE_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseEpisodeEntity(jsonPayload, episode));

    ASSERT_TRUE(episode.number == "8");
}

/**
 * Test successful parsing of Franchise entity payload.
 */
TEST_F(VideoEntityTest, test_parseFranchiseEntity) {
    VideoEntity::Franchise franchise;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(FRANCHISE_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseFranchiseEntity(jsonPayload, franchise));

    ASSERT_FALSE(FRANCHISE_ENTITY.find(franchise.name) == FRANCHISE_ENTITY.npos);
}

/**
 * Test successful parsing of Genre entity payload.
 */
TEST_F(VideoEntityTest, test_parseGenreEntity) {
    VideoEntity::Genre genre;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(GENRE_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseGenreEntity(jsonPayload, genre));

    ASSERT_FALSE(GENRE_ENTITY.find(genre.name) == GENRE_ENTITY.npos);
}

/**
 * Test successful parsing of League entity payload.
 */
TEST_F(VideoEntityTest, test_parseLeagueEntity) {
    VideoEntity::League league;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(LEAGUE_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseLeagueEntity(jsonPayload, league));

    ASSERT_FALSE(LEAGUE_ENTITY.find(league.name) == LEAGUE_ENTITY.npos);
}

/**
 * Test successful parsing of Media entity payload.
 */
TEST_F(VideoEntityTest, test_parseMediaEntity) {
    VideoEntity::Media media(VideoEntity::Media::Type::VIDEO);
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(MEDIA_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseMediaEntity(jsonPayload, media));

    ASSERT_TRUE(media.type == VideoEntity::Media::Type::MOVIE);
}

/**
 * Test successful parsing of Popularity entity payload.
 */
TEST_F(VideoEntityTest, test_parsePopularityEntity) {
    VideoEntity::Popularity popularity;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(POPULARITY_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parsePopularityEntity(jsonPayload, popularity));

    ASSERT_TRUE(popularity.value);
}

/**
 * Test successful parsing of ProductionCompany entity payload.
 */
TEST_F(VideoEntityTest, test_parseProductionCompanyEntity) {
    VideoEntity::ProductionCompany productionCompany;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(PRODUCTION_COMPANY_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseProductionCompanyEntity(jsonPayload, productionCompany));

    ASSERT_FALSE(PRODUCTION_COMPANY_ENTITY.find(productionCompany.name) == PRODUCTION_COMPANY_ENTITY.npos);
}

/**
 * Test successful parsing of Recency entity payload.
 */
TEST_F(VideoEntityTest, test_parseRecencyEntity) {
    VideoEntity::Recency recency(VideoEntity::Recency::Type::OLD);
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(RECENCY_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseRecencyEntity(jsonPayload, recency));

    ASSERT_TRUE(recency.type == VideoEntity::Recency::Type::NEW);
}

/**
 * Test successful parsing of Season entity payload.
 */
TEST_F(VideoEntityTest, test_parseSeasonEntity) {
    VideoEntity::Season season;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(SEASON_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseSeasonEntity(jsonPayload, season));

    ASSERT_TRUE(season.number == "2");
}

/**
 * Test successful parsing of Sport entity payload.
 */
TEST_F(VideoEntityTest, test_parseSportEntity) {
    VideoEntity::Sport sport;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(SPORT_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseSportEntity(jsonPayload, sport));

    ASSERT_FALSE(SPORT_ENTITY.find(sport.name) == SPORT_ENTITY.npos);
}

/**
 * Test successful parsing of SportsTeam entity payload.
 */
TEST_F(VideoEntityTest, test_parseSportsTeamEntity) {
    VideoEntity::SportsTeam sportsTeam;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(SPORTS_TEAM_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseSportsTeamEntity(jsonPayload, sportsTeam));

    ASSERT_FALSE(SPORTS_TEAM_ENTITY.find(sportsTeam.name) == SPORTS_TEAM_ENTITY.npos);
}

/**
 * Test successful parsing of Video entity payload.
 */
TEST_F(VideoEntityTest, test_parseVideoEntity) {
    VideoEntity::Video video;
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(VIDEO_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseVideoEntity(jsonPayload, video));

    ASSERT_FALSE(VIDEO_ENTITY.find(video.name) == VIDEO_ENTITY.npos);
}

/**
 * Test successful parsing of VideoResolution entity payload.
 */
TEST_F(VideoEntityTest, test_parseVideoResolutionEntity) {
    VideoEntity::VideoResolution videoResolution(VideoEntity::VideoResolution::Type::SD);
    rapidjson::Document jsonPayload;
    ASSERT_FALSE(jsonPayload.Parse(VIDEO_RESOLUTION_ENTITY).HasParseError());
    ASSERT_TRUE(m_videoEntity.parseVideoResolutionEntity(jsonPayload, videoResolution));

    ASSERT_TRUE(videoResolution.type == VideoEntity::VideoResolution::Type::HD);
}

}  // namespace test
}  // namespace acsdkAlexaVideoCommon
}  // namespace alexaClientSDK
