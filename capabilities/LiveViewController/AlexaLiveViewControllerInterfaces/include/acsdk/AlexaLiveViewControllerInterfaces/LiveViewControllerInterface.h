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
#ifndef ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLERINTERFACES_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLERINTERFACES_LIVEVIEWCONTROLLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLERINTERFACES_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLERINTERFACES_LIVEVIEWCONTROLLERINTERFACE_H_

#include <memory>
#include <string>

#include "LiveViewControllerConfiguration.h"
#include "LiveViewControllerObserverInterface.h"
#include "LiveViewControllerTypes.h"

namespace alexaClientSDK {
namespace alexaLiveViewControllerInterfaces {

/**
 * The @c LiveViewControllerInterface carries out camera live view actions such as starting or
 * stopping a live stream.
 *
 * @note Implementations of this interface must be thread-safe.
 */
class LiveViewControllerInterface {
public:
    /**
     * A request to start camera live view stream.
     */
    struct StartLiveViewRequest {
        /// Live streaming session identifier
        std::string sessionId;

        /// Identifies the viewing device.
        Target target;

        /// Specifies the role of the device for the streaming session.
        Role role;

        /// Camera source and a list of viewing devices in the requested streaming session.
        Participants participants;

        /// Defines the display and audio properties of the streaming session.
        ViewerExperience viewerExperience;
    };

    /**
     * Utility object used for reporting LiveViewController response event.
     */
    struct Response {
        /**
         * Enum for the different response types.
         */
        enum class Type {
            /// Request has been handled successfully.
            SUCCESS,
            /// Battery level at the endpoint is too low.
            FAILED_BATTERY_TOO_LOW,
            /// The media source for the camera can't be found.
            FAILED_MEDIA_SOURCE_NOT_FOUND,
            /// The media source for the camera is asleep.
            FAILED_MEDIA_SOURCE_ASLEEP,
            /// The media source for the camera is off.
            FAILED_MEDIA_SOURCE_TURNED_OFF,
            /// Request is not authorized to access the live view features.
            FAILED_UNAUTHORIZED,
            /// Indicates that an error occurred that can't be described by one of the other error types.
            FAILED_INTERNAL_ERROR
        };

        /// Response error type
        Type type;

        /// The error message for logging if the @c type is anything other than SUCCESS, for the purposes of aiding
        /// debugging.
        std::string errorMessage;

        /**
         * Default constructor, sets response type to @c SUCCESS.
         */
        Response() : type(Type::SUCCESS) {
        }

        /**
         * Constructor.
         * @param type The response type @c Type
         * @param errorMessage The error message if @c responseType is other than SUCCESS.
         *
         */
        Response(Type type, std::string errorMessage) : type{type}, errorMessage{std::move(errorMessage)} {
        }
    };

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~LiveViewControllerInterface() = default;

    /**
     * Start camera live view stream based on the user request.
     *
     * @param request The request for the StartLiveView directive.
     * @return whether camera live view stream has been started successfully, or if an error was encountered in the
     * process.
     * @c LiveViewControllerInterface::Response.type should return SUCCESS if no errors were encountered. Otherwise,
     * @c LiveViewControllerInterface::Response.type should contain the corresponding error code along with a log
     * message in @c LiveViewControllerInterface::Response.errorMessage.
     */
    virtual Response start(std::unique_ptr<StartLiveViewRequest> request) = 0;

    /**
     * Stop camera live view stream.
     *
     * @return whether camera live view stream has been stopped successfully, or if an error was encountered in the
     * process.
     * @c LiveViewControllerInterface::Response.type should return SUCCESS if no errors were encountered. Otherwise,
     * @c LiveViewControllerInterface::Response.type should contain the corresponding error code along with a log
     * message in @c LiveViewControllerInterface::Response.errorMessage.
     */
    virtual Response stop() = 0;

    /**
     * Update the camera state based on the LiveViewController interaction with other components.
     *
     * @param cameraState Incoming camera state.
     * @return Whether camera state has been updated successfully, or if an error was encountered in the
     * process.
     * @c LiveViewControllerInterface::Response.type should return SUCCESS if no errors were encountered. Otherwise,
     * @c LiveViewControllerInterface::Response.type should contain the corresponding error code along with a log
     * message in @c LiveViewControllerInterface::Response.errorMessage.
     */
    virtual Response setCameraState(CameraState cameraState) = 0;

    /**
     * Returns the LiveViewController configuration.
     *
     * @return A @c Configuration object that contains the supported display modes, overlay types and overlay positions.
     */
    virtual Configuration getConfiguration() = 0;

    /**
     * Adds a @c LiveViewControllerObserverInterface observer.
     *
     * @param observer The observer which to callback.
     * @return true if the observer was successfully added; otherwise false.
     */
    virtual bool addObserver(std::weak_ptr<LiveViewControllerObserverInterface> observer) = 0;

    /**
     * Removes an observer of @c LiveViewControllerObserverInterface.
     *
     * @param observer The observer which to callback.
     */
    virtual void removeObserver(std::weak_ptr<LiveViewControllerObserverInterface> observer) = 0;
};

}  // namespace alexaLiveViewControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLERINTERFACES_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLERINTERFACES_LIVEVIEWCONTROLLERINTERFACE_H_
