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

#ifndef ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_CHANNEL_H_
#define ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_CHANNEL_H_

#include <chrono>
#include <memory>
#include <string>

#include <AVSCommon/SDKInterfaces/ChannelObserverInterface.h>
#include <AVSCommon/AVS/FocusState.h>

namespace alexaClientSDK {
namespace afml {

/**
 * A Channel represents a focusable layer with a priority, allowing the observer which has acquired the Channel to
 * understand focus changes.
 */
class Channel {
public:
    /*
     * This class contains the states of the @c Channel.  The states inside this structure are intended to be shared via
     * the @c ActivityTrackerInterface.
     */
    struct State {
        /// Constructor with @c Channel name as the parameter.
        State(const std::string& name);

        /// Constructor.
        State();

        /*
         * The channel's name.  Although the name is not dynamic, it is useful for identifying which channel the state
         * belongs to.
         */
        std::string name;

        /// The current Focus of the Channel.
        avsCommon::avs::FocusState focusState;

        /// The name of the AVS interface that is occupying the Channel.
        std::string interfaceName;

        /// Time at which the channel goes to NONE focus.
        std::chrono::steady_clock::time_point timeAtIdle;
    };

    /**
     * Constructs a new Channel object with the provided priority.
     *
     * @param name The channel's name.
     * @param priority The priority of the channel.
     */
    Channel(const std::string& name, const unsigned int priority);

    /**
     * Returns the name of a channel.
     *
     * @return The channel's name.
     */
    const std::string& getName() const;

    /**
     * Returns the priority of a Channel.
     *
     * @return The Channel priority.
     */
    unsigned int getPriority() const;

    /**
     * Updates the focus and notifies the Channel's observer, if there is one, of the focus change. This method does
     * not return until the ChannelObserverInterface##onFocusChanged() callback to the observer returns. If the focus
     * @c NONE, the observer will be removed from the Channel.
     *
     * @param focus The focus of the Channel.
     * @return @c true if focus changed, else @c false.
     */
    bool setFocus(avsCommon::avs::FocusState focus);

    /**
     * Sets a new observer.
     *
     * @param observer The observer of the Channel.
     */
    void setObserver(std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> observer);

    /**
     * Checks whether the Channel has an observer.
     *
     * @return @c true if the Channel has an observer, else @c false.
     */
    bool hasObserver() const;

    /**
     * Compares this Channel and another Channel and checks which is higher priority. A Channel is considered higher
     * priority than another Channel if its m_priority is lower than the other Channel's.
     *
     * @param rhs The Channel to compare with this Channel.
     */
    bool operator>(const Channel& rhs) const;

    /**
     * Updates the AVS interface occupying the Channel.
     *
     * @param interface The name of the interface occupying the Channel.
     */
    void setInterface(const std::string& interface);

    /**
     * Returns the name of the AVS interface occupying the Channel.
     *
     * @return The name of the AVS interface.
     */
    std::string getInterface() const;

    /**
     * Checks whether the observer passed in currently owns the Channel.
     *
     * @param observer The observer to check.
     * @return @c true if the observer currently owns the Channel and @c false otherwise.
     */
    bool doesObserverOwnChannel(std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> observer) const;

    /**
     * Returns the @c State of the @c Channel.
     *
     * @return The @c State.
     */
    Channel::State getState() const;

private:
    /// The priority of the Channel.
    const unsigned int m_priority;

    /// The @c State of the @c Channel.
    State m_state;

    /// The current observer of the Channel.
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> m_observer;
};

}  // namespace afml
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_CHANNEL_H_
