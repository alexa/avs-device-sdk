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

#ifndef ACSDK_ALEXAREMOTEVIDEOPLAYERINTERFACES_REMOTEVIDEOPLAYERINTERFACE_H_
#define ACSDK_ALEXAREMOTEVIDEOPLAYERINTERFACES_REMOTEVIDEOPLAYERINTERFACE_H_

#include <memory>
#include <string>

#include "RemoteVideoPlayerConfiguration.h"
#include "RemoteVideoPlayerTypes.h"

namespace alexaClientSDK {
namespace alexaRemoteVideoPlayerInterfaces {

/**
 * The RemoteVideoPlayerInterface carries out remote video player actions such playing video content requested by the
 * user, or displaying results for a user query.
 *
 * @note Implementations of this interface must be thread-safe.
 */
class RemoteVideoPlayerInterface {
public:
    /**
     * Utility object used for reporting RemoteVideoPlayer handler response.
     */
    struct Response {
        /**
         * Enum for the different response types understood by the RemoteVideoPlayer capability agent.
         */
        enum class Type {
            /// RemoteVideoPlayer Request was handled successfully.
            SUCCESS,
            /// The number of allowed failed attempts to perform a RemoteVideoPlayer action has been exceeded.
            FAILED_TOO_MANY_FAILED_ATTEMPTS,
            /// Indicates the endpoint is unreachable or offline.
            FAILED_ENDPOINT_UNREACHABLE,
            /// The user is not subscribed to the content for a channel or other subscription-based content.
            FAILED_NOT_SUBSCRIBED,
            /// Indicates that an error occurred that can't be described by one of the other error types.
            FAILED_INTERNAL_ERROR
        };

        /**
         * Default Constructor, set the response type to success.
         */
        Response() : type{Type::SUCCESS} {
        }

        /**
         * Constructor.
         * @param type The response type @c Type
         * @param errorMessage The error message if @c responseType is other than SUCCESS.
         *
         */
        Response(Type type, std::string errorMessage) : type{type}, errorMessage{std::move(errorMessage)} {
        }

        /// Response type for RemoteVideoPlayer handler responses.
        Type type;

        /// The error message for logging if the @c responseType is anything other than SUCCESS, for the purposes of
        /// aiding debugging.
        std::string errorMessage;
    };

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~RemoteVideoPlayerInterface() = default;

    /**
     * Play video content based on a user request.
     *
     * @param request The request for the search and play directive.
     * @return whether the video playback was successful, or if an error was encountered in the process.
     * @c RemoteVideoPlayerInterface::Response.type should return SUCCESS if no errors were encountered. Otherwise,
     * @c RemoteVideoPlayerInterface::Response.type should contain the corresponding error code along with a log message
     * in @c RemoteVideoPlayerInterface::Response.errorMessage.
     */
    virtual Response playVideo(std::unique_ptr<RemoteVideoPlayerRequest> request) = 0;

    /**
     * Display results in response to a user query.
     *
     * @param request The request for the search and display results directive.
     * @return whether the results for a user search request were successfully displayed, or if an error was encountered
     * in the process. @c RemoteVideoPlayerInterface::Response.type should return SUCCESS if no errors were encountered.
     * Otherwise, @c RemoteVideoPlayerInterface::Response.type should contain the corresponding error code along with a
     * log message in @c RemoteVideoPlayerInterface::Response.errorMessage.
     */
    virtual Response displaySearchResults(std::unique_ptr<RemoteVideoPlayerRequest> request) = 0;

    /**
     * Returns the Remote Video Player configuration.
     *
     * @return A @c Configuration object that contains the supported directive types, entity types and catalog
     * information.
     */
    virtual Configuration getConfiguration() = 0;
};
}  // namespace alexaRemoteVideoPlayerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAREMOTEVIDEOPLAYERINTERFACES_REMOTEVIDEOPLAYERINTERFACE_H_