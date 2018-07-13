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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONRENDERERINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONRENDERERINTERFACE_H_

#include <iostream>
#include <memory>
#include <string>

#include "NotificationRendererObserverInterface.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace notifications {

/**
 * Interface to an object that handles rendering notification audio clips.
 */
class NotificationRendererInterface {
public:
    virtual ~NotificationRendererInterface() = default;

    /**
     * Add an observer to receive a notifications about rendering notification audio clips.
     *
     * @param observer The observer to call back.
     */
    virtual void addObserver(std::shared_ptr<NotificationRendererObserverInterface> observer) = 0;

    /**
     * Remove an observer from the set of observers to receive a notifications about rendering notification audio clips.
     *
     * @param observer The observer to call back.
     */
    virtual void removeObserver(std::shared_ptr<NotificationRendererObserverInterface> observer) = 0;

    /**
     * Render (play) a notification audio clip.  If the asset at the specified url cannot be played for some reason,
     * the default notification audio clip should be played, instead.  If renderNotification is called while another
     * rendering operation is in progress, this method fails and returns false.
     * @note: Calling this method from a NotificationRendererObserverInterface callback will lead to a deadlock.
     *
     * @param audioFactory A function that produces an audio stream to play if the audio specified by
     * @c url can not be played.
     * @param url URL of the preferred audio asset to play.
     * @return Whether rendering the notification was initiated.
     */
    virtual bool renderNotification(
        std::function<std::unique_ptr<std::istream>()> audioFactory,
        const std::string& url) = 0;

    /**
     * Cancel any ongoing rendering of a notification audio clip.  Further render requests will be refused until
     * an observer callback is made to indicate that rendering has finished (i.e. cancellation is complete).
     *
     * @return Whether or not the cancellation was allowed.
     */
    virtual bool cancelNotificationRendering() = 0;
};

}  // namespace notifications
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONRENDERERINTERFACE_H_
