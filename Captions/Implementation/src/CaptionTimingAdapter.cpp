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

#include <algorithm>
#include <iterator>

#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/Utils/Logger/ThreadMoniker.h>
#include <AVSCommon/Utils/Logger/LogEntry.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <Captions/DelayInterface.h>
#include <Captions/CaptionTimingAdapter.h>

namespace alexaClientSDK {
namespace captions {

using namespace avsCommon::utils::logger;
using namespace alexaClientSDK::avsCommon::utils::mediaPlayer;
using namespace alexaClientSDK::avsCommon::utils::threading;

/// String to identify log entries originating from this file.
static const std::string TAG = "CaptionTimingAdapter";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

CaptionTimingAdapter::CaptionTimingAdapter(
    std::shared_ptr<CaptionPresenterInterface> presenter,
    std::shared_ptr<DelayInterface> delayInterface) :
        m_presenter{presenter},
        m_currentCaptionFrameIndex{0},
        m_isCurrentlyPresenting{false},
        m_mediaHasBeenPaused{false},
        m_threadMoniker{ThreadMoniker::generateMoniker()},
        m_delayInterface{delayInterface} {
}

CaptionTimingAdapter::~CaptionTimingAdapter() {
    ACSDK_DEBUG3(LX(__func__));

    // signal to the thread that it's time to wrap up.
    std::unique_lock<std::mutex> lock(m_mutex);
    m_mediaHasBeenPaused = true;
    lock.unlock();
    if (m_thread.joinable()) {
        m_thread.join();
    }
    lock.lock();
    m_captionFrames.clear();
}

void CaptionTimingAdapter::reset() {
    ACSDK_DEBUG3(LX(__func__));

    // signal to the thread that it's time to wrap up.
    std::unique_lock<std::mutex> lock(m_mutex);
    m_mediaHasBeenPaused = true;
    lock.unlock();
    if (m_thread.joinable()) {
        m_thread.join();
    }

    // now reset the variables.
    lock.lock();
    m_captionFrames.clear();
    m_currentCaptionFrameIndex = 0;
    m_mediaHasBeenPaused = false;
}

void CaptionTimingAdapter::queueForDisplay(const CaptionFrame& captionFrame, bool autostart) {
    ACSDK_DEBUG3(LX(__func__).d("captionId", captionFrame.getSourceId()));
    std::unique_lock<std::mutex> lock(m_mutex);
    m_captionFrames.emplace_back(captionFrame);
    ACSDK_DEBUG3(LX("captionFrameQueued")
                     .d("currentIndex", m_currentCaptionFrameIndex)
                     .d("numberOfCaptionFrames", m_captionFrames.size()));
    lock.unlock();
    if (autostart) {
        startCaptionFramesJob();
    } else {
        ACSDK_DEBUG3(LX("presentCaptionFramesJobNotStarted").d("reason", "mediaHasBeenPaused"));
    }
}

void CaptionTimingAdapter::startCaptionFramesJob() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_isCurrentlyPresenting && !m_mediaHasBeenPaused) {
        ACSDK_DEBUG3(LX("startingJobToPresentCaptionFrames"));
        if (m_thread.joinable()) {
            m_thread.join();
        }
        m_isCurrentlyPresenting = true;
        lock.unlock();
        m_thread = std::thread{std::bind(&CaptionTimingAdapter::presentCaptionFramesJob, this)};
    } else {
        ACSDK_DEBUG3(LX("presentCaptionFramesJobNotStarted").d("reason", "jobAlreadyRunning"));
    }
}

void CaptionTimingAdapter::presentCaptionFramesJob() {
    ACSDK_DEBUG3(LX(__func__));

    ThreadMoniker::setThisThreadMoniker(m_threadMoniker);
    CaptionFrame frame;

    while (true) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_mediaHasBeenPaused) {
            lock.unlock();
            break;
        }
        if (m_currentCaptionFrameIndex >= m_captionFrames.size()) {
            // There are no more caption frames to be shown.
            lock.unlock();
            break;
        }
        frame = m_captionFrames[m_currentCaptionFrameIndex];

        ACSDK_DEBUG3(LX("sendingCaptionFrameToPresenter")
                         .d("sourceId", frame.getSourceId())
                         .d("currentIndex", m_currentCaptionFrameIndex)
                         .d("numberOfCaptionFrames", m_captionFrames.size())
                         .d("delay", std::to_string(frame.getDelay().count()))
                         .d("duration", std::to_string(frame.getDuration().count())));
        m_currentCaptionFrameIndex++;
        lock.unlock();

        m_delayInterface->delay(frame.getDelay());
        lock.lock();
        if (m_mediaHasBeenPaused) {
            m_presenter->onCaptionActivity(frame, avsCommon::avs::FocusState::BACKGROUND);
            lock.unlock();
            break;
        }
        m_presenter->onCaptionActivity(frame, avsCommon::avs::FocusState::FOREGROUND);
        lock.unlock();
        m_delayInterface->delay(frame.getDuration());
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_mediaHasBeenPaused) {
        m_presenter->onCaptionActivity(frame, avsCommon::avs::FocusState::BACKGROUND);
        ACSDK_DEBUG3(LX("endingCaptionDisplay").d("reason", "mediaNotPlaying"));
    } else {
        m_presenter->onCaptionActivity(frame, avsCommon::avs::FocusState::NONE);
        ACSDK_DEBUG3(LX("endingCaptionDisplay").d("reason", "reachedEndOfCaptions"));
    }

    m_isCurrentlyPresenting = false;
}

void CaptionTimingAdapter::start() {
    ACSDK_DEBUG3(LX(__func__));
    std::unique_lock<std::mutex> lock(m_mutex);
    m_mediaHasBeenPaused = false;
    lock.unlock();
    startCaptionFramesJob();
}

void CaptionTimingAdapter::stop() {
    ACSDK_DEBUG3(LX(__func__));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mediaHasBeenPaused = true;
    m_currentCaptionFrameIndex = 0;
}

void CaptionTimingAdapter::pause() {
    ACSDK_DEBUG3(LX(__func__));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mediaHasBeenPaused = true;
}

}  // namespace captions
}  // namespace alexaClientSDK
