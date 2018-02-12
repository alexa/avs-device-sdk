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
    /**
     * Constructs a new Channel object with the provided priority.
     *
     * @param priority The priority of the channel.
     */
    Channel(const unsigned int priority);

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
     */
    void setFocus(avsCommon::avs::FocusState focus);

    /**
     * Sets a new observer and notifies the old observer, if there is one, that it lost focus.
     *
     * @param observer The observer of the Channel.
     */
    void setObserver(std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> observer);

    /**
     * Compares this Channel and another Channel and checks which is higher priority. A Channel is considered higher
     * priority than another Channel if its m_priority is lower than the other Channel's.
     *
     * @param rhs The Channel to compare with this Channel.
     */
    bool operator>(const Channel& rhs) const;

    /**
     * Updates the Channel's activity id.
     *
     * @param activityId The activity id of the Channel.
     */
    void setActivityId(const std::string& activityId);

    /**
     * Returns the activity id of the Channel.
     *
     * @return The Channel activity id.
     */
    std::string getActivityId() const;

    /**
     * Notifies the Channel's observer to stop if the @c activityId matches the Channel's activity id.
     *
     * @param activityId The activity id to compare.
     * @return @c true if the activity on the Channel was stopped and @c false otherwise.
     */
    bool stopActivity(const std::string& activityId);

    /**
     * Checks whether the observer passed in currently owns the Channel.
     *
     * @param observer The observer to check.
     * @return @c true if the observer currently owns the Channel and @c false otherwise.
     */
    bool doesObserverOwnChannel(std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> observer) const;

private:
    /// The priority of the Channel.
    const unsigned int m_priority;

    /// The current Focus of the Channel.
    avsCommon::avs::FocusState m_focusState;

    /// The current observer of the Channel.
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> m_observer;

    /// An identifier which should be unique to any activity that can occur on any Channel.
    std::string m_currentActivityId;
};

}  // namespace afml
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_CHANNEL_H_
