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

#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_ERRORTYPECONVERSION_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_ERRORTYPECONVERSION_H_

#include <gst/gst.h>

#include <AVSCommon/Utils/MediaPlayer/ErrorTypes.h>

namespace alexaClientSDK {
namespace mediaPlayer {

/**
 * Convert a GStreamer @c GError to an @c ErrorType.
 *
 * @param error The @c GError to convert.
 * @param remoteResource Indicates whether it should be for a resource that is remote.
 * @return The converted @c ErrorType.
 */
avsCommon::utils::mediaPlayer::ErrorType gerrorToErrorType(const GError* error, bool remoteResource);

}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_ERRORTYPECONVERSION_H_
