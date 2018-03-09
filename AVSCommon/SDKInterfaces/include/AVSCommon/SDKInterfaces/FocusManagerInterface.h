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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_FOCUSMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_FOCUSMANAGERINTERFACE_H_

#include <future>
#include <memory>
#include <string>

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
class FocusManagerInterface {
public:
    /// The default Dialog Channel name.
    static constexpr const char* DIALOG_CHANNEL_NAME = "Dialog";

    /// The default dialog Channel priority.
    static constexpr unsigned int DIALOG_CHANNEL_PRIORITY = 100;

    /// The default Alerts Channel name.
    static constexpr const char* ALERTS_CHANNEL_NAME = "Alerts";

    /// The default Alerts Channel priority.
    static constexpr unsigned int ALERTS_CHANNEL_PRIORITY = 200;

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
     * it can start the activity. If the Channel was already held by a different observer, the observer will be
     * notified via ChannelObserverInterface##onFocusChanged() to stop before letting the new observer start.
     *
     * @param channelName The name of the Channel to acquire.
     * @param channelObserver The observer that will be acquiring the Channel and be notified of focus changes.
     * @param interface The name of the AVS interface occupying the Channel. This should be unique and represents the
     * name of the AVS interface using the Channel.  The name of the AVS interface is used by the ActivityTracker to
     * send Context to AVS.
     *
     * @return Returns @c true if the Channel can be acquired and @c false otherwise.
     */
    virtual bool acquireChannel(
        const std::string& channelName,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver,
        const std::string& interface) = 0;

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
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_FOCUSMANAGERINTERFACE_H_
