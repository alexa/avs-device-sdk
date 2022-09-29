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

#ifndef ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYERINTERFACES_INCLUDE_ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAADAPTERINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYERINTERFACES_INCLUDE_ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAADAPTERINTERFACE_H_

#include <chrono>
#include <set>
#include <string>
#include <unordered_map>

#include <AVSCommon/AVS/PlayRequestor.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayerInterfaces {

/// Enum class for the different Request Types that an ExternalMediaAdapter
/// handles.
enum class RequestType {
    /// Initialization
    INIT,

    /// DeInitialization
    DEINIT,

    /// Login
    LOGIN,

    /// Logout
    LOGOUT,

    /// Register
    REGISTER,

    /// Play
    PLAY,

    /// Resume
    RESUME,

    /// Pause
    PAUSE,

    /// Stop
    STOP,

    /// Pause or Resume depending on current state.
    PAUSE_RESUME_TOGGLE,

    /// Next
    NEXT,

    /// Previous
    PREVIOUS,

    /// Starover from the beginning
    START_OVER,

    /// Fast-forward
    FAST_FORWARD,

    /// rewind
    REWIND,

    /// Enable Repeat of a track.
    ENABLE_REPEAT_ONE,

    /// Disable Repeat of a track.
    DISABLE_REPEAT_ONE,

    /// Enable Loop on.
    ENABLE_REPEAT,

    /// Disable loop on.
    DISABLE_REPEAT,

    /// Enable shuffle.
    ENABLE_SHUFFLE,

    /// Disable shuffle.
    DISABLE_SHUFFLE,

    /// Mark a track as favorite.(thumbs up true)
    FAVORITE,

    /// Unmark a track as favorite.(thumbs up false)
    DESELECT_FAVORITE,

    /// Mark a track as not a favorite.(thumbs down true)
    UNFAVORITE,

    /// Unmark a track as not a favorite.(thumbs down false)
    DESELECT_UNFAVORITE,

    /// Seek to a given offset.
    SEEK,

    /// Seek to an offset relative to the current offset.
    ADJUST_SEEK,

    /// Set volume level to a given volume.
    SET_VOLUME,

    /// Adjust volume level relative to the existing volume.
    ADJUST_VOLUME,

    /// Set mute to true/false.
    SET_MUTE,

    /// Set the name the external media player will show for this device
    SET_DISPLAY_NAME,

    /// Get Info
    GET_INFO,

    /// Add User Message
    ADD_USER,

    /// Reset User Message
    RESET_USER,

    /// None means there are no pending requests.
    NONE
};

enum class SupportedPlaybackOperation {
    /// Play
    PLAY,

    /// Resume
    RESUME,

    /// Pause
    PAUSE,

    /// Stop
    STOP,

    /// Next
    NEXT,

    /// Previous
    PREVIOUS,

    /// Start over a track from the beginning
    START_OVER,

    /// Fast-forward
    FAST_FORWARD,

    /// rewind
    REWIND,

    /// Enable Loop on.
    ENABLE_REPEAT,

    /// Disable loop on.
    DISABLE_REPEAT,

    /// Enable repeat of a track .
    ENABLE_REPEAT_ONE,

    /// Disable repeat of a track.
    DISABLE_REPEAT_ONE,

    /// Enable shuffle.
    ENABLE_SHUFFLE,

    /// Disable shuffle.
    DISABLE_SHUFFLE,

    /// Mark a track as favorite.(thumbs up)
    FAVORITE,

    /// Mark a track as not a favorite.(thumbs down)
    UNFAVORITE,

    /// Seek to a given offset.
    SEEK,

    /// Seek to an offset relative to the current offset.
    ADJUST_SEEK,

};

/**
 * Enum which identifies how a state change was triggered.
 */
enum class ChangeCauseType {
    /// The state change was triggered as result of voice interaction.
    VOICE_INTERACTION,

    /// Change was triggered by a physical interaction.
    PHYSICAL_INTERACTION,

    /// Change was triggered by an app interaction.
    APP_INTERACTION,

    /// Change was triggered by a rule.
    RULE_TRIGGER,

    /// Change was triggered by periodic polling.
    PERIODIC_POLL
};

/**
 * Enum which identifies the ratings of a track.
 */
enum class Favorites {
    /// Favorite rating.
    FAVORITED,

    /// Unfavorite rating.
    UNFAVORITED,

    /// track not rated.
    NOT_RATED
};

/**
 * Enum which identifies the media type.
 */
enum class MediaType {
    /// The media is track.
    TRACK,

    /// The media is a podcast.
    PODCAST,

    /// The media is a station.
    STATION,

    /// The media is an ad.
    AD,

    /// The media is a sample.
    SAMPLE,

    /// The media type is something other than track/podcast/station/ad/sample
    OTHER
};

/**
 * Type of navigation when external media player app is first invoked via AVS
 */
enum class Navigation {
    /**
     * Source dependant behavior
     */
    DEFAULT,
    /**
     * No navigation should occur
     */
    NONE,
    /**
     * External app should take foreground
     */
    FOREGROUND
};

/**
 * Convert navigation enum to a string
 *
 * @param navigation Navigation to convert
 * @return Navigation in string form
 */
inline std::string navigationToString(Navigation navigation) {
    switch (navigation) {
        case Navigation::DEFAULT:
            return "DEFAULT";
        case Navigation::NONE:
            return "NONE";
        case Navigation::FOREGROUND:
            return "FOREGROUND";
    }
    return "";
}

/**
 * Write a @c Navigation to an @c ostream.
 *
 * @param stream The stream to write the value to.
 * @param event The @c Navigation value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const Navigation& navigation) {
    return stream << navigationToString(navigation);
}

/**
 * Convert the given string to a Navigation enum
 *
 * @param str String to convert
 * @return Navigation enum
 */
inline Navigation stringToNavigation(const std::string& str) {
    if (str == "DEFAULT") {
        return Navigation::DEFAULT;
    } else if (str == "NONE") {
        return Navigation::NONE;
    } else if (str == "FOREGROUND") {
        return Navigation::FOREGROUND;
    } else {
        // default to DEFAULT
        return Navigation::DEFAULT;
    }
}

/**
 * struct that represents the session state of an adapter.
 */
struct AdapterSessionState {
    /*
     * Default Constructor.
     */
    AdapterSessionState();

    /// The playerId of an adapter which is the pre-negotiated business id for a
    /// partner music provider.
    std::string playerId;

    /// The localPlayerId of an adapter which is the pre-negotiated business id
    /// for a partner music provider.
    std::string localPlayerId;

    /// The unique device endpoint.
    std::string endpointId;

    /// Flag that identifies if a user is currently logged in or not.
    bool loggedIn;

    /// The userName of the user currently logged in via a Login directive from
    /// the AVS.
    std::string userName;

    /// Flag that identifies if the user currently logged in is a guest or not.
    bool isGuest;

    /// Flag that identifies if an application has been launched or not.
    bool launched;

    /**
     * Flag that identifies if the application is currently active or not. This
     * could mean different things for different applications.
     */
    bool active;

    /// The service provider interface (SPI) version.
    std::string spiVersion;

    /// The playerCookie to select version-specific content or actions.
    std::string playerCookie;

    /// An opaque token for the domain or skill that is presently associated with
    /// this player.
    std::string skillToken;

    /// A universally unique identifier (UUID) generated to the RFC 4122
    /// specification.
    std::string playbackSessionId;

    /**
     * The accessToken used to login a user. The access token may also be used as
     * a bearer token if the adapter makes an authenticated Web API to the music
     * provider.
     */
    std::string accessToken;

    /// The validity period of the token in milliseconds.
    std::chrono::milliseconds tokenRefreshInterval;
};

/**
 * struct that encapsulates an adapter's playback state.
 */
struct AdapterPlaybackState {
    /// Default constructor.
    AdapterPlaybackState();

    /// The playerId of an adapter which is the pre-negotiated business id for a
    /// partner music provider.
    std::string playerId;

    /// The players current state
    std::string state;

    /// The set of states the default player can move into from its current state.
    std::set<SupportedPlaybackOperation> supportedOperations;

    /// The offset of the track in milliseconds.
    std::chrono::milliseconds trackOffset;

    /// Bool to identify if shuffling is enabled or not.
    bool shuffleEnabled;

    ///  Bool to identify if looping of songs is enabled or not.
    bool repeatEnabled;

    /// The favorite status {"FAVORITED"/"UNFAVORITED"/"NOT_RATED"}.
    Favorites favorites;

    /// The type of the media item. For now hard-coded to
    /// ExternalMediaPlayerMusicItem.
    std::string type;

    /// The display name for current playback context, e.g. playlist name.
    std::string playbackSource;

    /// An arbitrary identifier for current playback context as per the music
    /// provider, e.g. a URI that can be saved as a preset or queried to Music
    /// Service Provider services for additional info.
    std::string playbackSourceId;

    /// An unique identifiier for the current playback of the track.
    /// If same track exists twice in a context, each instance will have a unique playbackId. It is
    /// valid for only one playback of a track and once the track is played or skipped-over, the
    /// playback-id is no longer valid. e.g if you skip a track and then come back to it using
    /// skip-prev, the playback-id changes. It is for debugging purposes and it is optional.
    std::string playbackId;

    /// The display name for the currently playing trackname of the track.
    std::string trackName;

    /// The arbitrary identifier for currently playing trackid of the track as per
    /// the music provider.
    std::string trackId;

    /// The display value for the number or abstract position of the currently
    /// playing track in the album or context trackNumber of the track.
    std::string trackNumber;

    /// The display name for the currently playing artist.
    std::string artistName;

    /// An arbitrary identifier for currently playing artist as per the music
    /// provider, e.g. a URI that can be queried to MSP services for additional
    /// info.
    std::string artistId;

    /// The display name of the currently playing album.
    std::string albumName;

    /// Arbitrary identifier for currently playing album specific to the music
    /// provider, e.g. a URI that can be queried to MSP services for additional
    /// info.
    std::string albumId;

    /// The URL for tiny cover art image resource} .
    std::string tinyURL;

    /// The URL for small cover art image resource} .
    std::string smallURL;

    /// The URL for medium cover art image resource} .
    std::string mediumURL;

    /// The URL for large cover art image resource} .
    std::string largeURL;

    /// The Arbitrary identifier for cover art image resource specific to the
    /// music provider, for retrieval from an MSP API.
    std::string coverId;

    /// Music Service Provider name for the currently playing media item; distinct
    /// from the application identity although the two may be the same.
    std::string mediaProvider;

    /// The Media type enum value from {TRACK, PODCAST, STATION, AD, SAMPLE,
    /// OTHER} type of the media.
    MediaType mediaType;

    /// Media item duration in milliseconds.
    std::chrono::milliseconds duration;

    /// The PlayRequestor object from the @c PLAY directive.
    avsCommon::avs::PlayRequestor playRequestor;
};

/**
 * Class that encapsulates an adapter session and playback state.
 */
class AdapterState {
public:
    /// Default constructor.
    AdapterState() = default;

    /// Variable to hold the session state.
    AdapterSessionState sessionState;

    /// Variable to hold the playback state.
    AdapterPlaybackState playbackState;
};

/**
 * The ExternalMediaAdapterInterface specifies the interface of an adapter
 * object to interact with a third party music service provider library. The
 * adapter object handles session management of an user with the third party
 * library/cloud and provides users with an interface to manage behaviors to
 * control their play queue.
 */
class ExternalMediaAdapterInterface : public virtual avsCommon::utils::RequiresShutdown {
public:
    /**
     * ExternalMediaAdapterInterface constructor.
     *
     * @param adapterName The name of the adapter.
     */
    explicit ExternalMediaAdapterInterface(const std::string& adapterName);

    /**
     * Destructor.
     */
    virtual ~ExternalMediaAdapterInterface() = default;

    /**
     * HandlePlayParams is a struct that contains the parameters for the handlePlay method
     */
    struct HandlePlayParams {
        /// Play context token for specifying what to play
        std::string playContextToken;
        /// Index for track
        int64_t index;
        /// Offset to play from
        std::chrono::milliseconds offset;
        /// Associated skillToken
        std::string skillToken;
        /// Playback session id for identifying the session
        std::string playbackSessionId;
        /// Navigation for indicating foreground or not
        Navigation navigation;
        /// Whether or not to preload first
        bool preload;
        /// PlayRequestor for indicating who requested playback
        avsCommon::avs::PlayRequestor playRequestor;
        /// Playback target to play on
        std::string playbackTarget;

        /**
         * Construct HandlePlayParams
         *
         * @param playContextToken Play context
         * {Track/playlist/album/artist/station/podcast} identifier.
         * @param index The index of the media item in the container, if the container
         * is indexable.
         * @param offset The offset position within media item, in milliseconds.
         * @param skillToken An opaque token for the domain or skill that is presently
         * associated with this player.
         * @param playbackSessionId A universally unique identifier (UUID) generated
         * to the RFC 4122 specification.
         * @param navigation Communicates desired visual display behavior for the app
         * associated with playback.
         * @param preload If true, this Play directive is intended to preload the
         * identified content only but not begin playback.
         * @param playRequestor The @c PlayRequestor object that is used to
         * distinguish if it's a music alarm or not.
         * @param playbackTarget The @c PlaybackTarget is used to specify the targeted device
         * that will handle the playback action.
         */
        HandlePlayParams(
            const std::string& playContextToken,
            int64_t index,
            std::chrono::milliseconds offset,
            const std::string& skillToken,
            const std::string& playbackSessionId,
            Navigation navigation,
            bool preload,
            const avsCommon::avs::PlayRequestor& playRequestor,
            const std::string& playbackTarget) :
                playContextToken{playContextToken},
                index{index},
                offset{offset},
                skillToken{skillToken},
                playbackSessionId{playbackSessionId},
                navigation{navigation},
                preload{preload},
                playRequestor(playRequestor),
                playbackTarget{playbackTarget} {
        }
    };

    /// Method to initialize a third party library.
    virtual void init() = 0;

    /// Method to de-initialize a third party library.
    virtual void deInit() = 0;

    /**
     * Method to allow a user to login to a third party music provider.
     *
     * @param accessToken The access context of the user identifier.
     * @param userName The userName of the user logging in.
     * @param forceLogin bool which signifies if the adapter has to a force a
     * login or merely cache the access token.
     * @param tokenRefreshInterval The duration in milliseconds for which the
     * accessToken is valid.
     */
    virtual void handleLogin(
        const std::string& accessToken,
        const std::string& userName,
        bool forceLogin,
        std::chrono::milliseconds tokenRefreshInterval) = 0;

    /**
     * Method that handles logging out a user from a third party library/cloud.
     */
    virtual void handleLogout() = 0;

    /**
     * Method to allow a user to initiate play from a third party music service
     * provider based on a play context.
     *
     * @param params Handle play parameters required for playback
     */
    virtual void handlePlay(const HandlePlayParams& params) = 0;

    /**
     * Method to initiate the different types of play control like
     * PLAY/PAUSE/RESUME/NEXT/...
     *
     * @param requestType The type of REQUEST. Will always be PLAY/PAUSE/RESUME/NEXT...
     * @param playbackTarget The @c PlaybackTarget is used to specify the targeted device
     * that will handle the play control.
     * @param isLocal Whether play control is locally triggered on device
     */
    virtual void handlePlayControl(RequestType requestType, const std::string& playbackTarget) = 0;

    /**
     * Method to seek to the given offset.
     *
     * @param offset The offset to seek to.
     */
    virtual void handleSeek(std::chrono::milliseconds offset) = 0;

    /**
     * Method to seek to an offset from the current position.
     *
     * @param deltaOffset The offset to seek to relative to the current offset.
     */
    virtual void handleAdjustSeek(std::chrono::milliseconds deltaOffset) = 0;

    /**
     * Method to alert if a player has been authorized. This method also provides
     * the playerId and skillToken as identified by the cloud. Authorization may
     * be revoked.
     *
     * @param authorized Whether the player is authorized.
     * @param playerId The playerId of this player.
     * @param defaultSkillToken The default skillToken to use if no directives
     * have supplied one yet.
     */
    virtual void handleAuthorized(
        bool authorized,
        const std::string& playerId,
        const std::string& defaultSkillToken) = 0;

    /// Method to fetch the state(session state and playback state) of an adapter.
    virtual AdapterState getState() = 0;

    /**
     * This function retrieves the offset of the current track the adapter is
     * handling.
     *
     * @return This returns the offset in milliseconds.
     */
    virtual std::chrono::milliseconds getOffset() = 0;
};

inline AdapterSessionState::AdapterSessionState() : loggedIn{false}, isGuest{false}, launched{false}, active{false} {
}

inline AdapterPlaybackState::AdapterPlaybackState() :
        state{"IDLE"},
        trackOffset{0},
        shuffleEnabled{false},
        repeatEnabled{false},
        favorites{Favorites::NOT_RATED},
        mediaType{MediaType::TRACK},
        duration{0} {
}

inline ExternalMediaAdapterInterface::ExternalMediaAdapterInterface(const std::string& adapterName) :
        RequiresShutdown{adapterName} {
}

/**
 * Convert a @c RequestType to an AVS-compliant @c std::string.
 *
 * @param requestType The @c RequestType to convert.
 * @return The AVS-compliant string representation of @c RequestType
 */
inline std::string RequestTypeToString(RequestType requestType) {
    switch (requestType) {
        case RequestType::INIT:
            return "INIT";
        case RequestType::DEINIT:
            return "DEINIT";
        case RequestType::LOGIN:
            return "LOGIN";
        case RequestType::LOGOUT:
            return "LOGOUT";
        case RequestType::REGISTER:
            return "REGISTER";
        case RequestType::PLAY:
            return "PLAY";
        case RequestType::RESUME:
            return "RESUME";
        case RequestType::PAUSE:
            return "PAUSE";
        case RequestType::STOP:
            return "STOP";
        case RequestType::PAUSE_RESUME_TOGGLE:
            return "PAUSE_RESUME_TOGGLE";
        case RequestType::NEXT:
            return "NEXT";
        case RequestType::PREVIOUS:
            return "PREVIOUS";
        case RequestType::START_OVER:
            return "START_OVER";
        case RequestType::FAST_FORWARD:
            return "FAST_FORWARD";
        case RequestType::REWIND:
            return "REWIND";
        case RequestType::ENABLE_REPEAT_ONE:
            return "ENABLE_REPEAT_ONE";
        case RequestType::DISABLE_REPEAT_ONE:
            return "DISABLE_REPEAT_ONE";
        case RequestType::ENABLE_REPEAT:
            return "ENABLE_REPEAT";
        case RequestType::DISABLE_REPEAT:
            return "DISABLE_REPEAT";
        case RequestType::ENABLE_SHUFFLE:
            return "ENABLE_SHUFFLE";
        case RequestType::DISABLE_SHUFFLE:
            return "DISABLE_SHUFFLE";
        case RequestType::FAVORITE:
            return "FAVORITE";
        case RequestType::DESELECT_FAVORITE:
            return "DESELECT_FAVORITE";
        case RequestType::UNFAVORITE:
            return "UNFAVORITE";
        case RequestType::DESELECT_UNFAVORITE:
            return "DESELECT_UNFAVORITE";
        case RequestType::SEEK:
            return "SEEK";
        case RequestType::ADJUST_SEEK:
            return "ADJUST_SEEK";
        case RequestType::SET_VOLUME:
            return "SET_VOLUME";
        case RequestType::ADJUST_VOLUME:
            return "ADJUST_VOLUME";
        case RequestType::SET_MUTE:
            return "SET_MUTE";
        case RequestType::SET_DISPLAY_NAME:
            return "SET_DISPLAY_NAME";
        case RequestType::GET_INFO:
            return "GET_INFO";
        case RequestType::ADD_USER:
            return "ADD_USER";
        case RequestType::RESET_USER:
            return "RESET_USER";
        case RequestType::NONE:
            return "NONE";
    }

    return "unknown request type";
}

/**
 * Convert a @c SupportedPlaybackOperation to an AVS-compliant @c std::string.
 *
 * @param operation The @c SupportedPlaybackOperation to convert.
 * @return The AVS-compliant string representation of @c operation.
 */
inline std::string SupportedPlaybackOperationToString(SupportedPlaybackOperation operation) {
    switch (operation) {
        case SupportedPlaybackOperation::PLAY:
            return "Play";
        case SupportedPlaybackOperation::RESUME:
            return "Resume";
        case SupportedPlaybackOperation::PAUSE:
            return "Pause";
        case SupportedPlaybackOperation::STOP:
            return "Stop";
        case SupportedPlaybackOperation::NEXT:
            return "Next";
        case SupportedPlaybackOperation::PREVIOUS:
            return "Previous";
        case SupportedPlaybackOperation::START_OVER:
            return "StartOver";
        case SupportedPlaybackOperation::FAST_FORWARD:
            return "FastForward";
        case SupportedPlaybackOperation::REWIND:
            return "Rewind";
        case SupportedPlaybackOperation::ENABLE_REPEAT:
            return "EnableRepeat";
        case SupportedPlaybackOperation::ENABLE_REPEAT_ONE:
            return "EnableRepeatOne";
        case SupportedPlaybackOperation::DISABLE_REPEAT:
            return "DisableRepeat";
        case SupportedPlaybackOperation::DISABLE_REPEAT_ONE:
            return "DisableRepeatOne";
        case SupportedPlaybackOperation::ENABLE_SHUFFLE:
            return "EnableShuffle";
        case SupportedPlaybackOperation::DISABLE_SHUFFLE:
            return "DisableShuffle";
        case SupportedPlaybackOperation::FAVORITE:
            return "Favorite";
        case SupportedPlaybackOperation::UNFAVORITE:
            return "Unfavorite";
        case SupportedPlaybackOperation::SEEK:
            return "SetSeekPosition";
        case SupportedPlaybackOperation::ADJUST_SEEK:
            return "AdjustSeekPosition";
    }

    return "unknown operation";
}

/**
 * Convert a @c ChangeCauseType to an AVS-compliant @c std::string.
 *
 * @param changeType The @c ChangeCauseType to convert.
 * @return The AVS-compliant string representation of @c changeType.
 */
inline std::string ChangeTriggerToString(ChangeCauseType changeType) {
    switch (changeType) {
        case ChangeCauseType::VOICE_INTERACTION:
            return "VOICE_INTERACTION";
        case ChangeCauseType::PHYSICAL_INTERACTION:
            return "PHYSICAL_INTERACTION";
        case ChangeCauseType::APP_INTERACTION:
            return "APP_INTERACTION";
        case ChangeCauseType::RULE_TRIGGER:
            return "RULE_TRIGGER";
        case ChangeCauseType::PERIODIC_POLL:
            return "PERIODIC_POLL";
    }

    return "unknown changeTrigger";
}

/**
 * Convert a @c Favorites to an AVS-compliant @c std::string.
 *
 * @param rating The @c ChangeCauseType to convert.
 * @return The AVS-compliant string representation of @c rating.
 */
inline std::string RatingToString(Favorites rating) {
    switch (rating) {
        case Favorites::FAVORITED:
            return "FAVORITED";
        case Favorites::UNFAVORITED:
            return "UNFAVORITED";
        case Favorites::NOT_RATED:
            return "NOT_RATED";
    }

    return "unknown rating";
}

/**
 * Convert a @c Favorites to an AVS-compliant @c std::string.
 *
 * @param mediaType The @c MediaType to convert.
 * @return The AVS-compliant string representation of @c mediaType.
 */
inline std::string MediaTypeToString(MediaType mediaType) {
    switch (mediaType) {
        case MediaType::TRACK:
            return "TRACK";
        case MediaType::PODCAST:
            return "PODCAST";
        case MediaType::STATION:
            return "STATION";
        case MediaType::AD:
            return "AD";
        case MediaType::SAMPLE:
            return "SAMPLE";
        case MediaType::OTHER:
            return "OTHER";
    }

    return "unknown mediaType";
}

inline std::string SHUFFLE_STATUS_STRING(bool shuffleEnabled) {
    return (shuffleEnabled == true) ? "SHUFFLED" : "NOT_SHUFFLED";
}

inline std::string REPEAT_STATUS_STRING(bool repeatEnabled) {
    return (repeatEnabled == true) ? "REPEATED" : "NOT_REPEATED";
}

}  // namespace acsdkExternalMediaPlayerInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYERINTERFACES_INCLUDE_ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAADAPTERINTERFACE_H_
