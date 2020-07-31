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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYERFACTORYINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYERFACTORYINTERFACE_H_

#include <memory>
#include <string>

#include <AVSCommon/Utils/MediaPlayer/MediaPlayerFactoryObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

// Forward-declare the MediaPlayerInterface class
class MediaPlayerInterface;

/**
 * A struct to hold fingerprint for MediaPlayer.
 * This is an optional structure sent to cloud as part of AudioPlayer Capability.
 *
 * For further details, see AVS AudioPlayer (>= 1.4) Capability documentation.
 */
struct Fingerprint {
    /// Package name of media player.
    std::string package;

    /// Build type of media player.
    std::string buildType;

    /// Version number of media player.
    std::string versionNumber;
};

/**
 * A @c MediaPlayerFactoryInterface allows access to @c MediaPlayerInterface instances as needed (and if availible)
 * This is a capability needed to support pre-buffering
 *
 * Instances are not expected to be Thread-safe.
 */
class MediaPlayerFactoryInterface {
public:
    /**
     * Destructor.
     */
    virtual ~MediaPlayerFactoryInterface() = default;

    /**
     * Get @c Fingerprint of @c MediaPlayer.
     *
     * @return @c Fingerprint of @c MediaPlayer.
     */
    virtual Fingerprint getFingerprint() = 0;

    /**
     * Acquire an instance of a @c MediaPlayerInterface, if available.
     *
     * @return an instance of of @c MediaPlayerInterface, or nullptr if none available.
     */
    virtual std::shared_ptr<MediaPlayerInterface> acquireMediaPlayer() = 0;

    /**
     * Return a @c MediaPlayerInterface instance to the Factory.
     * This MUST be an instance acquired from @c acquireMediaPlayer in this Factory.
     * See @c MediaPlayerFactoryObserverInterface::onReadyToProvideNextPlayer()
     *
     * @param mediaPlayer an instance of of @c MediaPlayerInterface returned from a call to @c acquireMediaPlayer
     * @return true for success, false if mediaPlayer is empty or not acquired from this Factory
     */
    virtual bool releaseMediaPlayer(std::shared_ptr<MediaPlayerInterface> mediaPlayer) = 0;

    /**
     * Returns true if a @c MediaPlayerInterface instance is available
     * (i.e. if a call to @c acquireMediaPlayer would return a valid Player)
     * The return value is valid until the next call to acquireMediaPlayer() or releaseMediaPlayer()
     *
     * @return true if a  @c MediaPlayerInterface instance is available.
     */
    virtual bool isMediaPlayerAvailable() = 0;

    /**
     * Register an @c MediaPlayerFactoryObserverInterface to this Factory.
     *
     * @param observer The @c MediaPlayerFactoryObserverInterface instance to add
     */
    virtual void addObserver(std::shared_ptr<MediaPlayerFactoryObserverInterface> observer) = 0;

    /**
     * Unregister an @c MediaPlayerFactoryObserverInterface to this Factory.
     *
     * @param observer The @c MediaPlayerFactoryObserverInterface instance to remove
     */
    virtual void removeObserver(std::shared_ptr<MediaPlayerFactoryObserverInterface> observer) = 0;
};

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIAPLAYERFACTORYINTERFACE_H_
