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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONTIMINGADAPTER_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONTIMINGADAPTER_H_

#include <atomic>
#include <thread>
#include <vector>

#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/Threading/TaskThread.h>
#include <Captions/CaptionPresenterInterface.h>

#include "CaptionTimingAdapterInterface.h"
#include "SystemClockDelay.h"
#include "TimingAdapterFactory.h"

namespace alexaClientSDK {
namespace captions {

/**
 * A helper class to handle the timing of when to display @c CaptionFrame objects for the appropriate media sources.
 */
class CaptionTimingAdapter : public CaptionTimingAdapterInterface {
public:
    /**
     * Constructor.
     *
     * @param presenter The @c CaptionPresenterInterface which should receive caption frames that are ready to display.
     * @param delayInterface The @c DelayInterface to use while waiting to display the next caption frame.
     */
    CaptionTimingAdapter(
        std::shared_ptr<CaptionPresenterInterface> presenter,
        std::shared_ptr<DelayInterface> delayInterface);

    /**
     * Destructor.
     */
    virtual ~CaptionTimingAdapter() override;

    /// @name CaptionTimingAdapterInterface methods
    /// @{
    virtual void queueForDisplay(const CaptionFrame& captionFrame, bool autostart) override;
    virtual void reset() override;
    virtual void start() override;
    virtual void stop() override;
    virtual void pause() override;
    ///@}

private:
    /**
     * The function which runs on another thread, sending @c CaptionFrame objects to the presenter at the appropriate
     * times.
     */
    void presentCaptionFramesJob();

    /**
     * Helper function to handle the starting of @c m_thread.
     */
    void startCaptionFramesJob();

    /// The presenter instance which will receive the display requests for the caption frames.
    std::shared_ptr<CaptionPresenterInterface> m_presenter;

    /// Mutex to protect access to @c m_captionFrames and any supporting values.
    std::mutex m_mutex;

    /// The index of the caption frame in m_captionFrames which should be shown next, gated by @c m_captionFrameMutex.
    size_t m_currentCaptionFrameIndex;

    /// Indicates that @c m_thread is currently or waiting to display a caption frame, gated by @c m_captionFrameMutex.
    bool m_isCurrentlyPresenting;

    /// Used to communicate with @c m_thread that the captions should also be paused, gated by @c m_captionFrameMutex.
    bool m_mediaHasBeenPaused;

    /// Contains all the caption frames that have been or should be shown, gated by @c m_captionFrameMutex.
    std::vector<captions::CaptionFrame> m_captionFrames;

    /// The thread that uses @c m_presenter and @c m_delayInterface to send caption frames at the right times.
    std::thread m_thread;

    /// The task thread moniker, used for logging and debugging to identify @c m_thread.
    std::string m_threadMoniker;

    /// The time delay implementation.
    std::shared_ptr<DelayInterface> m_delayInterface;
};

}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONTIMINGADAPTER_H_
