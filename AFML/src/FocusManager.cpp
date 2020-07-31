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

#include "AFML/FocusManager.h"
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace afml {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::avs;
using namespace interruptModel;

/// String to identify log entries originating from this file.
static const std::string TAG("FocusManager");

/// Key for @c FocusManager configurations in configuration node.
static const std::string VIRTUAL_CHANNELS_CONFIG_KEY = "virtualChannels";

/// Key for the name of the channel in configuration node.
static const std::string CHANNEL_NAME_KEY = "name";

/// Key for the priority of the channel in configuration node.
static const std::string CHANNEL_PRIORITY_KEY = "priority";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

FocusManager::FocusManager(
    const std::vector<ChannelConfiguration>& channelConfigurations,
    std::shared_ptr<ActivityTrackerInterface> activityTrackerInterface,
    const std::vector<ChannelConfiguration>& virtualChannelConfigurations,
    std::shared_ptr<InterruptModel> interruptModel) :
        m_activityTracker{activityTrackerInterface},
        m_interruptModel{interruptModel} {
    // Read AVS channel configurations.
    readChannelConfiguration(channelConfigurations, false);
    // Read virtual channel configurations.
    readChannelConfiguration(virtualChannelConfigurations, true);
}

bool FocusManager::acquireChannel(
    const std::string& channelName,
    std::shared_ptr<ChannelObserverInterface> channelObserver,
    const std::string& interfaceName) {
    ACSDK_DEBUG1(LX("acquireChannel").d("channelName", channelName).d("interface", interfaceName));
    std::shared_ptr<Channel> channelToAcquire = getChannel(channelName);
    if (!channelToAcquire) {
        ACSDK_ERROR(LX("acquireChannelFailed").d("reason", "channelNotFound").d("channelName", channelName));
        return false;
    }

    auto channelActivity = FocusManagerInterface::Activity::create(interfaceName, channelObserver);
    if (!channelActivity) {
        ACSDK_ERROR(LX("acquireChannelFailed").d("reason", "failedToCreateActivity").d("interface", interfaceName));
        return false;
    }

    m_executor.submit(
        [this, channelToAcquire, channelActivity]() { acquireChannelHelper(channelToAcquire, channelActivity); });
    return true;
}

bool FocusManager::acquireChannel(
    const std::string& channelName,
    std::shared_ptr<FocusManagerInterface::Activity> channelActivity) {
    ACSDK_DEBUG1(LX("acquireChannel").d("channelName", channelName).d("interface", channelActivity->getInterface()));
    std::shared_ptr<Channel> channelToAcquire = getChannel(channelName);
    if (!channelToAcquire) {
        ACSDK_ERROR(LX("acquireChannelFailed").d("reason", "channelNotFound").d("channelName", channelName));
        return false;
    }

    if (!channelActivity) {
        ACSDK_ERROR(LX("acquireChannelFailed").d("reason", "channelActivityIsNull"));
        return false;
    }

    m_executor.submit(
        [this, channelToAcquire, channelActivity]() { acquireChannelHelper(channelToAcquire, channelActivity); });
    return true;
}

std::future<bool> FocusManager::releaseChannel(
    const std::string& channelName,
    std::shared_ptr<ChannelObserverInterface> channelObserver) {
    ACSDK_DEBUG1(LX("releaseChannel").d("channelName", channelName));

    // Using a shared_ptr here so that the promise stays in scope by the time the Executor picks up the task.
    auto releaseChannelSuccess = std::make_shared<std::promise<bool>>();
    std::future<bool> returnValue = releaseChannelSuccess->get_future();
    std::shared_ptr<Channel> channelToRelease = getChannel(channelName);
    if (!channelToRelease) {
        ACSDK_ERROR(LX("releaseChannelFailed").d("reason", "channelNotFound").d("channelName", channelName));
        releaseChannelSuccess->set_value(false);
        return returnValue;
    }

    m_executor.submit([this, channelToRelease, channelObserver, releaseChannelSuccess, channelName]() {
        releaseChannelHelper(channelToRelease, channelObserver, releaseChannelSuccess, channelName);
    });

    return returnValue;
}

void FocusManager::stopForegroundActivity() {
    // We lock these variables so that we can correctly capture the currently foregrounded channel and activity.
    std::unique_lock<std::mutex> lock(m_mutex);
    std::shared_ptr<Channel> foregroundChannel = getHighestPriorityActiveChannelLocked();
    if (!foregroundChannel) {
        ACSDK_DEBUG(LX("stopForegroundActivityFailed").d("reason", "noForegroundActivity"));
        return;
    }

    std::string foregroundChannelInterface = foregroundChannel->getInterface();
    lock.unlock();

    m_executor.submitToFront([this, foregroundChannel, foregroundChannelInterface]() {
        stopForegroundActivityHelper(foregroundChannel, foregroundChannelInterface);
    });
}

void FocusManager::stopAllActivities() {
    ACSDK_DEBUG5(LX(__func__));

    if (m_activeChannels.empty()) {
        ACSDK_DEBUG5(LX(__func__).m("no active channels"));
        return;
    }

    ChannelsToInterfaceNamesMap channelOwnersCapture;
    std::unique_lock<std::mutex> lock(m_mutex);

    for (const auto& channel : m_activeChannels) {
        for (const auto& interfaceName : channel->getInterfaceList()) {
            channelOwnersCapture.insert(std::pair<std::shared_ptr<Channel>, std::string>(channel, interfaceName));
        }
    }

    lock.unlock();

    m_executor.submitToFront([this, channelOwnersCapture]() { stopAllActivitiesHelper(channelOwnersCapture); });
}

void FocusManager::addObserver(const std::shared_ptr<FocusManagerObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observers.insert(observer);
}

void FocusManager::removeObserver(const std::shared_ptr<FocusManagerObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observers.erase(observer);
}

void FocusManager::readChannelConfiguration(
    const std::vector<ChannelConfiguration>& channelConfigurations,
    bool isVirtual) {
    for (const auto& config : channelConfigurations) {
        if (doesChannelNameExist(config.name)) {
            ACSDK_ERROR(
                LX("readChannelConfigurationFailed").d("reason", "channelNameExists").d("config", config.toString()));
            continue;
        }
        if (doesChannelPriorityExist(config.priority)) {
            ACSDK_ERROR(LX("readChannelConfigurationFailed")
                            .d("reason", "channelPriorityExists")
                            .d("config", config.toString()));
            continue;
        }

        auto channel = std::make_shared<Channel>(config.name, config.priority, isVirtual);
        m_allChannels.insert({config.name, channel});
    }
}

void FocusManager::setChannelFocus(
    const std::shared_ptr<Channel>& channel,
    FocusState focus,
    MixingBehavior behavior,
    bool forceUpdate) {
    if (!channel->setFocus(focus, behavior, forceUpdate)) {
        return;
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    // take a copy of the observers
    auto observers = m_observers;
    lock.unlock();

    // inform copy of the observers in an unlocked content
    for (auto& observer : observers) {
        observer->onFocusChanged(channel->getName(), focus);
    }
}

MixingBehavior FocusManager::getMixingBehavior(
    std::shared_ptr<Channel> lowPrioChannel,
    std::shared_ptr<Channel> highPrioChannel) {
    if (!m_interruptModel) {
        ACSDK_ERROR(LX(__func__).m("Null InterruptModel"));
        return MixingBehavior::UNDEFINED;
    }

    if (!lowPrioChannel || !highPrioChannel) {
        ACSDK_ERROR(LX("getMixingBehaviorFailed").d("reason", "nullInputChannels"));
        return MixingBehavior::UNDEFINED;
    }

    if (*lowPrioChannel > *highPrioChannel) {
        ACSDK_ERROR(LX("getMixingBehaviorFailed")
                        .d("reason", "Priorities of input channels violate API contract")
                        .d("lowPrioChannel priority", lowPrioChannel->getPriority())
                        .d("highPrioChannel priority", highPrioChannel->getPriority()));
        return MixingBehavior::UNDEFINED;
    }

    auto lowPrioChannelName = lowPrioChannel->getName();
    auto lowPrioChannelPrimaryActivity = lowPrioChannel->getPrimaryActivity();
    if (!lowPrioChannelPrimaryActivity) {
        ACSDK_ERROR(LX("getMixingBehaviorFailed").d("No PrimaryActivity on lowPrioChannel", lowPrioChannelName));
        return MixingBehavior::UNDEFINED;
    }

    auto highPrioChannelName = highPrioChannel->getName();
    auto highPrioChannelPrimaryActivity = highPrioChannel->getPrimaryActivity();
    if (!highPrioChannelPrimaryActivity) {
        ACSDK_ERROR(LX("getMixingBehaviorFailed").d("No PrimaryActivity on highPrioChannel", highPrioChannelName));
        return MixingBehavior::UNDEFINED;
    }

    return m_interruptModel->getMixingBehavior(
        lowPrioChannelName,
        lowPrioChannelPrimaryActivity->getContentType(),
        highPrioChannelName,
        highPrioChannelPrimaryActivity->getContentType());
}

void FocusManager::acquireChannelHelper(
    std::shared_ptr<Channel> channelToAcquire,
    std::shared_ptr<FocusManagerInterface::Activity> channelActivity) {
    // Lock here to update internal state which stopForegroundActivity may concurrently access.
    std::unique_lock<std::mutex> lock(m_mutex);
    std::shared_ptr<Channel> foregroundChannel = getHighestPriorityActiveChannelLocked();
    // insert the incoming channel
    m_activeChannels.insert(channelToAcquire);
    lock.unlock();

    ACSDK_DEBUG5(LX(__func__)
                     .d("incomingChannel", channelToAcquire->getName())
                     .d("incomingInterface", channelActivity->getInterface()));

    // attach Activity to the Channel
    channelToAcquire->setPrimaryActivity(std::move(channelActivity));

    if (!foregroundChannel) {
        // channelToAcquire is the only active channel
        setChannelFocus(channelToAcquire, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    } else if (foregroundChannel == channelToAcquire) {
        // acquireChannel request is for the same channel as the current foreground channel
        // NOTE : the primaryActivity interface may change , even though focus state has not changed for the channel
        setChannelFocus(channelToAcquire, FocusState::FOREGROUND, MixingBehavior::PRIMARY, true);
    } else if (*channelToAcquire > *foregroundChannel) {
        // channelToAcquire will now become the foreground channel, other channels shall be backgrounded
        // For each background channel : consult interrupt model to determine the mixability
        setBackgroundChannelMixingBehavior(channelToAcquire);

        // set channelToAcquire as Foreground
        setChannelFocus(channelToAcquire, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    } else {
        // channelToAcquire is to be backgrounded
        auto mixingBehavior = getMixingBehavior(channelToAcquire, foregroundChannel);
        setChannelFocus(channelToAcquire, FocusState::BACKGROUND, mixingBehavior);
    }

    notifyActivityTracker();
}

void FocusManager::releaseChannelHelper(
    std::shared_ptr<Channel> channelToRelease,
    std::shared_ptr<ChannelObserverInterface> channelObserver,
    std::shared_ptr<std::promise<bool>> releaseChannelSuccess,
    const std::string& name) {
    ACSDK_DEBUG5(LX(__func__).d("channelToRelease", channelToRelease->getName()));

    bool success = channelToRelease->releaseActivity(std::move(channelObserver));
    releaseChannelSuccess->set_value(success);

    if (!success) {
        ACSDK_ERROR(LX(__func__)
                        .d("reason", "releaseActivityFailed")
                        .d("channel", channelToRelease)
                        .d("interface", channelToRelease->getInterface()));
        return;
    }

    // Only release and set entire channel focus to NONE if there are no active Activity remaining.
    if (!channelToRelease->isActive()) {
        // Lock here to update internal state which stopForegroundActivity may concurrently access.
        std::unique_lock<std::mutex> lock(m_mutex);
        m_activeChannels.erase(channelToRelease);
        lock.unlock();
        setChannelFocus(channelToRelease, FocusState::NONE, MixingBehavior::MUST_STOP);
    }
    foregroundHighestPriorityActiveChannel();
    notifyActivityTracker();
}

void FocusManager::stopForegroundActivityHelper(
    std::shared_ptr<Channel> foregroundChannel,
    std::string foregroundChannelInterface) {
    if (foregroundChannelInterface != foregroundChannel->getInterface()) {
        return;
    }
    ACSDK_DEBUG5(LX(__func__).d("interface", foregroundChannelInterface));
    bool success = foregroundChannel->releaseActivity(foregroundChannel->getInterface());
    if (!success) {
        ACSDK_ERROR(LX(__func__)
                        .d("reason", "releaseActivityFailed")
                        .d("channel", foregroundChannel)
                        .d("interface", foregroundChannel->getInterface()));
    }

    // Only release and set entire channel focus to NONE if there are no active Activity remaining.
    if (!foregroundChannel->isActive()) {
        ACSDK_DEBUG1(LX(__func__).m("Channel is not active ... releasing"));
        // Lock here to update internal state which stopForegroundActivity may concurrently access.
        std::unique_lock<std::mutex> lock(m_mutex);
        m_activeChannels.erase(foregroundChannel);
        lock.unlock();
        setChannelFocus(foregroundChannel, FocusState::NONE, MixingBehavior::MUST_STOP);
    }
    foregroundHighestPriorityActiveChannel();
    notifyActivityTracker();
}

void FocusManager::stopAllActivitiesHelper(const ChannelsToInterfaceNamesMap& channelsOwnersMap) {
    ACSDK_DEBUG3(LX(__func__));

    std::set<std::shared_ptr<Channel>> channelsToClear;

    std::unique_lock<std::mutex> lock(m_mutex);

    for (const auto& channelAndInterface : channelsOwnersMap) {
        auto channel = channelAndInterface.first;
        auto interfaceName = channelAndInterface.second;
        ACSDK_DEBUG3(LX(__func__).d("channel", channel).d("interface", interfaceName));

        bool success = channel->releaseActivity(channelAndInterface.second);
        if (!success) {
            ACSDK_ERROR(
                LX(__func__).d("reason", "releaseActivityFailed").d("channel", channel).d("interface", interfaceName));
        }
        channelsToClear.insert(channel);
    }

    lock.unlock();

    for (const auto& channel : channelsToClear) {
        // Only release and set entire channel focus to NONE if there are no active Activity remaining.
        if (!channel->isActive()) {
            // Lock here to update internal state which stopForegroundActivity may concurrently access.
            std::unique_lock<std::mutex> lock(m_mutex);
            m_activeChannels.erase(channel);
            lock.unlock();
            setChannelFocus(channel, FocusState::NONE, MixingBehavior::MUST_STOP);
        }
    }
    foregroundHighestPriorityActiveChannel();
    notifyActivityTracker();
}

std::shared_ptr<Channel> FocusManager::getChannel(const std::string& channelName) const {
    auto search = m_allChannels.find(channelName);
    if (search != m_allChannels.end()) {
        return search->second;
    }
    return nullptr;
}

std::shared_ptr<Channel> FocusManager::getHighestPriorityActiveChannelLocked() const {
    if (m_activeChannels.empty()) {
        return nullptr;
    }
    return *m_activeChannels.begin();
}

bool FocusManager::isChannelForegroundedLocked(const std::shared_ptr<Channel>& channel) const {
    return getHighestPriorityActiveChannelLocked() == channel;
}

bool FocusManager::doesChannelNameExist(const std::string& name) const {
    return m_allChannels.find(name) != m_allChannels.end();
}

bool FocusManager::doesChannelPriorityExist(const unsigned int priority) const {
    for (const auto& m_allChannel : m_allChannels) {
        if (m_allChannel.second->getPriority() == priority) {
            return true;
        }
    }
    return false;
}

void FocusManager::modifyContentType(
    const std::string& channelName,
    const std::string& interfaceName,
    ContentType contentType) {
    // find the channel
    auto channel = getChannel(channelName);
    if (!channel) {
        ACSDK_ERROR(LX("modifyContentTypeFailed").d("reason", "channelNotFound").d("channel", channelName));
        return;
    }

    // find the activity associated with the interfacename in the channel
    auto activity = channel->getActivity(interfaceName);
    if (!activity) {
        ACSDK_ERROR(LX("modifyContentTypeFailed").d("no activity found associated with interfaceName", interfaceName));
        return;
    }

    if (contentType == activity->getContentType()) {
        ACSDK_WARN(LX("modifyContentTypeFailed").d("no contentType to modify it is already identical: ", contentType));
        return;
    }

    // modify the contentType associated with the activity
    activity->setContentType(contentType);

    // reconsult the InterruptModel and set the new MixingBehaviors for all backgrounded channelobservers
    std::unique_lock<std::mutex> lock(m_mutex);
    std::shared_ptr<Channel> foregroundChannel = getHighestPriorityActiveChannelLocked();
    lock.unlock();
    setBackgroundChannelMixingBehavior(foregroundChannel);
}

void FocusManager::setBackgroundChannelMixingBehavior(std::shared_ptr<Channel> foregroundChannel) {
    std::unique_lock<std::mutex> lock(m_mutex);

    auto channelIter = m_activeChannels.find(foregroundChannel);
    if (channelIter == m_activeChannels.end()) {
        ACSDK_ERROR(
            LX("setBackgroundChannelMixingBehaviorFailed").d("Could not find channel", foregroundChannel->getName()));
        return;
    }
    // skip to the next channel in priority
    channelIter++;

    for (; channelIter != m_activeChannels.end(); channelIter++) {
        // determine mixingBehavior for each background channel
        auto mixingBehavior = getMixingBehavior(*channelIter, foregroundChannel);
        lock.unlock();
        setChannelFocus(*channelIter, FocusState::BACKGROUND, mixingBehavior);
        lock.lock();
    }
}

void FocusManager::foregroundHighestPriorityActiveChannel() {
    std::unique_lock<std::mutex> lock(m_mutex);
    std::shared_ptr<Channel> channelToForeground = getHighestPriorityActiveChannelLocked();
    lock.unlock();

    if (channelToForeground) {
        // inform background channels of the new MixingBehavior as per the new Foreground Channel
        setBackgroundChannelMixingBehavior(channelToForeground);

        // Foreground the highest priority channel
        setChannelFocus(channelToForeground, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    }
}

void FocusManager::notifyActivityTracker() {
    std::unique_lock<std::mutex> lock(m_mutex);
    for (const auto& channel : m_allChannels) {
        auto activityUpdates = channel.second->getActivityUpdates();
        for (const auto& activity : activityUpdates) {
            m_activityUpdates.push_back(activity);
            ACSDK_DEBUG1(LX(__func__)
                             .d("name", activity.name)
                             .d("interfaceName", activity.interfaceName)
                             .d("focusState", activity.focusState));
        }
    }
    lock.unlock();

    if (m_activityTracker && !m_activityUpdates.empty()) {
        m_activityTracker->notifyOfActivityUpdates(m_activityUpdates);
    }
    m_activityUpdates.clear();
}

const std::vector<FocusManager::ChannelConfiguration> FocusManager::getDefaultAudioChannels() {
    static const std::vector<FocusManager::ChannelConfiguration> defaultAudioChannels = {
        {FocusManagerInterface::DIALOG_CHANNEL_NAME, FocusManagerInterface::DIALOG_CHANNEL_PRIORITY},
        {FocusManagerInterface::ALERT_CHANNEL_NAME, FocusManagerInterface::ALERT_CHANNEL_PRIORITY},
        {FocusManagerInterface::COMMUNICATIONS_CHANNEL_NAME, FocusManagerInterface::COMMUNICATIONS_CHANNEL_PRIORITY},
        {FocusManagerInterface::CONTENT_CHANNEL_NAME, FocusManagerInterface::CONTENT_CHANNEL_PRIORITY}};

    return defaultAudioChannels;
}

const std::vector<FocusManager::ChannelConfiguration> FocusManager::getDefaultVisualChannels() {
    static const std::vector<FocusManager::ChannelConfiguration> defaultVisualChannels = {
        {FocusManagerInterface::VISUAL_CHANNEL_NAME, FocusManagerInterface::VISUAL_CHANNEL_PRIORITY}};

    return defaultVisualChannels;
}

bool afml::FocusManager::ChannelConfiguration::readChannelConfiguration(
    const std::string& channelTypeKey,
    std::vector<afml::FocusManager::ChannelConfiguration>* virtualChannelConfigurations) {
    if (!virtualChannelConfigurations) {
        ACSDK_ERROR(LX("readChannelConfigurationFailed").d("reason", "nullVirtualChannelConfiguration"));
        return false;
    }

    auto configRoot =
        alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode::getRoot()[VIRTUAL_CHANNELS_CONFIG_KEY];
    if (!configRoot) {
        ACSDK_DEBUG9(LX(__func__).m("noConfigurationRoot"));
        return true;
    }

    bool returnValue = true;
    auto channelArray = configRoot.getArray(channelTypeKey);
    if (!channelArray) {
        ACSDK_DEBUG9(LX(__func__).d("key", channelTypeKey).m("keyNotFoundOrNotAnArray"));
    } else {
        for (std::size_t i = 0; i < channelArray.getArraySize(); i++) {
            auto elem = channelArray[i];
            if (!elem) {
                ACSDK_ERROR(LX("readChannelConfigurationFailed").d("reason", "noNameKey"));
                returnValue = false;
                break;
            }

            std::string name;
            if (!elem.getString(CHANNEL_NAME_KEY, &name)) {
                ACSDK_ERROR(LX("readChannelConfigurationFailed").d("reason", "noNameKey"));
                returnValue = false;
                break;
            }

            int priority = 0;
            if (!elem.getInt(CHANNEL_PRIORITY_KEY, &priority)) {
                ACSDK_ERROR(LX("readChannelConfigurationFailed").d("reason", "noPriorityKey"));
                returnValue = false;
                break;
            }
            if (priority < 0) {
                ACSDK_ERROR(LX("ChannelConfigurationFailed").d("reason", "invalidPriority").d("priority", priority));
                returnValue = false;
                break;
            }

            afml::FocusManager::ChannelConfiguration channelConfig{name, static_cast<unsigned int>(priority)};
            virtualChannelConfigurations->push_back(channelConfig);
        }
    }
    return returnValue;
}

}  // namespace afml
}  // namespace alexaClientSDK
