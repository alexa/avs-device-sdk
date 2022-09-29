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

#ifndef ACSDK_ALEXARECORDCONTROLLERINTERFACES_RECORDCONTROLLERINTERFACE_H_
#define ACSDK_ALEXARECORDCONTROLLERINTERFACES_RECORDCONTROLLERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace alexaRecordControllerInterfaces {

/**
 * The RecordControllerInterface carries out record controller actions such as start recording and stop recording.
 *
 * @note Implementations of this interface must be thread-safe.
 */
class RecordControllerInterface {
public:
    /**
     * Utility object used for reporting RecordController handler response.
     */
    struct Response {
        /**
         * Enum for the different response types understood by the RecordController capability agent.
         */
        enum class Type {
            /// RecordController Request was handled successfully.
            SUCCESS,
            /// The number of allowed failed attempts to perform a RecordController action has been exceeded.
            FAILED_TOO_MANY_FAILED_ATTEMPTS,
            /// Indicates an additional confirmation must occur before the requested RecordController action can be
            /// completed.
            FAILED_CONFIRMATION_REQUIRED,
            /// Indicates the record operation failed due to restrictions on the content.
            FAILED_CONTENT_NOT_RECORDABLE,
            /// Indicates that a recording request failed because the DVR storage is full.
            FAILED_STORAGE_FULL,
            /// Indicates that the endpoint is unreachable or offline.
            FAILED_ENDPOINT_UNREACHABLE,
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
         * @param errorMessage The error message if @c Type is other than SUCCESS.
         *
         */
        Response(Type type, std::string errorMessage) : type{type}, errorMessage{std::move(errorMessage)} {};

        /// Response type for RecordController handler responses.
        Type type;

        /// The error message for logging if the @c Type is anything other than SUCCESS, for the
        /// purposes of aiding debugging.
        std::string errorMessage;
    };

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~RecordControllerInterface() = default;

    /**
     * Start the recording of the content that is currently playing.
     *
     * @return Whether the Recording was successfully started, or if an error was encountered in the process. @c
     * RecordControllerInterface::Response.type should be set to SUCCESS if no errors were encountered. Otherwise, @c
     * RecordControllerInterface::Response.type should contain the corresponding error code along with a log message in
     * @c RecordControllerInterface::Response.message.
     */
    virtual Response startRecording() = 0;

    /**
     * Stop the current recording.
     *
     * @return Whether the Recording was successfully stopped, or if an error was encountered in the process. @c
     * RecordControllerInterface::Response.type should be set to SUCCESS if no errors were encountered. Otherwise, @c
     * RecordControllerInterface::Response.type should contain the corresponding error code along with a log message in
     * @c RecordControllerInterface::Response.message.
     */
    virtual Response stopRecording() = 0;

    /**
     * Get the current recording state information of the endpoint.
     *
     * @return whether the endpoint is currently recording.
     */
    virtual bool isRecording() = 0;
};

}  // namespace alexaRecordControllerInterfaces
}  // namespace alexaClientSDK

#endif  //  ACSDK_ALEXARECORDCONTROLLERINTERFACES_RECORDCONTROLLERINTERFACE_H_
