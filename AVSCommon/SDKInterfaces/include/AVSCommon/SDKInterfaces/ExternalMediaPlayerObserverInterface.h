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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EXTERNALMEDIAPLAYEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EXTERNALMEDIAPLAYEROBSERVERINTERFACE_H_

#include "AVSCommon/SDKInterfaces/ExternalMediaAdapterInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace externalMediaPlayer {

/**
 * struct that includes the Session properties exposed to observers
 */
struct ObservableSessionProperties {
    /**
     * Default Constructor.
     */
    ObservableSessionProperties();

    /**
     * Constructor
     * @param pLoggedIn
     * @param pUserName
     */
    ObservableSessionProperties(bool loggedIn, const std::string& userName);

    /// Flag that identifies if a user is currently logged in or not.
    bool loggedIn;

    /// The userName of the user currently logged in via a Login directive from the AVS.
    std::string userName;
};

inline ObservableSessionProperties::ObservableSessionProperties() : loggedIn{false}, userName{""} {
}

inline ObservableSessionProperties::ObservableSessionProperties(bool loggedIn, const std::string& userName) :
        loggedIn{loggedIn},
        userName{userName} {
}

inline bool operator==(
    const ObservableSessionProperties& observableSessionPropertiesA,
    const ObservableSessionProperties& observableSessionPropertiesB) {
    return observableSessionPropertiesA.loggedIn == observableSessionPropertiesB.loggedIn &&
           observableSessionPropertiesA.userName == observableSessionPropertiesB.userName;
}

/**
 * struct that includes the PlaybackState properties exposed to observers
 */
struct ObservablePlaybackStateProperties {
    /**
     * Default Constructor.
     */
    ObservablePlaybackStateProperties();

    /**
     * Constructor
     * @param pState
     * @param pTrackName
     */
    ObservablePlaybackStateProperties(const std::string& state, const std::string& trackName);

    /// The players current state
    std::string state;

    /// The display name for the currently playing trackname of the track.
    std::string trackName;
};

inline ObservablePlaybackStateProperties::ObservablePlaybackStateProperties() : state{""}, trackName{""} {};

inline ObservablePlaybackStateProperties::ObservablePlaybackStateProperties(
    const std::string& state,
    const std::string& trackName) :
        state{state},
        trackName{trackName} {};

inline bool operator==(
    const ObservablePlaybackStateProperties& observableA,
    const ObservablePlaybackStateProperties& observableB) {
    return observableA.state == observableB.state && observableA.trackName == observableB.trackName;
}

/**
 * This interface allows a derived class to know when a new Login or PlaybackState has been provided
 */
class ExternalMediaPlayerObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ExternalMediaPlayerObserverInterface() = default;

    /**
     * This function is called when the login state is provided as a state observer
     * @param playerId  the ExternalMediaAdapter being reported on
     * @param sessionStateProperties the observable session properties being reported
     */
    virtual void onLoginStateProvided(
        const std::string& playerId,
        const avsCommon::sdkInterfaces::externalMediaPlayer::ObservableSessionProperties sessionStateProperties) = 0;

    /**
     * This function is called when the playback state is provided as a state observer
     * @param playerId the ExternalMediaAdapter being reported on
     * @param playbackStateProperties the observable playback state properties being reported
     */
    virtual void onPlaybackStateProvided(
        const std::string& playerId,
        const avsCommon::sdkInterfaces::externalMediaPlayer::ObservablePlaybackStateProperties
            playbackStateProperties) = 0;
};

}  // namespace externalMediaPlayer
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EXTERNALMEDIAPLAYEROBSERVERINTERFACE_H_
