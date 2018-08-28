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

#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_OFFSETMANAGER_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_OFFSETMANAGER_H_

#include <chrono>

namespace alexaClientSDK {
namespace mediaPlayer {

/**
 * TODO ACSDK-459: Implement support for seeking across playlists.
 */
class OffsetManager {
public:
    /**
     * Constructor with initialization of members.
     */
    OffsetManager();

    /**
     * Set whether the stream is seekable.
     *
     * @param seekable A boolean indicating whether the stream is seekable.
     */
    void setIsSeekable(bool seekable);

    /**
     * Returns whether the stream is seekable.
     *
     * @return A boolean indicating whether the stream is seekable.
     */
    bool isSeekable();

    /**
     * Set a seek point.
     *
     * @param seekPoint The seek point in milliseconds.
     */
    void setSeekPoint(std::chrono::milliseconds seekPoint);

    /**
     * Get the seek point.
     *
     * @return The seek point in milliseconds.
     */
    std::chrono::milliseconds getSeekPoint();

    /**
     * Returns whether a seek point has been set.
     *
     * @return A boolean indicating whether the stream is seekable.
     */
    bool isSeekPointSet();

    /**
     * Explicitly clears locally cached data, ex @c seekPoint.
     */
    void clear();

private:
    /// The seekpoint in milliseconds.
    std::chrono::milliseconds m_seekPoint;

    /// Whether the stream is seekable.
    bool m_isSeekable;

    /// Whether the seek point has been set.
    bool m_isSeekPointSet;
};

}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_OFFSETMANAGER_H_
