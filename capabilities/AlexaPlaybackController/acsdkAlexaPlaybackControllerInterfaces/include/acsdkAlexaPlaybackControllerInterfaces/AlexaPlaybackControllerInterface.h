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

#ifndef ACSDKALEXAPLAYBACKCONTROLLERINTERFACES_ALEXAPLAYBACKCONTROLLERINTERFACE_H_
#define ACSDKALEXAPLAYBACKCONTROLLERINTERFACES_ALEXAPLAYBACKCONTROLLERINTERFACE_H_

#include <string>

#include <AVSCommon/Utils/Optional.h>

#include "acsdkAlexaPlaybackControllerInterfaces/AlexaPlaybackControllerObserverInterface.h"
#include "acsdkAlexaPlaybackControllerInterfaces/PlaybackOperation.h"
#include "acsdkAlexaPlaybackControllerInterfaces/PlaybackState.h"

namespace alexaClientSDK {
namespace acsdkAlexaPlaybackControllerInterfaces {

/**
 * The @c AlexaPlaybackControllerInterface carries out playback controller actions such as play, pause, stop, start
 * over, previous, next, rewind, fast forward.
 *
 * An implementation of the AlexaPlaybackControllerInterface lets the user control media content interactions. Sends
 * operations to the device for controlling playback of audio or video content. This includes play, pause, fastforward,
 * etc. This can be called by multiple callers; for example, the AlexaPlaybackController Capability Agent or the
 * application's GUI.
 *
 * This interface can report to subscribers implemented the @c AlexaPlaybackControllerObserverInterface when there is a
 * playback state change.
 *
 * @note Implementations of this interface must be thread-safe.
 */
class AlexaPlaybackControllerInterface {
public:
    /**
     * Struct for holding AlexaPlaybackController response for AlexaPlaybackController interface
     *
     * @c responseType should be `SUCCESS` if there are no errors while executing playback controller operation.
     * Otherwise, it should return the corresponding error response along with a log message @c errorMessage that would
     * be logged in the cloud, for the purposes of aiding debugging.
     */
    struct Response {
        /**
         * Enum for the different error types understood by the AlexaPlaybackController capability agent.
         */
        enum class Type {
            /// Device processed command successfully without any errors
            SUCCESS,

            /// Indicates the playback operation is not supported on the device.
            PLAYBACK_OPERATION_NOT_SUPPORTED,

            /// Indicates there is no audio or video content that is available when 'play', 'previous', 'next' or other
            /// playback operation is invoked.
            NO_CONTENT_AVAILABLE,

            /// Indicates that the operation cannot be operated in the current mode. For example, if the device should
            /// disable 'play' operation for a video in driving mode
            NOT_SUPPORTED_IN_CURRENT_MODE,

            /// An error occurred that can't be described by one of the other error types. For example, a runtime
            /// exception occurred. We recommend that you always send a more specific error type, if possible.
            INTERNAL_ERROR
        };

        /**
         * Default Constructor. Initializes the response value to 'SUCCESS' by default.
         */
        Response() : responseType{Type::SUCCESS} {};

        /**
         * Constructor
         *
         * @param responseType The response type @c Type
         * @param errorMessage The error message if @c responseType is other than SUCCESS.
         */
        Response(Type responseType, const std::string& errorMessage) :
                responseType{responseType},
                errorMessage{errorMessage} {};

        /// The AlexaPlaybackController response type
        Type responseType;

        /// The error message for logging if the @c responseType is any other than SUCCESS, for the purposes of aiding
        /// debugging.
        std::string errorMessage;
    };

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~AlexaPlaybackControllerInterface() = default;

    /**
     * Execute play operation for audio or video content on the device
     *
     * @return A @c AlexaPlaybackControllerResponse to indicate whether playback operation was successfully executed or
     * encountered an error while executing.
     */
    virtual Response play() = 0;

    /**
     * Execute pause operation for audio or video content on the device
     *
     * @return A @c AlexaPlaybackControllerResponse to indicate whether playback operation was successfully executed or
     * encountered an error while executing.
     */
    virtual Response pause() = 0;

    /**
     * Execute stop operation for audio or video content on the device
     *
     * @return A @c AlexaPlaybackControllerResponse to indicate whether playback operation was successfully executed or
     * encountered an error while executing.
     */
    virtual Response stop() = 0;

    /**
     * Execute start over operation for audio or video content on the device
     *
     * @return A @c AlexaPlaybackControllerResponse to indicate whether playback operation was successfully executed or
     * encountered an error while executing.
     */
    virtual Response startOver() = 0;

    /**
     * Execute previous operation for audio or video content on the device
     *
     * @return A @c AlexaPlaybackControllerResponse to indicate whether playback operation was successfully executed or
     * encountered an error while executing.
     */
    virtual Response previous() = 0;

    /**
     * Execute next operation for audio or video content on the device
     *
     * @return A @c AlexaPlaybackControllerResponse to indicate whether playback operation was successfully executed or
     * encountered an error while executing.
     */
    virtual Response next() = 0;

    /**
     * Execute rewind operation for audio or video content on the device
     *
     * @return A @c AlexaPlaybackControllerResponse to indicate whether playback operation was successfully executed or
     * encountered an error while executing.
     */
    virtual Response rewind() = 0;

    /**
     * Execute fast forward operation for audtio or video content on the device
     *
     * @return A @c AlexaPlaybackControllerResponse to indicate whether playback operation was successfully executed or
     * encountered an error while executing.
     */
    virtual Response fastForward() = 0;

    /**
     * Get the current playback state of the device.
     *
     * @return Return the current playback state for the audio or video content @c PlaybackState
     *
     * @note If @c AlexaPlaybackControllerInterface implementation has configured its instance's property as
     * retrievable, this method should return the current launch target state. Unlike @c
     * AlexaPlaybackControllerObserverInterface observer methods, this returns the current playback state at any given
     * point when requested, with or without a change in playback state.
     */
    virtual acsdkAlexaPlaybackControllerInterfaces::PlaybackState getPlaybackState() = 0;

    /**
     * Returns all supported playback operations the device is expected to understand.
     *
     * @return A @c PlaybackOperation set object that contains the supported playback operations. e.g. Play, Pause,
     * Stop, StartOver, Previous, Next, Rewind, FastForward
     */
    virtual std::set<acsdkAlexaPlaybackControllerInterfaces::PlaybackOperation> getSupportedOperations() = 0;

    /**
     * Adds a @c AlexaPlaybackControllerObserverInterface observer.
     *
     * @note If AlexaPlaybackControllerInterface implementation has configured its instance's property as proactively
     * reported, then it is required to notify observers of @c AlexaPlaybackControllerObserverInterface for any change
     * in its property state. This includes notifying the value when the device starts, if it is different from the last
     * reported value.
     *
     * @param observer The pointer to the @c AlexaPlaybackControllerObserverInterface.
     * @return @c true if the object supports observer notification and observer was successfully added; otherwise,
     * return @c false
     */
    virtual bool addObserver(const std::weak_ptr<AlexaPlaybackControllerObserverInterface>& observer) = 0;

    /**
     * Removes an observer of @c AlexaPlaybackControllerObserverInterface.
     *
     * @param observer The pointer to the @c AlexaPlaybackControllerObserverInterface.
     */
    virtual void removeObserver(const std::weak_ptr<AlexaPlaybackControllerObserverInterface>& observer) = 0;
};

}  // namespace acsdkAlexaPlaybackControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKALEXAPLAYBACKCONTROLLERINTERFACES_ALEXAPLAYBACKCONTROLLERINTERFACE_H_
