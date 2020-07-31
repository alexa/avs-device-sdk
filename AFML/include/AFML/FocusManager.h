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

#ifndef ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_FOCUSMANAGER_H_
#define ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_FOCUSMANAGER_H_

#include <algorithm>
#include <map>
#include <mutex>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <AVSCommon/SDKInterfaces/ChannelObserverInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>

#include "AFML/Channel.h"
#include "AFML/ActivityTrackerInterface.h"
#include "AVSCommon/Utils/Threading/Executor.h"
#include "InterruptModel/InterruptModel.h"

namespace alexaClientSDK {
namespace afml {

/**
 * The FocusManager takes requests to acquire and release Channels and updates the focuses of other Channels based on
 * their priorities so that the invariant that there can only be one Foreground Channel is held. The following
 * operations are provided:
 *
 * acquire Channel - clients should call the acquireChannel() method, passing in the name of the Channel they wish to
 * acquire, a pointer to the observer that they want to be notified once they get focus, and a unique interface name.
 *
 * release Channel - clients should call the releaseChannel() method, passing in the name of the Channel and the
 * observer of the Channel they wish to release.
 *
 * stop foreground Channel - clients should call the stopForegroundActivity() method.
 *
 * All of these methods will notify the observer of the Channel of focus changes via an asynchronous callback to the
 * ChannelObserverInterface##onFocusChanged() method, at which point the client should make a user observable change
 * based on the focus it receives.
 */
class FocusManager : public avsCommon::sdkInterfaces::FocusManagerInterface {
public:
    /**
     * The configuration used by the FocusManager to create Channel objects. Each configuration object has a
     * name and priority.
     */
    class ChannelConfiguration {
    public:
        /**
         * Constructor.
         *
         * @param configName The name of the Channel.
         * @param configPriority The priority of the channel. Lower number priorities result in higher priority
         * Channels. The highest priority number possible is 0.
         */
        ChannelConfiguration(const std::string& configName, unsigned int configPriority) :
                name{configName},
                priority{configPriority} {
        }

        /**
         * Converts the config to a string.
         *
         * @return A string version of the @c ChannelConfiguration data.
         */
        std::string toString() const {
            return "name:'" + name + "', priority:" + std::to_string(priority);
        }

        /// The name of the channel.
        std::string name;

        /// The priority of the channel.
        unsigned int priority;

        /**
         * Get the virtual channel configurations.
         *
         * @param channelTypeKey The key of the virtual channel configuration to get.
         * @param[out] virtualChannelConfiguration The @c ChannelConfiguration for the virtual channels as specified in
         * @c channelTypeKey.  An empty vector if there is no such info on the configuration file.
         * @return true if there's no error, false otherwise.
         */
        static bool readChannelConfiguration(
            const std::string& channelTypeKey,
            std::vector<afml::FocusManager::ChannelConfiguration>* virtualChannelConfigurations);
    };

    /**
     * This constructor creates Channels based on the provided configurations.
     *
     * @param channelConfigurations A vector of @c channelConfiguration objects that will be used to create the
     * Channels. No two Channels should have the same name or priority. If there are multiple configurations with the
     * same name or priority, the latter Channels with that name or priority will not be created.
     * @param activityTrackerInterface The interface to notify the activity tracker a vector of channel states that has
     * been updated.
     * @param virtualChannelConfigurations A vector of @c channelConfiguration objects that will be used to create the
     * Virtual Channels. No two Channels should have the same name or priority. If there are multiple configurations
     * with the same name or priority, the latter Channels with that name or priority will not be created.
     * @param interruptModel @c InterruptModel object that provides MixingBehavior inputs to ChannelObservers upon
     * Focus State Change.
     */
    FocusManager(
        const std::vector<ChannelConfiguration>& channelConfigurations,
        std::shared_ptr<ActivityTrackerInterface> activityTrackerInterface = nullptr,
        const std::vector<ChannelConfiguration>& virtualChannelConfigurations = std::vector<ChannelConfiguration>(),
        std::shared_ptr<interruptModel::InterruptModel> interruptModel = nullptr);

    bool acquireChannel(
        const std::string& channelName,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver,
        const std::string& interfaceName) override;

    bool acquireChannel(
        const std::string& channelName,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity> channelActivity) override;

    std::future<bool> releaseChannel(
        const std::string& channelName,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) override;

    void stopForegroundActivity() override;

    void stopAllActivities() override;

    void addObserver(const std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerObserverInterface>& observer) override;

    void removeObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerObserverInterface>& observer) override;

    void modifyContentType(
        const std::string& channelName,
        const std::string& interfaceName,
        avsCommon::avs::ContentType contentType) override;

    /**
     * Retrieves the default @c ChannelConfiguration for AVS audio channels.
     *
     * @return the default @c ChannelConfiguration for AVS audio channels.
     */
    static const std::vector<FocusManager::ChannelConfiguration> getDefaultAudioChannels();

    /**
     * Retrieves the default @c ChannelConfiguration for AVS visual channels.
     *
     * @return the default @c ChannelConfiguration for AVS visual channels.
     */
    static const std::vector<FocusManager::ChannelConfiguration> getDefaultVisualChannels();

private:
    /// A Map for mapping a @c Channel to its owner.
    using ChannelsToInterfaceNamesMap = std::map<std::shared_ptr<Channel>, std::string>;
    /**
     * Functor so that we can compare Channel objects via shared_ptr.
     */
    struct ChannelPtrComparator {
        /**
         * Compares two shared_ptrs of Channel objects.
         *
         * @param first The first Channel to compare.
         * @param second The second Channel to compare.
         *
         * @return Returns @c true if the first Channel has a higher Channel priority than the second Channel.
         */
        bool operator()(const std::shared_ptr<Channel>& first, const std::shared_ptr<Channel>& second) const {
            return *first > *second;
        }
    };

    /**
     * Helper function to read the @c ChannelConfiguration into @c m_allChannels.  This function also ensures the name
     * and priority for all channels are unique.
     *
     * @param channelConfigurations The @c channelConfigurations of the channels.
     * @param isVirtual Whether the channels are virtual or not.
     */
    void readChannelConfiguration(const std::vector<ChannelConfiguration>& channelConfigurations, bool isVirtual);

    /**
     * Sets the @c FocusState for @c channel and notifies observers of the change.
     *
     * @param channel The @c Channel to set the @c FocusState for.
     * @param focus The @c FocusState to set @c channel to.
     * @param behavior The @c MixingBehavior to set @c channel to.
     * @param forceUpdate optional, if set to true this function will update
     *        activitytracker context (even if focus/behavior did not change).
     */
    void setChannelFocus(
        const std::shared_ptr<Channel>& channel,
        avsCommon::avs::FocusState focus,
        avsCommon::avs::MixingBehavior behavior,
        bool forceUpdate = false);

    /**
     * Grants access to the Channel specified and updates other Channels as needed. This function provides the full
     * implementation which the public method will call.
     *
     * @param channelToAcquire The Channel to acquire.
     * @param channelActivity The Activity to acquire.
     */
    void acquireChannelHelper(
        std::shared_ptr<Channel> channelToAcquire,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity> channelActivity);

    /**
     * Releases the Channel specified and updates other Channels as needed. This function provides the full
     * implementation which the public method will call.
     *
     * @param channelToRelease The Channel to release.
     * @param channelObserver The observer of the Channel to release.
     * @param releaseChannelSuccess The promise to satisfy.
     * @param channelName The name of the Channel.
     */
    void releaseChannelHelper(
        std::shared_ptr<Channel> channelToRelease,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver,
        std::shared_ptr<std::promise<bool>> releaseChannelSuccess,
        const std::string& channelName);

    /**
     * Stops the Channel specified and updates other Channels as needed if the interface name passed in matches the
     * interface occupying the Channel. This function provides the full implementation which the public method will
     * call.
     *
     * @param foregroundChannel The Channel to stop.
     * @param foregroundChannelInterface The name of the interface to stop.
     */
    void stopForegroundActivityHelper(
        std::shared_ptr<Channel> foregroundChannel,
        std::string foregroundChannelInterface);

    /**
     *  Stops all channels specified in @c channelsOwnersMap.
     * A channel will get stopped if it's currently owned by the interface  mapped to the channel
     * in @c channelsOwnersMap.
     * This function provides the full implementation which the public method will
     * call.
     *
     * @param channelsOwnersMap Mapping of channel to owning interfaces
     */
    void stopAllActivitiesHelper(const ChannelsToInterfaceNamesMap& channelsOwnersMap);

    /**
     * Finds the channel from the given channel name.
     *
     * @param channelName The name of the channel to find.
     * @return Return a @c Channel if found or @c nullptr otherwise.
     */
    std::shared_ptr<Channel> getChannel(const std::string& channelName) const;

    /**
     * Gets the currently foregrounded Channel.
     *
     * @return Returns the highest priority active Channel if there is one and @c nullptr otherwise.
     */
    std::shared_ptr<Channel> getHighestPriorityActiveChannelLocked() const;

    /**
     * Checks to see if the provided channel currently has foreground focus or not.
     *
     * @param channel The channel to check.
     * @return Returns @c true if the @c Channel is foreground focused and @c false if it is not.
     */
    bool isChannelForegroundedLocked(const std::shared_ptr<Channel>& channel) const;

    /**
     * Checks to see if the provided Channel name already exists.
     *
     * @param name The Channel name to check.
     *
     * @return Returns @c true if the name is already associated with a Channel and @c false otherwise.
     */
    bool doesChannelNameExist(const std::string& name) const;

    /**
     * Checks to see if the provided Channel priority already exists.
     *
     * @param priority The Channel priority to check.
     *
     * @return Returns @c true if the priority is already associated with a Channel and @c false otherwise.
     */
    bool doesChannelPriorityExist(const unsigned int priority) const;

    /**
     * Foregrounds the highest priority active Channel.
     */
    void foregroundHighestPriorityActiveChannel();

    /**
     * This function is called inside the executor context of @c FocusManager to notify its activityTracker of updates
     * to a set of @c Channel via the @c ActivityTrackerInterface interface.
     */
    void notifyActivityTracker();

    /**
     * Get the mixing behavior for ChannelObserver associated with a low priority channel , when a high priority channel
     * barges in
     *
     * @param lowPrioChannel channel with the lower priority
     * @param highPrioChannel channel with the higher priority
     * @return MixingBehavior to be taken by the ChannelObserver associated with the lowPrioChannel
     */
    avsCommon::avs::MixingBehavior getMixingBehavior(
        std::shared_ptr<Channel> lowPrioChannel,
        std::shared_ptr<Channel> highPrioChannel);

    /**
     * This function determines the mixingBehavior for each backgrounded channel, when the @param foregroundChannel is
     * in Foreground. It also invokes the ChannelObserverInterface::onFocusChanged callback for each backgrounded
     * channel.
     *
     * @param foregroundChannel the channel currently holding foreground focus
     */
    void setBackgroundChannelMixingBehavior(std::shared_ptr<Channel> foregroundChannel);

    /// Mutex used to lock m_activeChannels, m_observers and Channels' interface name.
    std::mutex m_mutex;

    /// Map of channel names to shared_ptrs of Channel objects and contains every channel.
    std::unordered_map<std::string, std::shared_ptr<Channel>> m_allChannels;

    /// Set of currently observed Channels ordered by Channel priority.
    std::set<std::shared_ptr<Channel>, ChannelPtrComparator> m_activeChannels;

    /// The set of observers to notify about focus changes.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerObserverInterface>> m_observers;

    /*
     * A vector of channel's State that has been updated due to @c acquireChannel(), @c releaseChannel() or
     * stopForegroundActivity().  This is accessed by functions in the @c m_executor worker thread, and do not require
     * any synchronization.
     */
    std::vector<Channel::State> m_activityUpdates;

    /// The interface to notify its activity tracker of any changes to its channels.
    std::shared_ptr<ActivityTrackerInterface> m_activityTracker;

    /// The interrupt Model associated with the focus manager
    std::shared_ptr<interruptModel::InterruptModel> m_interruptModel;

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace afml
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_FOCUSMANAGER_H_
