/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EXTERNALMEDIAADAPTERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EXTERNALMEDIAADAPTERINTERFACE_H_

#include "AVSCommon/Utils/RequiresShutdown.h"

#include <chrono>
#include <string>
#include <vector>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace externalMediaPlayer {

/// Enum class for the different Request Types that an ExternalMediaAdapter handles.
enum class RequestType {
    /// Initialization
    INIT,

    /// DeInitialization
    DEINIT,

    /// Login
    LOGIN,

    /// Logout
    LOGOUT,

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

    /// Starover a track from the beginning
    START_OVER,

    /// Fast-forward
    FAST_FORWARD,

    /// rewind
    REWIND,

    /// Enable Loop on.
    ENABLE_REPEAT,

    /// Enable repeat of a track .
    ENABLE_REPEAT_ONE,

    /// Disable loop on.
    DISABLE_REPEAT,

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

    /// Change was triggerd by periodic polling.
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
 * struct that represents the session state of an adapter.
 */
struct AdapterSessionState {
    /*
     * Default Constructor.
     */
    AdapterSessionState();

    /// The playerId of an adapter which is the pre-negotiated business id for a partner music provider.
    std::string playerId;

    /// The unique device endpoint.
    std::string endpointId;

    /// Flag that identifies if a user is currently logged in or not.
    bool loggedIn;

    /// The userName of the user currently logged in via a Login directive from the AVS.
    std::string userName;

    /// Flag that identifies if the user currently logged in is a guest or not.
    bool isGuest;

    /// Flag that identifies if an application has been launched or not.
    bool launched;

    /**
     * Flag that identifies if the application is currently active or not. This could mean different things
     * for different applications.
     */
    bool active;

    /**
     * The accessToken used to login a user. The access token may also be used as a bearer token if the adapter
     * makes an authenticated Web API to the music provider.
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

    /// The playerId of an adapter which is the pre-negotiated business id for a partner music provider.
    std::string playerId;

    /// The players current state
    std::string state;

    /// The set of states the default player can move into from its current state.
    std::vector<SupportedPlaybackOperation> supportedOperations;

    /// The offset of the track in milliseconds.
    std::chrono::milliseconds trackOffset;

    /// Bool to identify if shuffling is enabled or not.
    bool shuffleEnabled;

    ///  Bool to identify if looping of songs is enabled or not.
    bool repeatEnabled;

    /// The favorite status {"FAVORITED"/"UNFAVORITED"/"NOT_RATED"}.
    Favorites favorites;

    /// The type of the media item. For now hard-coded to ExternalMediaPlayerMusicItem.
    std::string type;

    /// The display name for current playback context, e.g. playlist name.
    std::string playbackSource;

    /// An arbitrary identifier for current playback context as per the music provider, e.g. a URI that can be saved as
    /// a preset or queried to Music Service Provider services for additional info.
    std::string playbackSourceId;

    /// The display name for the currently playing trackname of the track.
    std::string trackName;

    /// The arbitrary identifier for currently playing trackid of the track as per the music provider.
    std::string trackId;

    /// The display value for the number or abstract position of the currently playing track in the album or context
    /// trackNumber of the track.
    std::string trackNumber;

    /// The display name for the currently playing artist.
    std::string artistName;

    /// An arbitrary identifier for currently playing artist as per the music provider, e.g. a URI that can be queried
    /// to MSP services for additional info.
    std::string artistId;

    /// The display name of the currently playing album.
    std::string albumName;

    /// Arbitrary identifier for currently playing album specific to the music provider, e.g. a URI that can be queried
    /// to MSP services for additional info.
    std::string albumId;

    /// The URL for tiny cover art image resource} .
    std::string tinyURL;

    /// The URL for small cover art image resource} .
    std::string smallURL;

    /// The URL for medium cover art image resource} .
    std::string mediumURL;

    /// The URL for large cover art image resource} .
    std::string largeURL;

    /// The Arbitrary identifier for cover art image resource specific to the music provider, for retrieval from an MSP
    /// API.
    std::string coverId;

    /// Music Service Provider name for the currently playing media item; distinct from the application identity
    /// although the two may be the same.
    std::string mediaProvider;

    /// The Media type enum value from {TRACK, PODCAST, STATION, AD, SAMPLE, OTHER} type of the media.
    MediaType mediaType;

    /// Media item duration in milliseconds.
    std::chrono::milliseconds duration;
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
 * The ExternalMediaAdapterInterface specifies the interface of an adapter object to interact with a third party
 * music service provider library. The adapter object handles session management of an user with the third party
 * library/cloud and provides users with an interface to manage behaviors to control their play queue.
 */
class ExternalMediaAdapterInterface : public avsCommon::utils::RequiresShutdown {
public:
    /**
     * ExternalMediaAdapterInterface constructor.
     *
     * @param adapaterName The name of the adapter.
     */
    ExternalMediaAdapterInterface(const std::string& adapaterName);

    /**
     * Destructor.
     */
    virtual ~ExternalMediaAdapterInterface() = default;

    /// Method to initialize a third party library.
    virtual void init() = 0;

    /// Method to de-initialize a third party library.
    virtual void deInit() = 0;

    /**
     * Method to allow a user to login to a third party music provider.
     *
     * @param accessToken The access context of the user identifier.
     * @param userName The userName of the user logging in.
     * @param forceLogin bool which signifies if the adapter has to a force a login or merely cache the access token.
     * @param tokenRefreshInterval The duration in milliseconds for which the accessToken is valid.
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
     * Method to allow a user to initiate play from a third party music service provider based on a play context.
     *
     * @param playContextToken Play context {Track/playlist/album/artist/station/podcast} identifier.
     * @param index The index of the media item in the container, if the container is indexable.
     * @param offset The offset position within media item, in milliseconds.
     */
    virtual void handlePlay(std::string& playContextToken, int64_t index, std::chrono::milliseconds offset) = 0;

    /**
     * Method to initiate the different types of play control like PLAY/PAUSE/RESUME/NEXT/...
     *
     * @param requestType The type of REQUEST. Will always be PLAY/PAUSE/RESUME/NEXT...
     */
    virtual void handlePlayControl(RequestType requestType) = 0;

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

    /// Method to fetch the state(session state and playback state) of an adapter.
    virtual AdapterState getState() = 0;
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

}  // namespace externalMediaPlayer
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EXTERNALMEDIAADAPTERINTERFACE_H_
