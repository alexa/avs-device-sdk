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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_AUDIOITEM_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_AUDIOITEM_H_

#include <chrono>
#include <memory>
#include <string>

#include <AVSCommon/AVS/Attachment/AttachmentReader.h>

#include "StreamFormat.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace audioPlayer {

/// Struct which contains all the fields which define an audio item for a @c Play directive.
struct AudioItem {
    /// Identifies the @c audioItem.
    std::string id;

    /// Contains the parameters of the stream.
    struct Stream {
        /**
         * Identifies the location of audio content. If the audio content is a binary audio attachment, the value will
         * be a unique identifier for the content, which is formatted as follows: @c "cid:". Otherwise the value will
         * be a remote http/https location.
         */
        std::string url;

        /**
         * The attachment reader for @c url if the audio content is a binary audio attachment.  For http/https
         * attachments, this field is set to @c nullptr and unused.
         */
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> reader;

        /**
         * This field is defined when the @c AudioItem has an associated binary audio attachment. This parameter is
         * ignored if the associated audio is a stream.
         */
        StreamFormat format;

        /**
         * A timestamp indicating where in the stream the client must start playback. For example, when offset is set
         * to 0, this indicates playback of the stream must start at 0, or the start of the stream. Any other value
         * indicates that playback must start from the provided offset.
         */
        std::chrono::milliseconds offset;

        /// The date and time in ISO 8601 format for when the stream becomes invalid.
        std::chrono::steady_clock::time_point expiryTime;

        /// Contains values for progress reports.
        struct ProgressReport {
            /**
             * Specifies when to send the @c ProgressReportDelayElapsed event to AVS.  @c ProgressReportDelayElapsed
             * must only be sent once at the specified interval.
             *
             * @note Some music providers do not require this report. If the report is not required, @c delay will be
             *     set to @c std:chrono::milliseconds::max().
             */
            std::chrono::milliseconds delay;

            /**
             * Specifies when to emit a @c ProgressReportIntervalElapsed event to AVS.
             * @c ProgressReportIntervalElapsed must be sent periodically at the specified interval.
             *
             * @note Some music providers do not require this report. If the report is not required, @c interval will
             *     be set to @c std::chrono::milliseconds::max().
             */
            std::chrono::milliseconds interval;
        } progressReport;

        /// An opaque token that represents the current stream.
        std::string token;

        /// An opaque token that represents the expected previous stream.
        std::string expectedPreviousToken;
    } stream;
};

}  // namespace audioPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AUDIOPLAYER_INCLUDE_AUDIOPLAYER_AUDIOITEM_H_
