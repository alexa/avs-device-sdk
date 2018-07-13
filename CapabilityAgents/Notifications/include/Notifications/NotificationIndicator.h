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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONINDICATOR_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONINDICATOR_H_

#include <string>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace notifications {

/**
 * A struct representation of a @c NotificationIndicator.
 *
 * There is one instance of this class created per SetIndicator directive.
 */
struct NotificationIndicator {
    /**
     * Default constructor
     */
    NotificationIndicator();

    /**
     * Constructor
     *
     * @param persistVisualIndicator A flag indicating whether the visual indicator on the device should stay lit.
     * @param playAudioIndicator A flag indicating whether audio should be played.
     * @param assetId The id of the audio asset to be played.
     * @param url The url of the audio asset to be played.
     */
    NotificationIndicator(
        bool persistVisualIndicator,
        bool playAudioIndicator,
        const std::string& assetId,
        const std::string& url);

    /**
     * Flag specifying if the device must display a persistent visual indicator when processing this
     * @c NotificationIndicator.
     */
    bool persistVisualIndicator;

    /// Flag specifying if the device must play audio when processing this @c NotificationIndicator.
    bool playAudioIndicator;

    struct Asset {
        /// Identifier for asset.
        std::string assetId;

        /** The location of the sound to be played when this @c NotificationIndicator is processed.
         *  @note If the device is offline the device should play the default indicator sound.
         */
        std::string url;
    } asset;
};

}  // namespace notifications
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONINDICATOR_H_
