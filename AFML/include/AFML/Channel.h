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

#ifndef ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_CHANNEL_H_
#define ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_CHANNEL_H_

#include <algorithm>
#include <chrono>
#include <list>
#include <memory>
#include <string>

#include <AVSCommon/AVS/ContentType.h>
#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/SDKInterfaces/ChannelObserverInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/Utils/Timing/Timer.h>

namespace alexaClientSDK {
namespace afml {

/**
 * A Channel represents a focusable layer with a priority, allowing the observer which has acquired the Channel to
 * understand focus changes. A Channel can be acquired by multiple Activities, but only a single Activity can be
 * the primary activity. All other Activities that are not the primary Activity will be backgrounded.
 *
 * A Channel can optionally be virtual. Activity changes will not be captured for virtual
 * channels.
 */
class Channel {
public:
    /*
     * This class contains the states of the @c Channel.
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
     * @param isVirtual bool to indicate if this channel is a virtual channel.
     *
     * @note refer to the class level doc on virtual channel paramter.
     */
    Channel(const std::string& name, const unsigned int priority, bool isVirtual = false);

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
     * Updates the focus and notifies all activities associated with the @c Channel of the focus change. If @c
     * FocusState is FOREGROUND, then the most recent Activity added to the Channel will be moved to foreground state,
     * and all other Activites will remain in background state. This method does not return until the
     * ChannelObserverInterface##onFocusChanged() callbacks to all the associated activities return. If the focus is @c
     * NONE, the Activity will be removed from the Channel.
     *
     * @note the @c MixingBehavior should pertain to the primary Activity for the Channel.
     *
     * @param focus The focus of the Channel.
     * @param behavior The MixingBehavior of the Channel.
     * @param forceUpdate bool to indicate if the operation must be forced (irrespective of focus/behavior change).
     * @return @c true if focus changed, else @c false.
     */
    bool setFocus(avsCommon::avs::FocusState focus, avsCommon::avs::MixingBehavior behavior, bool forceUpdate = false);

    /**
     * Set the Primary Activity on the @c Channel given the Activity Object. The function must be called after the
     * correct FocusState of the Channel is set. Any other Activities on the Channel will be moved to the background
     * state.
     *
     * @param activity @c Activity to be set as Primary Activity.
     */
    void setPrimaryActivity(std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity> activity);

    /**
     * Activity to be released by observer.
     *
     * @param observer Channel Observer to release.
     * @return @c true if Activity was found and released, @c false otherwise.
     */
    bool releaseActivity(std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> observer);

    /**
     * Activity to be released by interface.
     *
     * @param interfaceName The interface name of the Activity to release.
     */
    bool releaseActivity(const std::string& interfaceName);

    /**
     * Returns if channel is active.
     * A Channel is considered active if there is any Activity on the Channel.
     *
     * @return @c true if Channel is active, @c false otherwise.
     */
    bool isActive();

    /**
     * Compares this Channel and another Channel and checks which is higher priority. A Channel is considered higher
     * priority than another Channel if its m_priority is lower than the other Channel's.
     *
     * @param rhs The Channel to compare with this Channel.
     */
    bool operator>(const Channel& rhs) const;

    /**
     * Returns the name of the interface of the Primary Activity occupying the Channel.
     *
     * @return The name of the AVS interface.
     */
    std::string getInterface() const;

    /**
     * Returns the @c State of the @c Channel.
     *
     * @return The @c State.
     */
    Channel::State getState() const;

    /**
     * Returns a list of Channel::State updates. This API will internally clear
     * the stored updates when called.
     *
     * @return vector containing all Channel::State Updates
     */
    std::vector<Channel::State> getActivityUpdates();

    /**
     * Returns the primary activity associated with this channel
     *
     * @return the primary activity associated with the channel.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity> getPrimaryActivity();

    /**
     * Gets the activity associated with a given interfaceName.
     *
     * @param interfaceName interface name associated with the activity being searched
     * @return activity associated with @param interfaceName, nullptr if not found.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity> getActivity(
        const std::string& interfaceName);

    /**
     * Retrieve vector of interfaces of all Activites on the Channel.
     */
    std::vector<std::string> getInterfaceList() const;

private:
    /**
     * Notify all the Activities associated with the @c Channel. All activities that are not
     * the primary activity, will be passed MixingBehavior::MUST_PAUSE. The primary activity shall
     * receive the behavior passed as input to the API.
     *
     * @param behavior MixingBehavior to notify the activities with.
     * @param focusState FocusState to notify the activities with.
     */
    void notifyActivities(avsCommon::avs::MixingBehavior behavior, avsCommon::avs::FocusState focusState);

    /**
     * Activity to be released.
     *
     * @param activityToRelease The Activity to be released from the Channel.
     * @return bool true if the operation was successful, otherwise false.
     */
    bool releaseActivityLocked(
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity> activityToRelease);

    /**
     * Helper to remove Activity from the list, also update Channel::State updates as required.
     *
     * @param activity Activity Iterator to remove.
     * @return The @c true if successful and @c false otherwise.
     */
    bool removeActivityHelperLocked(
        std::list<std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity>>::iterator
            activityToRemoveIt);
    /**
     * Adds a Channel State entry to Channel::State updates with current m_state with a specified
     * focusState and interface name.
     *
     * @param interfaceName Interface Name to add to the Channel::State updates.
     * @param focusState FocusState to add to the Channel::State updates.
     */
    void addToChannelUpdatesLocked(const std::string& interfaceName, avsCommon::avs::FocusState focusState);

    /**
     * Given incoming and current activity, process the policies to either release, not release,
     * or start patience timer for the current Activity.
     *
     * @param incomingActivity incoming Activity.
     * @param currentActivity current Activity.
     */
    void processPolicyLocked(
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity> incomingActivity,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity> currentActivity);

    /**
     * Patience Timer Callback. Function is to be executed when patience timer expires.
     *
     * @param activity activity to release after timer is expired.
     */
    void patienceTimerCallback(std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity> activity);

    /**
     * Update Channel Interface with the Activity at top of stack.
     */
    void updateChannelInterfaceLocked();

    /**
     * Returns the primary activity associated with this channel in a locked context.
     *
     * @return the primary activity associated with the channel.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity> getPrimaryActivityLocked() const;

private:
    /// The priority of the Channel.
    const unsigned int m_priority;

    /// Flag to indicate if this is a virtual channel
    bool m_isVirtual;

    /// The @c State of the @c Channel.
    State m_state;

    /// The list to hold shared pointer to all Activities.
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity>> m_activities;

    /// Mutex to protect m_state and m_activities.
    mutable std::mutex m_mutex;

    /// The vector to hold activity updates for the Channel.
    std::vector<Channel::State> m_activityUpdates;

    /// A timer task to trace the patience.
    avsCommon::utils::timing::Timer m_patienceTimer;

    /// Activity to track the initiator of patience.
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity> m_patienceInitiator;

    /// Activity to track the receiver of patience.
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity> m_patienceReceiver;
};
}  // namespace afml
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_CHANNEL_H_
