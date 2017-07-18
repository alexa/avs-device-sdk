/*
 * Renderer.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDERER_H_
#define ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDERER_H_

#include "Alerts/Renderer/RendererInterface.h"
#include "Alerts/Renderer/RendererObserverInterface.h"

#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>

#include <mutex>
#include <string>
#include <vector>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace renderer {

/**
 * An implementation of an alert renderer.  This class is thread-safe.
 */
class Renderer : public RendererInterface, public avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface {
public:
    /**
     * Creates a @c Renderer.
     *
     * @param mediaPlayer the @c MediaPlayerInterface that the @c Renderer object will interact with.
     * @return The @c Renderer object.
     */
    static std::shared_ptr<Renderer> create(
            std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer);

    void setObserver(RendererObserverInterface* observer) override;

    void start(const std::string & localAudioFilePath,
               const std::vector<std::string> & urls = std::vector<std::string>(),
               int loopCount = 0,
               std::chrono::milliseconds loopPauseInMilliseconds = std::chrono::milliseconds::zero()) override;

    void stop() override;

    void onPlaybackStarted() override;

    void onPlaybackFinished() override;

    void onPlaybackError(std::string error) override;

private:
    /**
     * Constructor.
     *
     * @param mediaPlayer The @c MediaPlayerInterface, which will render audio for an alert.
     */
    Renderer(std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer);

    /**
     * A utility function to actually start rendering the alert, which the executor may call on its own thread.
     */
    void executeStart();

    /**
     * A utility function to actually stop rendering the alert, which the executor may call on its own thread.
     */
    void executeStop();

    /// The @cMediaPlayerInterface which renders the audio files.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_mediaPlayer;

    /// Mutex to control access to local variables from the public api, and also the internal executor.
    std::mutex m_mutex;

    /// Our observer.
    RendererObserverInterface* m_observer;

    /// The local audio file to be rendered.  This should always be set as a fallback resource in case the
    /// rendering of a url happens to fail (eg. network is down at that time).
    std::string m_localAudioFilePath;

    /// An optional sequence of urls to be rendered.  If the vector is empty, m_localAudioFilePath will be
    /// rendered instead.
    std::vector<std::string> m_urls;

    /// The number of times @c m_urls should be rendered.
    int m_loopCount;

    /// The time to pause between the rendering of the @c m_urls sequence.
    std::chrono::milliseconds m_loopPauseInMilliseconds;

    /// A flag to capture if renderer is active.
    bool m_isRendering;

    /// The time when the rendering started.
    std::chrono::steady_clock::time_point m_timeRenderingStarted;

    /// A flag to capture if the renderer has been asked to stop.
    bool m_isStopping;

    /// The @c Executor which serially and asynchronously handles operations with regard to rendering the alert.
    /// TODO : ACSDK-388 to update the onPlayback* callback functions to also go through the executor.
    avsCommon::utils::threading::Executor m_executor;
};

} // namespace renderer
} // namespace alerts
} // namespace capabilityAgents
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDERER_H_