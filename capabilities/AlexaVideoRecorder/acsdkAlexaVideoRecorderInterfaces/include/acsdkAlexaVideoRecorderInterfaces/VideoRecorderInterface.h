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

#ifndef ALEXA_CLIENT_SDK_ACSDKALEXAVIDEORECORDERINTERFACES_INCLUDE_ACSDKALEXAVIDEORECORDERINTERFACES_VIDEORECORDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACSDKALEXAVIDEORECORDERINTERFACES_INCLUDE_ACSDKALEXAVIDEORECORDERINTERFACES_VIDEORECORDERINTERFACE_H_

#include <memory>
#include <string>

#include "VideoRecorderTypes.h"

namespace alexaClientSDK {
namespace acsdkAlexaVideoRecorderInterfaces {

/**
 * The VideoRecorderInterface carries out video recorder actions such as search and record, cancel recording
 * and delete recording.
 *
 * A realization of the VideoRecorderInterface sends response events back to endpoint for search and record
 * and is responsible for providing information regarding storage level and extended GUI.
 *
 * @note Implementations of this interface must be thread-safe.
 */
class VideoRecorderInterface {
public:
    /**
     * Utility object used for reporting VideoRecorder handler response.
     */
    struct Response {
        /**
         * Enum for the different response types understood by the VideoRecorder capability agent.
         */
        enum class Type {
            /// VideoRecorder Request was handled successfully.
            SUCCESS,
            /// The number of allowed failed attempts to perform a VideoRecorder action has been exceeded.
            FAILED_TOO_MANY_FAILED_ATTEMPTS,
            /// Indicates the content does not allow the VideoRecorder action requested. For example, if the user tries
            /// to delete
            /// a recording that is marked as not deletable.
            FAILED_ACTION_NOT_PERMITTED_FOR_CONTENT,
            /// Indicates an additional confirmation must occur before the requested VideoRecorder action can be
            /// completed.
            FAILED_CONFIRMATION_REQUIRED,
            /// Indicates the record operation failed due to restrictions on the content.
            FAILED_CONTENT_NOT_RECORDABLE,
            /// The user is not subscribed to the content for a channel or other subscription-based content.
            FAILED_NOT_SUBSCRIBED,
            /// Indicates that a recording request failed because the recording already exists.
            FAILED_RECORDING_EXISTS,
            /// Indicates that a recording request failed because the DVR storage is full.
            FAILED_STORAGE_FULL,
            /// Indicates the title specified yielded multiple results, and disambiguation is required to determine
            /// the program to record. This value should be used to indicate that the target device will provide a
            /// mechanism for disambiguation. For example, this error could indicate that there are multiple airings
            /// of a program or that the entity requested for recording has multiple programs associated with it.
            FAILED_VIDEO_TITLE_DISAMBIGUATION_REQUIRED,
            /// Indicates that a recording request failed because of a scheduling conflict with another recording.
            FAILED_RECORDING_SCHEDULE_CONFLICT
        };

        /// Response type for VideoRecorder handler responses.
        Type type;

        /// Response message.
        std::string message;

        /*
         * Default Constructor, sets response type to success.
         */
        Response() : type(Type::SUCCESS) {
        }
    };

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~VideoRecorderInterface() = default;

    /**
     * Request to find and record a specified video item, given a set of search criteria.
     *
     * @param request The request for search and record directive.
     * @return An instance of @c VideoRecorderInterface::Response with response. In case of success, the response
     * message will be a string value indicating the status of the recording aka recordingStatus.
     */
    virtual Response searchAndRecord(std::unique_ptr<VideoRecorderRequest> request) = 0;

    /**
     * Request to cancel a scheduled recording for a specified title. This request should result in the cancellation of
     * the specified scheduled recording, or a title that best matches the requested entity.
     *
     * @param request The request for cancel recording directive.
     * @return An instance of @c VideoRecorderInterface::Response with response.
     */
    virtual Response cancelRecording(std::unique_ptr<VideoRecorderRequest> request) = 0;

    /**
     * Request to delete a recorded item. This request should result in the deletion of the specified title, or a title
     * that best matches the requested entity.
     *
     * @param request The request for delete recording directive.
     * @return An instance of @c VideoRecorderInterface::Response with response.
     */
    virtual Response deleteRecording(std::unique_ptr<VideoRecorderRequest> request) = 0;

    /**
     * Gets the property for extended GUI which indicates the type of graphical user interface shown to the
     * user. true to indicate an extended recording GUI is shown, false if the extended recording GUI isn't shown.
     *
     * @return true if extended GUI is shown.
     */
    virtual bool isExtendedRecordingGUIShown() = 0;

    /**
     * Gets the property for storage level which indicates the storage used on the recording device as a
     * percentage.
     *
     * @return Integer indicating storage used on the recording device as a percentage.
     */
    virtual int getStorageUsedPercentage() = 0;
};
}  // namespace acsdkAlexaVideoRecorderInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACSDKALEXAVIDEORECORDERINTERFACES_INCLUDE_ACSDKALEXAVIDEORECORDERINTERFACES_VIDEORECORDERINTERFACE_H_
