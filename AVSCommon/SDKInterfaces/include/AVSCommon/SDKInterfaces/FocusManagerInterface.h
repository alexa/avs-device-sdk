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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_FOCUSMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_FOCUSMANAGERINTERFACE_H_

#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <AVSCommon/AVS/ContentType.h>
#include "ChannelObserverInterface.h"
#include "FocusManagerObserverInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * A FocusManager takes requests to acquire and release Channels and updates the focuses of other Channels based on
 * their priorities so that the invariant that there can only be one Foreground Channel is held. The following
 * operations are provided:
 *
 * acquire Channel - clients should call the acquireChannel() method, passing in the name of the Channel they wish to
 * acquire, a pointer to the observer that they want to be notified once they get focus, and a unique interface name.
 * clients could alternatively construct an @c Activity object and pass that along with the channel name to acquire the
 * channel.
 *
 * release Channel - clients should call the releaseChannel() method, passing in the name of the Channel and the
 * observer of the Channel they wish to release.
 *
 * stop foreground Channel - clients should call the stopForegroundActivity() method.
 *
 * stop all activities - client should call stopAllActivities
 *
 * All of these methods will notify the observer of the Channel of focus changes via an asynchronous callback to the
 * ChannelObserverInterface##onFocusChanged() method, at which point the client should make a user observable change
 * based on the focus it receives.
 */
class FocusManagerInterface {
public:
    /**
     * An activity representation of an entity that includes details of policy and patience duration that can acquire
     * a channel.
     *
     * If activity A has a patience duration greater than 0, and pushes the current activity B to background,
     * activity B is eligible to be reinstated as foreground if activity A releases the channel before the duration
     * of the patience has lapsed.
     */
    class Activity {
    public:
        /**
         * Constructs a new Activity object.
         *
         * @param interfaceName The Activity's interface.
         * @param channelObserver The Activity's Channel Observer.
         * @param patienceDuration The Activity's Patience Duration.
         * @param contentType The Activity's Content Type.
         */
        static std::shared_ptr<Activity> create(
            const std::string& interfaceName,
            const std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface>& channelObserver,
            const std::chrono::milliseconds& patienceDuration = std::chrono::milliseconds::zero(),
            const avsCommon::avs::ContentType contentType = avsCommon::avs::ContentType::NONMIXABLE);

        bool operator==(const Activity& rhs) {
            return this->m_interface == rhs.m_interface;
        }

        /**
         * Returns the name of Activity's AVS interface.
         *
         * @return The name of the AVS interface.
         */
        const std::string getInterface() const;

        /**
         * Returns the patience duration in milliseconds of Activity. After the release duration, the
         * backgrounded Activity due to the forgrounding of the Activity with patience will be kicked
         * out of the stack and will be set to NONE FocusState.
         *
         * @return The patience duration in milliseconds of Activity.
         */
        std::chrono::milliseconds getPatienceDuration() const;

        /**
         * Returns the @c ContentType associated with the @c Activity.
         *
         * @return The @c ContentType associated with this @c Activity.
         */
        avsCommon::avs::ContentType getContentType() const;

        /**
         * Sets the @c ContentType associated with this @c Activity.
         *
         * @param contentType The @c ContentType associated with this @c Activity.
         */
        void setContentType(avsCommon::avs::ContentType contentType);

        /**
         * Gets the last @c MixingBehavior set for this Activity.
         *
         * @param The @c MixingBehavior to be set for this @c Activity
         */
        avsCommon::avs::MixingBehavior getMixingBehavior() const;

        /**
         * Returns the channel observer of Activity.
         *
         * @return The channel observer of Activity.
         */
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> getChannelObserver() const;

        /**
         * Notifies the channel Observer of focus of Channel and Channel owner interface.
         *
         * @return @c true if observer was notified, else @c false.
         */
        bool notifyObserver(avs::FocusState focus, avsCommon::avs::MixingBehavior behavior);

    private:
        /**
         * Constructs a new Activity object.
         *
         * @param interfaceName The Activity's interface.
         * @param patience The Activity's Patience Duration.
         * @param channelObserver The Activity's Channel Observer.
         * @param contentType The Activity's Content Type.
         */
        Activity(
            const std::string& interfaceName,
            const std::chrono::milliseconds& patienceDuration,
            const std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface>& channelObserver,
            const avsCommon::avs::ContentType contentType) :
                m_interface{interfaceName},
                m_patienceDuration{patienceDuration},
                m_channelObserver{channelObserver},
                m_contentType{contentType},
                m_mixingBehavior{avsCommon::avs::MixingBehavior::UNDEFINED} {
        }

        /**
         * Sets the @c MixingBehavior for this @c Activity
         *
         * @param behavior The @c MixingBehavior to be set for this @c Activity.
         */
        void setMixingBehavior(avsCommon::avs::MixingBehavior behavior);

        // The mutex that synchronizes all operations within the activity
        mutable std::mutex m_mutex;

        // The interface name of the Activity.
        const std::string m_interface;

        // The duration of patience in milliseconds of the Activity.
        const std::chrono::milliseconds m_patienceDuration;

        // The channel observer of the Activity.
        const std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> m_channelObserver;

        // The ContentType associated with this Activity
        avsCommon::avs::ContentType m_contentType;

        // Last MixingBehavior associated with this activity
        avsCommon::avs::MixingBehavior m_mixingBehavior;
    };

    /// The default Dialog Channel name.
    static constexpr const char* DIALOG_CHANNEL_NAME = "Dialog";

    /// The default dialog Channel priority.
    static constexpr unsigned int DIALOG_CHANNEL_PRIORITY = 100;

    /// The default Communications Channel name.
    static constexpr const char* COMMUNICATIONS_CHANNEL_NAME = "Communications";

    /// The default Communications Channel priority.
    static constexpr unsigned int COMMUNICATIONS_CHANNEL_PRIORITY = 150;

    /// The default Alert Channel name.
    static constexpr const char* ALERT_CHANNEL_NAME = "Alert";

    /// The default Alert Channel priority.
    static constexpr unsigned int ALERT_CHANNEL_PRIORITY = 200;

    /// The default Content Channel name.
    static constexpr const char* CONTENT_CHANNEL_NAME = "Content";

    /// The default Content Channel priority.
    static constexpr unsigned int CONTENT_CHANNEL_PRIORITY = 300;

    /// The default Visual Channel name.
    static constexpr const char* VISUAL_CHANNEL_NAME = "Visual";

    /// The default Visual Channel priority.
    static constexpr unsigned int VISUAL_CHANNEL_PRIORITY = 100;

    /// Destructor.
    virtual ~FocusManagerInterface() = default;

    /**
     * This method will acquire the channel and grant the appropriate focus to it and other channels if needed. The
     * caller will be notified via an ChannelObserverInterface##onFocusChanged() call to the @c channelObserver when
     * it can start the activity.
     *
     * @param channelName The name of the Channel to acquire.
     * @param channelObserver The observer that will be acquiring the Channel and be notified of focus changes.
     * @param interfaceName The name of the AVS interface occupying the Channel. This should be unique and represents
     * the name of the AVS interface using the Channel.  The name of the AVS interface is used by the ActivityTracker to
     * send Context to AVS.
     *
     * @return Returns @c true if the Channel can be acquired and @c false otherwise.
     */
    virtual bool acquireChannel(
        const std::string& channelName,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver,
        const std::string& interfaceName) = 0;

    /**
     * This method will acquire the channel and grant the appropriate focus to it and other channels if needed. The
     * caller will be notified via an ChannelObserverInterface##onFocusChanged() call to the @c channelObserver when
     * it can start the activity.
     *
     * @param channelName The name of the Channel to acquire.
     * @param channelActivity Activity object associated with the Channel.
     *
     * @return Returns @c true if the Channel can be acquired and @c false otherwise.
     */
    virtual bool acquireChannel(
        const std::string& channelName,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface::Activity> channelActivity) = 0;

    /**
     * This method will release the Channel and notify the observer of the Channel, if the observer is the same as the
     * observer passed in the acquireChannel call, to stop via ChannelObserverInterface##onFocusChanged(). If the
     * Channel to release is the current foreground focused Channel, it will also notify the next highest priority
     * Channel via an ChannelObserverInterface##onFocusChanged() callback that it has gained foreground focus.
     *
     * @param channelName The name of the Channel to release.
     * @param channelObserver The observer to be released from the Channel.
     * @return @c std::future<bool> which will contain @c true if the Channel can be released and @c false otherwise.
     */
    virtual std::future<bool> releaseChannel(
        const std::string& channelName,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) = 0;

    /**
     * This method will request that the currently foregrounded Channel activity be stopped, if there is one. This will
     * be performed asynchronously, and so, if at the time of processing, the activity has stopped for any reason, then
     * no stop will be performed. If something was stopped, the next highest priority active Channel will be brought
     * to the foreground.
     */
    virtual void stopForegroundActivity() = 0;

    /**
     *  This method will request to stop all active channels. This will be performed asynchronously, and so, if at the
     *  time performing the stop, the channel is owned by another interface, this channel won't get stopped.
     */
    virtual void stopAllActivities() = 0;

    /**
     * Add an observer to the focus manager.
     *
     * @param observer The observer to add.
     */
    virtual void addObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerObserverInterface>& observer) = 0;

    /**
     * Remove an observer from the focus manager.
     *
     * @param observer The observer to remove.
     */
    virtual void removeObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerObserverInterface>& observer) = 0;

    /**
     * This function allows ChannelObservers to modify the ContentType rendering on their associated Activity
     * This will cause the focus manager to reconsult the interruptModel in order to determine the new MixingBehavior
     * for all backgrounded channels.
     *
     * @param channelName the channel associated with the ChannelObserver
     * @param interfaceName the interface name associated with the ChannelObserver
     * @param contentType the new content type
     */
    virtual void modifyContentType(
        const std::string& channelName,
        const std::string& interfaceName,
        avsCommon::avs::ContentType contentType) = 0;
};

inline std::shared_ptr<FocusManagerInterface::Activity> FocusManagerInterface::Activity::create(
    const std::string& interfaceName,
    const std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface>& channelObserver,
    const std::chrono::milliseconds& patienceDuration,
    const avsCommon::avs::ContentType contentType) {
    if (interfaceName.empty() || patienceDuration.count() < 0 || channelObserver == nullptr) {
        return nullptr;
    }

    auto activity = std::shared_ptr<FocusManagerInterface::Activity>(
        new FocusManagerInterface::Activity(interfaceName, patienceDuration, channelObserver, contentType));
    return activity;
}

inline const std::string FocusManagerInterface::Activity::getInterface() const {
    return m_interface;
}

inline std::chrono::milliseconds FocusManagerInterface::Activity::getPatienceDuration() const {
    return m_patienceDuration;
}

inline avsCommon::avs::ContentType FocusManagerInterface::Activity::getContentType() const {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_contentType;
}

inline void FocusManagerInterface::Activity::setContentType(avsCommon::avs::ContentType contentType) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_contentType = contentType;
}

inline avsCommon::avs::MixingBehavior FocusManagerInterface::Activity::getMixingBehavior() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_mixingBehavior;
}

inline std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> FocusManagerInterface::Activity::
    getChannelObserver() const {
    return m_channelObserver;
}

inline bool FocusManagerInterface::Activity::notifyObserver(
    avs::FocusState focus,
    avsCommon::avs::MixingBehavior behavior) {
    if (m_channelObserver) {
        avsCommon::avs::MixingBehavior overrideBehavior{behavior};
        // If the activity/channelObserver is already paused , do not duck
        if ((avsCommon::avs::MixingBehavior::MUST_PAUSE == getMixingBehavior()) &&
            (avsCommon::avs::MixingBehavior::MAY_DUCK == behavior)) {
            overrideBehavior = avsCommon::avs::MixingBehavior::MUST_PAUSE;
        }

        m_channelObserver->onFocusChanged(focus, overrideBehavior);
        // Set the current mixing behavior that the observer received.
        setMixingBehavior(overrideBehavior);
        return true;
    }
    return false;
}

inline void FocusManagerInterface::Activity::setMixingBehavior(avsCommon::avs::MixingBehavior behavior) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_mixingBehavior = behavior;
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_FOCUSMANAGERINTERFACE_H_
