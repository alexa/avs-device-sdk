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

#ifndef ACSDKALEXASEEKCONTROLLERINTERFACES_ALEXASEEKCONTROLLERINTERFACE_H_
#define ACSDKALEXASEEKCONTROLLERINTERFACES_ALEXASEEKCONTROLLERINTERFACE_H_

#include <chrono>
#include <string>

namespace alexaClientSDK {
namespace acsdkAlexaSeekControllerInterfaces {

/**
 * The @c AlexaSeekControllerInterface carries out actions such as navigating to a specific position in a media item.
 * You should implement this interface for your devices and services that can seek to a specific position. If a device
 * or service can only fast forward or rewind a media item, implement the @c Alexa.PlaybackController interface instead.
 *
 * An implementation of the AlexaSeekControllerInterface lets the user control media content interactions for seek
 * navigation such as AdjustSeekPosition.
 *
 * @note Implementations of this interface must be thread-safe. As a new adjustSeekPosition operation can be invoked
 * before the current operation completes on the device, the seek controller must handle concurrent operations, either
 * by canceling the currently processing operation with an appropriate error @c Response or wait for completion of the
 * first operation before handling the second operation.
 */
class AlexaSeekControllerInterface {
public:
    /**
     * Struct for holding AlexaSeekController response for AlexaSeekController interface
     *
     * @c type should be `SUCCESS` and @c currentMediaPosition should be set to the current media's timestamp
     * position after seeking if there are no errors while executing seek controller operation. Otherwise, it should
     * return the corresponding error response type @c type along with a log message @c errorMessage that would
     * be logged in the cloud, for the purposes of aiding debugging.
     */
    struct Response {
        /**
         * Enum for the different error types understood by the AlexaSeekController capability agent.
         */
        enum class Type {
            /// Device processed command successfully without any errors
            SUCCESS,

            /// Indicates the operation will be canceled due to an existing seek operation still processing on the
            /// device. This error should be returned as the response to the new operation if the device chooses
            /// to have the previous request take precedence.
            ALREADY_IN_OPERATION,

            /// Indicates the operation will be canceled as a new seek operation is initiated before the previous
            /// operation is completed. This error should be returned as the response to the previous operation if
            /// the device chooses to have the new request take precedence.
            CANCELED_BY_NEW_REQUEST,

            /// Indicates there is no audio or video content that is available when a seek operation is invoked
            NO_CONTENT_AVAILABLE,

            /// Indicates the current audio or video content cannot be seeked. For example, a YouTube video ad.
            CONTENT_CANNOT_BE_SEEKED,

            /// An error occurred that can't be described by one of the other error types. For example, a runtime
            /// exception occurred. We recommend that you always send a more specific error type, if possible.
            INTERNAL_ERROR
        };

        /**
         * Constructor for @c Response. This response should be used when the seek
         * operation is executed successfully on the device and should send back the new timestamp of the current
         * media @c currentMediaPosition after seeking. Initializes the type to `SUCCESS` by default.
         *
         * @param currentMediaPosition The media's new timestamp position in milliseconds after executing the seek
         * operation.
         */
        Response(std::chrono::milliseconds currentMediaPosition) :
                type{Type::SUCCESS},
                currentMediaPosition{currentMediaPosition} {};

        /**
         * Constructor for @c Response. This response should be used when the seek operation is not executed
         * successfully on the device. Initializes the @c currentMediaPosition to `-1` to indicate an error and
         * should not be used when @c type is `SUCCESS`.
         *
         * @param type The response type @c Type. Should indicate a corresponding error type and must not be `SUCCESS`.
         * @param errorMessage The error message if @c type is other than `SUCCESS`.
         */
        Response(Type type, const std::string& errorMessage) :
                type{type},
                currentMediaPosition{-1},
                errorMessage{errorMessage} {};

        /// The AlexaSeekControllerInterface response type
        Type type;

        /// The current position of the video or audio content in milliseconds after seeking. This value must always be
        /// greater than or equal to zero if the operation is successful and should be negative if errors are
        /// encountered during the execution of the seek operation.
        std::chrono::milliseconds currentMediaPosition;

        /// The error message for logging if the @c type is any other than SUCCESS, for the purposes of aiding
        /// debugging.
        std::string errorMessage;
    };

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~AlexaSeekControllerInterface() = default;

    /**
     * Execute seek operation for audio or video content on the device. If the seek delta @c deltaPosition would result
     * in the adjusted timestamp position exceeding the timestamp boundaries of the media content, the operation should
     * set the new media's position to the maximum allotted timestamp of the media. For example, the content's new
     * timestamp position should be set to zero if the last position plus the seek delta is less than zero. This method
     * must respond with the new timestamp position @c Response.currentMediaPosition.
     *
     * @param deltaPosition The value to seek a media content. Negative to seek the content backward and positive to
     * seek the content forward.
     *
     * @return A @c AlexaSeekControllerInterface.Response to indicate whether seek operation was successfully executed
     * or encountered an error while executing.
     */
    virtual Response adjustSeekPosition(const std::chrono::milliseconds& deltaPosition) = 0;
};

}  // namespace acsdkAlexaSeekControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKALEXASEEKCONTROLLERINTERFACES_ALEXASEEKCONTROLLERINTERFACE_H_
