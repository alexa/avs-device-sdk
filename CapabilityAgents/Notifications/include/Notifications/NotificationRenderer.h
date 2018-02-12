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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONRENDERER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONRENDERER_H_

#include <mutex>
#include <unordered_set>

#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>

#include "NotificationRendererInterface.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace notifications {

/**
 * Implementation of NotificationRendererInterface using the @c MediaPlayerInterface
 */
class NotificationRenderer
        : public NotificationRendererInterface
        , public avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface
        , public std::enable_shared_from_this<NotificationRenderer> {
public:
    /// A type that identifies which source is currently being operated on.
    using SourceId = avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId;

    /**
     * Create a NotificationRenderer.  The new NotificationRenderer starts life in the
     * IDLE state, awaiting request to render notifications.
     *
     * @param mediaPlayer The @c MediaPlayer instance to use to render audio.
     * @return The new NotificationRenderer, or null if the operation fails.
     */
    static std::shared_ptr<NotificationRenderer> create(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer);

    /// @name NotificationRendererInterface methods
    /// @{
    void addObserver(std::shared_ptr<NotificationRendererObserverInterface> observer) override;
    void removeObserver(std::shared_ptr<NotificationRendererObserverInterface> observer) override;
    bool renderNotification(std::function<std::unique_ptr<std::istream>()> audioFactory, const std::string& url)
        override;
    bool cancelNotificationRendering() override;
    /// @}

    /// @name MediaPlayerObserverInterface methods
    /// @{
    void onPlaybackStarted(SourceId sourceId) override;
    void onPlaybackStopped(SourceId sourceId) override;
    void onPlaybackFinished(SourceId sourceId) override;
    void onPlaybackError(SourceId sourceId, const avsCommon::utils::mediaPlayer::ErrorType& type, std::string error)
        override;
    /// @}

private:
    /**
     * The different states that a NotificationRenderer may be in.  The normal flow of states is:
     * IDLE -> RENDERING_PREFERRED -> (if failed, RENDERING_DEFAULT) -> NOTIFYING -> IDLE.
     * If @c cancelNotificationRendering() is called during rendering, the state can also transition from
     * {RENDERING_PREFERRED|RENDERING_DEFAULT} -> CANCELLING -> NOTIFYING -> IDLE.
     */
    enum class State {
        /**
         * Awaiting a request to render a notification. Transitions to:
         * - RENDERING_PREFERRED when a new rendering request is received.
         */
        IDLE,
        /**
         * Rendering the preferred audio asset. Requests to render while already rendering are refused, not queued).
         * Transitions to:
         * - RENDERING_DEFAULT if rendering the preferred asset fails.
         * - CANCELLING if a request is made to cancel rendering.
         * - NOTIFYING if rendering the preferred asset completes.
         */
        RENDERING_PREFERRED,
        /**
         * Rendering the default audio asset. Requests to render while already rendering are refused, not queued).
         * Transitions to:
         * - CANCELLING if a request is made to cancel rendering.
         * - NOTIFYING if rendering the default asset completes.
         */
        RENDERING_DEFAULT,
        /**
         * Canceling a request to render a notification. Transitions to:
         * - NOTIFYING once cancellation has completed.
         */
        CANCELLING,
        /**
         * Notifying that rendering finished (even if rendering failed or was cancelled). Transitions to:
         * - IDLE once callbacks to observers have returned.
         */
        NOTIFYING
    };

    /**
     * Constructor.
     *
     * @param mediaPlayer The @c MediaPlayer instance to use to render audio.
     */
    NotificationRenderer(std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer);

    /**
     * Handle the completion of rendering an audio asset, whether successful or not.
     *
     * @param sourceId The @c SourceId of the audio that is expected to be playing.
     */
    void onRenderingFinished(SourceId sourceId);

    /**
     * Set a new state - rejecting invalid state changes.
     *
     * @param newState The new state to enter.
     * @return Whether the new state was accepted.
     */
    bool setState(State newState);

    /**
     * Set a new state - rejecting invalid state changes. @note Caller must be holdling m_mutex.
     *
     * @param newState The new state to enter.
     * @return Whether the new state was accepted.
     */
    bool setStateLocked(State newState);

    /**
     * Verify that audio rendering is in progress.
     *
     * @param caller The name of the calling method (for logging a more useful error)
     * @param sourceId The @c SourceId to verify.
     * @return Whether the specified @c SourceId matches @c m_sourceId.
     */
    bool verifyRenderingLocked(const std::string& caller, SourceId sourceId);

    /// The mediaPlayer with which to render the notification.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_mediaPlayer;

    /// The observers to notify when rendering is finished.  Access serialized by @c m_mutex.
    std::unordered_set<std::shared_ptr<NotificationRendererObserverInterface>> m_observers;

    /// Mutex to serialize access to various data members.
    std::mutex m_mutex;

    /// Used to wake a thread waiting for a state change.
    std::condition_variable m_wakeTrigger;

    /// Current state. Access serialized by @c m_mutex.
    State m_state;

    /// Factory for creating @c istream instances containing the default audio asset.
    std::function<std::unique_ptr<std::istream>()> m_audioFactory;

    /// The id associated with the media that our MediaPlayer is currently handling.
    SourceId m_sourceId;

    /// Future used to capture result from std::async() call used if PLAYBACK_PREFERRED fails.
    std::future<void> m_renderFallbackFuture;

    /// Friend relationship to allow accessing State to convert it to a string for logging.
    friend std::ostream& operator<<(std::ostream& stream, const NotificationRenderer::State state);
};

}  // namespace notifications
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONRENDERER_H_
