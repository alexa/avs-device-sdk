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

#include "Integration/AipStateObserver.h"

namespace alexaClientSDK {
namespace integration {

using avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface;

AipStateObserver::AipStateObserver() : m_state(AudioInputProcessorObserverInterface::State::IDLE) {
}

void AipStateObserver::onStateChanged(AudioInputProcessorObserverInterface::State newState) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue.push_back(newState);
    m_state = newState;
    m_wakeTrigger.notify_all();
}

bool AipStateObserver::checkState(
    const AudioInputProcessorObserverInterface::State expectedState,
    const std::chrono::seconds duration) {
    AudioInputProcessorObserverInterface::State hold = waitForNext(duration);
    return hold == expectedState;
}

AudioInputProcessorObserverInterface::State AipStateObserver::waitForNext(const std::chrono::seconds duration) {
    AudioInputProcessorObserverInterface::State ret;
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTrigger.wait_for(lock, duration, [this]() { return !m_queue.empty(); })) {
        ret = AudioInputProcessorObserverInterface::State::IDLE;
        return ret;
    }
    ret = m_queue.front();
    m_queue.pop_front();
    return ret;
}

}  // namespace integration
}  // namespace alexaClientSDK
