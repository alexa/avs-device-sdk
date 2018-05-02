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

Channel::State::State(const std::string& name) :
        name{name},
        focusState{FocusState::NONE},
        timeAtIdle{std::chrono::steady_clock::now()} {
}

Channel::State::State() : focusState{FocusState::NONE}, timeAtIdle{std::chrono::steady_clock::now()} {
}

Channel::Channel(const std::string& name, const unsigned int priority) :
        m_priority{priority},
        m_state{name},
        m_observer{nullptr} {
}

const std::string& Channel::getName() const {
    return m_state.name;
}

unsigned int Channel::getPriority() const {
    return m_priority;
}

bool Channel::setFocus(FocusState focus) {
    if (focus == m_state.focusState) {
        return false;
    }

    m_state.focusState = focus;
    if (m_observer) {
        m_observer->onFocusChanged(m_state.focusState);
    }

    if (FocusState::NONE == m_state.focusState) {
        m_observer = nullptr;
        m_state.timeAtIdle = std::chrono::steady_clock::now();
    }
    return true;
}

void Channel::setObserver(std::shared_ptr<ChannelObserverInterface> observer) {
    m_observer = observer;
}

bool Channel::hasObserver() const {
    return m_observer != nullptr;
}

bool Channel::operator>(const Channel& rhs) const {
    return m_priority < rhs.getPriority();
}

void Channel::setInterface(const std::string& interface) {
    m_state.interfaceName = interface;
}

std::string Channel::getInterface() const {
    return m_state.interfaceName;
}

bool Channel::doesObserverOwnChannel(std::shared_ptr<ChannelObserverInterface> observer) const {
    return observer == m_observer;
}

Channel::State Channel::getState() const {
    return m_state;
}

}  // namespace afml
}  // namespace alexaClientSDK
