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

#ifndef ALEXA_CLIENT_SDK_VIDEOCONTENT_INCLUDE_ACSDK_VIDEOCONTENT_VIDEOENTITYTYPES_H_
#define ALEXA_CLIENT_SDK_VIDEOCONTENT_INCLUDE_ACSDK_VIDEOCONTENT_VIDEOENTITYTYPES_H_

#include <string>
#include <unordered_map>

#include <rapidjson/document.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace acsdkAlexaVideoCommon {

/**
 * @brief A helper class that provides an aggregation of all video entity types. Users can ask Alexa to search for video
 * content by specifying characteristics of the content, such as a season and episode of a TV show, or a genre of movie.
 * Alexa sends these characteristics, called entity types for searching the requested content. This class also provides
 * methods to parse the entity payload into respective types.
 *
 * @see https://developer.amazon.com/en-US/docs/alexa/video/entity-types-for-video-content.html
 */
class VideoEntity {
public:
    /**
     * Enum definitions for all video entity types.
     */
    enum class EntityType {
        /// An actor credited in video media content.
        ACTOR,
        /// An application that the user might launch, such as Prime Video.
        APP,
        /// Represents the identifying data for a television channel.
        CHANNEL,
        /// A character in a movie or TV show.
        CHARACTER,
        /// A director credited in video media content.
        DIRECTOR,
        /// Episode number for serial content.
        EPISODE,
        /// A type of event, such as a music concert or sporting event.
        EVENT,
        /// Related video entities, such as a trilogy or five part series.
        FRANCHISE,
        /// Category of video content, such as action, drama, or comedy.
        GENRE,
        /// Represents the name of a sporting league.
        LEAGUE,
        /// Type category of the video content, such as movie or TV show.
        MEDIA_TYPE,
        /// Indicates whether the user asks for popular content.
        POPULARITY,
        /// Production company name for the video media content.
        PRODUCTION_COMPANY,
        /// Indicates whether the user content is new or old.
        RECENCY,
        /// Season number for serial content.
        SEASON,
        /// Category of a sport, such as football.
        SPORT,
        /// Name of a professional sports team.
        SPORTS_TEAM,
        /// Identifying data for the piece of video content, such as the movie title.
        VIDEO,
        /// Represents the requested video resolution, such as HD.
        VIDEO_RESOLUTION
    };

    /**
     * An actor type entity object that represents an actor credited in video media content.
     */
    struct Actor {
        /// External Id map where key is the provider, value is the Id; for example, a key-value pair could be asin_us
        /// and B089PWRYQL.
        std::unordered_map<std::string, std::string> externalIds;
        /// Name of the actor. There is no restriction on the format of actor name.
        std::string name;
    };

    /**
     * An app type entity object that represents an application; for example, Prime Video.
     */
    struct App {
        /// External Id map where key is the provider, value is the Id; for example, a key-value pair could be asin_us
        /// and B089PWRYQL.
        std::unordered_map<std::string, std::string> externalIds;
        /// Name of the application.
        std::string name;
    };

    /**
     * A channel type entity object that represents the identifying data for a television channel.
     */
    struct Channel {
        /**
         * Instantiate a @c Channel entity type.
         *
         * @param channelNum Channel number for this instance.
         */
        Channel(int channelNum) : channelNumber{channelNum} {};

        /// External Id map where key is the provider, value is the Id; for example, a key-value pair could be asin_us
        /// and B089PWRYQL.
        std::unordered_map<std::string, std::string> externalIds;
        /// The channel number.
        int channelNumber;
        /// The call sign for the channel.
        std::string channelCallSign;
        /// The URI of the channel.
        std::string uri;
        /// The name of the channel.
        std::string name;
    };

    /**
     * An character type entity object represented in video media content; for example, Snow White.
     */
    struct Character {
        /// External Id map where key is the provider, value is the Id; for example, a key-value pair could be asin_us
        /// and B089PWRYQL.
        std::unordered_map<std::string, std::string> externalIds;
        /// The name of the character
        std::string name;
    };

    /**
     * A director is an entity object that represents a director credited for a video media content.
     */
    struct Director {
        /// External Id map where key is the provider, value is the Id; for example, a key-value pair could be asin_us
        /// and B089PWRYQL.
        std::unordered_map<std::string, std::string> externalIds;
        /// The name of the director.
        std::string name;
    };

    /**
     * A episode type entity object that represents the episode numbers for serial content like TV. For example, the
     * eighth episode in season two of "Transparent" would have episode number 8.
     */
    struct Episode {
        /// The episode number.
        std::string number;
    };

    /**
     * An event is an entity object to a type of event; examples would include sports, music, or other types of events.
     * For example, searching for "Football games" would search for a 'game' event entity and a 'football' sport entity.
     */
    struct Event {
        /// External Id map where key is the provider, value is the Id; for example, a key-value pair could be asin_us
        /// and B089PWRYQL.
        std::unordered_map<std::string, std::string> externalIds;
        /// The name of the event type.
        std::string name;
    };

    /**
     * A franchise type entity object that represents a video entity which can represent a number of video entities,
     * like movies or TV shows. For example, Intergalactic Wars
     */
    struct Franchise {
        /// The name of the franchise.
        std::string name;
    };

    /**
     * A genre type entity object that represents the genre of video media content such as action, drama or comedy. For
     * example, Action.
     */
    struct Genre {
        /// External Id map where key is the provider, value is the Id; for example, a key-value pair could be asin_us
        /// and B089PWRYQL.
        std::unordered_map<std::string, std::string> externalIds;
        /// The name of the genre.
        std::string name;
    };

    /**
     * A league type entity object that represents the categorical information for a sporting league.
     */
    struct League {
        /// External Id map where key is the provider, value is the Id; for example, a key-value pair could be asin_us
        /// and B089PWRYQL.
        std::unordered_map<std::string, std::string> externalIds;
        /// The name of the league.
        std::string name;
    };

    /**
     * A media type entity object that represents the media type category of a particular piece of content.
     */
    struct Media {
        /**
         * Enum definitions for all types of Media.
         */
        enum class Type {
            /// Movie type.
            MOVIE,
            /// Video type.
            VIDEO,
            /// TV Show type.
            TV_SHOW
        };

        /**
         * Instantiate a @c Media entity.
         *
         * @param typeValue Enum value for the media type.
         */
        Media(Type typeValue) : type{typeValue} {};

        /// The media type.
        Type type;
    };

    /**
     * A popularity type entity object that indicates whether the user asks for popular content.
     */
    struct Popularity {
        /// Indicates to provider that the user searches for popular content
        bool value;
    };

    /**
     * A production company type entity object that represents the production company name for the video media content.
     */
    struct ProductionCompany {
        /// Production company name
        std::string name;
    };

    /**
     * A recency type entity object that resolved values for Recency indicate whether the user asks for new or old
     * content.
     */
    struct Recency {
        /**
         * Enum definitions for whether the content is recent or not.
         */
        enum class Type {
            /// New content.
            NEW,
            /// Old content.
            OLD
        };

        /**
         * Instantiate a @c Recency entity.
         *
         * @param recencyType indicates how recent was this content.
         */
        Recency(Type recencyType) : type{recencyType} {};

        /// Indicates whether the user searches for new or old content.
        Type type;
    };

    /**
     * A season type entity object that represents the season numbers for serial content like TV Shows. For example, the
     * eighth episode in season two of a serial content would have season number 2.
     */
    struct Season {
        /// The season number.
        std::string number;
    };

    /**
     * A sport type entity object that represents the categorical information of a sport; for example, football.
     */
    struct Sport {
        /// External Id map where key is the provider, value is the Id; for example, a key-value pair could be asin_us
        /// and B089PWRYQL.
        std::unordered_map<std::string, std::string> externalIds;
        /// The name of the sport.
        std::string name;
    };

    /**
     * A sports team type entity object that represents the categorical information of a professional sports team.
     */
    struct SportsTeam {
        /// External Id map where key is the provider, value is the Id; for example, a key-value pair could be asin_us
        /// and B089PWRYQL.
        std::unordered_map<std::string, std::string> externalIds;
        /// The name of the sports team.
        std::string name;
    };

    /**
     * A video type entity object that represents the identifying data for the piece of video content.
     */
    struct Video {
        /// External Id map where key is the provider, value is the Id; for example, a key-value pair could be asin_us
        /// and B089PWRYQL.
        std::unordered_map<std::string, std::string> externalIds;
        /// The name of the video.
        std::string name;
    };

    /**
     * A video resolution type entity object that represents the requested video resolution.
     */
    struct VideoResolution {
        /**
         * Enum definitions for all video resolution types.
         */
        enum class Type {
            /// SD type (480p).
            SD,
            /// HD type (720p-1080p).
            HD,
            /// 4K type (2160p).
            FOUR_K,
            /// 8K type (4320p).
            EIGHT_K
        };

        /**
         * Instantiate a @c VideoResolution entity.
         *
         * @param resolutionType The resolution type of the content.
         */
        VideoResolution(Type resolutionType) : type{resolutionType} {};

        /// Video resolution.
        Type type;
    };

    /**
     * Parse input json to determine if it contains a video entity type.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] entityType Parsed @c entity type object.
     * @return true if json contains a video entity type, false otherwise.
     */
    bool parseVideoEntityType(const rapidjson::Value& entityJson, VideoEntity::EntityType& entityType);

    /**
     * Parse video entity of type @c Actor.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] actor Parsed @c Media object.
     * @return true if parsing is successful.
     */
    bool parseActorEntity(const rapidjson::Value& entityJson, Actor& actor);

    /**
     * Parse video entity of type @c App.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] actor Parsed @c Media object.
     * @return true if parsing is successful.
     */
    bool parseAppEntity(const rapidjson::Value& entityJson, App& app);

    /**
     * Parse video entity of type @c Channel.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] channel Parsed @c Channel object.
     * @return true if parsing is successful.
     */
    bool parseChannelEntity(const rapidjson::Value& entityJson, Channel& channel);

    /**
     * Parse video entity of type @c Character.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] character Parsed @c Character object.
     * @return true if parsing is successful.
     */
    bool parseCharacterEntity(const rapidjson::Value& entityJson, Character& character);

    /**
     * Parse video entity of type @c Director.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] director Parsed @c Director object.
     * @return true if parsing is successful.
     */
    bool parseDirectorEntity(const rapidjson::Value& entityJson, Director& director);

    /**
     * Parse video entity of type @c Episode.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] episode Parsed @c Episode object.
     * @return true if parsing is successful.
     */
    bool parseEpisodeEntity(const rapidjson::Value& entityJson, Episode& episode);

    /**
     * Parse video entity of type @c Event.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] event Parsed @c Event object.
     * @return true if parsing is successful.
     */
    bool parseEventEntity(const rapidjson::Value& entityJson, Event& event);

    /**
     * Parse video entity of type @c Franchise.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] franchise Parsed @c Franchise object.
     * @return true if parsing is successful.
     */
    bool parseFranchiseEntity(const rapidjson::Value& entityJson, Franchise& franchise);

    /**
     * Parse video entity of type @c Genre.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] genre Parsed @c Genre object.
     * @return true if parsing is successful.
     */
    bool parseGenreEntity(const rapidjson::Value& entityJson, Genre& genre);

    /**
     * Parse video entity of type @c League.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] league Parsed @c Media object.
     * @return true if parsing is successful.
     */
    bool parseLeagueEntity(const rapidjson::Value& entityJson, League& league);

    /**
     * Parse video entity of type @c Media.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] media Parsed @c Media object.
     * @return true if parsing is successful.
     */
    bool parseMediaEntity(const rapidjson::Value& entityJson, Media& media);

    /**
     * Parse video entity of type @c Popularity.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] popularity Parsed @c Popularity object.
     * @return true if parsing is successful.
     */
    bool parsePopularityEntity(const rapidjson::Value& entityJson, Popularity& popularity);

    /**
     * Parse video entity of type @c ProductionCompany.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] actor Parsed @c ProductionCompany object.
     * @return true if parsing is successful.
     */
    bool parseProductionCompanyEntity(const rapidjson::Value& entityJson, ProductionCompany& productionCompany);

    /**
     * Parse video entity of type @c Recency.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] recency Parsed @c Recency object.
     * @return true if parsing is successful.
     */
    bool parseRecencyEntity(const rapidjson::Value& entityJson, Recency& recency);

    /**
     * Parse video entity of type @c Season.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] season Parsed @c Season object.
     * @return true if parsing is successful.
     */
    bool parseSeasonEntity(const rapidjson::Value& entityJson, Season& season);

    /**
     * Parse video entity of type @c Sport.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] sport Parsed @c Sport object.
     * @return true if parsing is successful.
     */
    bool parseSportEntity(const rapidjson::Value& entityJson, Sport& sport);

    /**
     * Parse video entity of type @c SportsTeam.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] sportsTeam Parsed @c SportsTeam object.
     * @return true if parsing is successful.
     */
    bool parseSportsTeamEntity(const rapidjson::Value& entityJson, SportsTeam& sportsTeam);

    /**
     * Parse video entity of type @c Video.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] video Parsed @c Video object.
     * @return true if parsing is successful.
     */
    bool parseVideoEntity(const rapidjson::Value& entityJson, Video& video);

    /**
     * Parse video entity of type @c VideoResolution.
     * @param[in] entityJson RapidJson object with the entity json to be parsed.
     * @param[out] videoResolution Parsed @c VideoResolution object.
     * @return true if parsing is successful.
     */
    bool parseVideoResolutionEntity(const rapidjson::Value& entityJson, VideoResolution& videoResolution);

    /**
     * Convert Video Entity type to its corresponding string.
     * @param entityType the video entity type.
     * @return the string corresponding to the entity type.
     */
    static std::string convertEntityTypeToString(const VideoEntity::EntityType& entityType);
};

}  // namespace acsdkAlexaVideoCommon
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_VIDEOCONTENT_INCLUDE_ACSDK_VIDEOCONTENT_VIDEOENTITYTYPES_H_
