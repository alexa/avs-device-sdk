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

#include "AFML/Channel.h"

namespace alexaClientSDK {
namespace afml {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

Channel::Channel(const unsigned int priority) :
        m_priority{priority},
        m_focusState{FocusState::NONE},
        m_observer{nullptr} {
}

unsigned int Channel::getPriority() const {
    return m_priority;
}

void Channel::setFocus(FocusState focus) {
    if (focus == m_focusState) {
        return;
    }

    m_focusState = focus;
    if (m_observer) {
        m_observer->onFocusChanged(m_focusState);
    }

    if (FocusState::NONE == m_focusState) {
        m_observer = nullptr;
    }
}

void Channel::setObserver(std::shared_ptr<ChannelObserverInterface> observer) {
    setFocus(FocusState::NONE);
    m_observer = observer;
}

bool Channel::operator>(const Channel& rhs) const {
    return m_priority < rhs.getPriority();
}

void Channel::setActivityId(const std::string& activityId) {
    m_currentActivityId = activityId;
}

std::string Channel::getActivityId() const {
    return m_currentActivityId;
}

bool Channel::stopActivity(const std::string& activityId) {
    if (activityId != m_currentActivityId) {
        return false;
    }
    if (!m_observer) {
        return false;
    }
    setFocus(FocusState::NONE);
    return true;
}

bool Channel::doesObserverOwnChannel(std::shared_ptr<ChannelObserverInterface> observer) const {
    return observer == m_observer;
}

}  // namespace afml
}  // namespace alexaClientSDK
