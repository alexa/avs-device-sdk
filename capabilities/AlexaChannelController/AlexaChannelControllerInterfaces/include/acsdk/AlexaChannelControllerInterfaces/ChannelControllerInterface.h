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

#ifndef ACSDK_ALEXACHANNELCONTROLLERINTERFACES_CHANNELCONTROLLERINTERFACE_H_
#define ACSDK_ALEXACHANNELCONTROLLERINTERFACES_CHANNELCONTROLLERINTERFACE_H_

#include <memory>
#include <string>

#include "ChannelControllerObserverInterface.h"
#include "ChannelType.h"

namespace alexaClientSDK {
namespace alexaChannelControllerInterfaces {

/**
 * The @c ChannelControllerInterface carries out channel controller actions such as changing the channel, and
 * skipping (going to the next or previous) channels.
 *
 * @note Implementations of this interface must be thread-safe.
 */
class ChannelControllerInterface {
public:
    /**
     * Utility object used for reporting ChannelController handler response.
     */
    struct Response {
        /**
         * Enum for the different response types understood by the ChannelController capability agent.
         */
        enum class Type {
            /// ChannelController Request was handled successfully.
            SUCCESS,
            /// The number of allowed failed attempts to perform a ChannelController action has been exceeded.
            FAILED_TOO_MANY_FAILED_ATTEMPTS,
            /// Indicates the endpoint is unreachable or offline.
            FAILED_ENDPOINT_UNREACHABLE,
            /// The directive contains a value that is not valid for the target endpoint, such as an invalid channel
            /// value.
            FAILED_INVALID_VALUE,
            /// Indicates the content does not allow the ChannelController action requested.
            FAILED_ACTION_NOT_PERMITTED_FOR_CONTENT,
            /// Indicates the user is not subscribed to the content for a channel.
            FAILED_NOT_SUBSCRIBED,
            /// Indicates that an error occurred that can't be described by one of the other error types.
            FAILED_INTERNAL_ERROR
        };

        /**
         * Default Constructor, set the response type to success.
         */
        Response() : type{Type::SUCCESS} {};

        /**
         * Constructor.
         * @param type The response type @c Type
         * @param errorMessage The error message if @c responseType is other than SUCCESS.
         *
         */
        Response(Type type, std::string errorMessage) : type{type}, errorMessage{std::move(errorMessage)} {};

        /// Response type for ChannelController handler responses.
        Type type;

        /// The error message for logging if the @c responseType is anything other than SUCCESS, for the purposes of
        /// aiding debugging.
        std::string errorMessage;
    };

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~ChannelControllerInterface() = default;

    /**
     * Changes the channel on the endpoint.
     *
     * @param channel The channel related values such as number, callSign and affiliateCallSign, uri, as well as
     * metadata for the channel like name and image.
     * @return Whether the channel was successfully changed, or if an error was encountered in the process.
     * @c ChannelControllerInterface::Response.type should return SUCCESS if no errors were encountered.
     * Otherwise, @c ChannelControllerInterface::Response.type should contain the corresponding error code along with a
     * log message in @c ChannelControllerInterface::Response.errorMessage.
     */
    virtual Response change(std::unique_ptr<alexaChannelControllerTypes::Channel> channel) = 0;

    /**
     * Jump to the previous channel on the the endpoint. Decrementing the first channel must wrap to the end of the
     * list.
     *
     * @return Whether the channel was successfully decremented, or if an error was encountered in the process.
     * @c ChannelControllerInterface::Response.type should return SUCCESS if no errors were encountered.
     * Otherwise, @c ChannelControllerInterface::Response.type should contain the corresponding error code along with a
     * log message in @c ChannelControllerInterface::Response.errorMessage.
     *
     */
    virtual Response decrementChannel() = 0;

    /**
     * Jump to the next channel on the the endpoint. Incrementing the last channel must wrap to the beginning of the
     * list.
     *
     * @return Whether the channel was successfully incremented, or if an error was encountered in the process.
     * @c ChannelControllerInterface::Response.type should return SUCCESS if no errors were encountered.
     * Otherwise, @c ChannelControllerInterface::Response.type should contain the corresponding error code along with a
     * log message in @c ChannelControllerInterface::Response.errorMessage.
     *
     */
    virtual Response incrementChannel() = 0;

    /**
     * Get the current channel information of the endpoint.
     *
     * @return the current @c Channel of the endpoint
     */
    virtual std::unique_ptr<alexaChannelControllerTypes::Channel> getCurrentChannel() = 0;

    /**
     * Adds a @c ChannelControllerObserverInterface observer.
     *
     * @note If ChannelControllerInterface implementation has configured its instance's property as proactively
     * reported, then it is required to notify observers of @c ChannelControllerObserverInterface for any change in
     * its property state. This includes notifying the value when the device starts, if it is different from the last
     * reported value.
     *
     * @param observer The pointer to the @c ChannelControllerObserverInterface.
     * @return @c true if the object supports observer notification and observer was successfully added; otherwise,
     * return @c false
     */
    virtual bool addObserver(std::weak_ptr<ChannelControllerObserverInterface> observer) = 0;

    /**
     * Removes a observer of @c ChannelControllerObserverInterface.
     *
     * @param observer The pointer to the @c ChannelControllerObserverInterface.
     */
    virtual void removeObserver(std::weak_ptr<ChannelControllerObserverInterface> observer) = 0;
};

}  // namespace alexaChannelControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXACHANNELCONTROLLERINTERFACES_CHANNELCONTROLLERINTERFACE_H_
