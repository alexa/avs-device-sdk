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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEAKERMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEAKERMANAGERINTERFACE_H_

#include <future>
#include <memory>

#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * The @c SpeakerManagerInterface is used to control speaker settings across all @c ChannelVolumeInterface(s)
 * associated with a given @c SpeakerInterface Type.
 */
class SpeakerManagerInterface {
public:
    /**
     * The @c NotificationProperties struct defines the properties about the source that invokes the APIs and
     * how to send volume or mute change notifications. The APIs of @c SpeakerManagerInterface uses the properties
     * to decide whether AVS and observers should be notified or not.
     */
    struct NotificationProperties {
        /**
         * Constructor.
         * @param source Whether the call is a result from an AVS directive or local interaction.
         * @param notifyAVS Whether AVS should be notified or not.
         * @param notifyObservers Whether observers should be notified or not.
         */
        NotificationProperties(
            SpeakerManagerObserverInterface::Source source = SpeakerManagerObserverInterface::Source::LOCAL_API,
            bool notifyAVS = true,
            bool notifyObservers = true) :
                notifyAVS{notifyAVS},
                notifyObservers{notifyObservers},
                source{source} {
        }

        // Setting this to true will ensure AVS to be notified of the change.
        bool notifyAVS;
        // Setting this to true will ensure observers to be notified of the change.
        bool notifyObservers;
        // Whether the call is a result from an AVS directive or local interaction.
        SpeakerManagerObserverInterface::Source source;
    };

    /**
     * Set the volume for ChannelVolumeInterfaces of a certain @c Type.
     *
     * @param type The type of @c ChannelVolumeInterface to modify.
     * @param volume The volume to set. Values must be between [0,100].
     * @param properties Notification properties that specify how the volume change will be notified.
     * @return A future to be set with the operation's result. The future must be checked for validity
     * before attempting to obtain the shared state. An invalid future indicates an internal error,
     * and the caller should not assume the operation was successful.
     */
    virtual std::future<bool> setVolume(
        ChannelVolumeInterface::Type type,
        int8_t volume,
        const NotificationProperties& properties) = 0;

    /**
     * Handle an external volume/mute state event in the system and update the settings.
     *
     * A volume could be changed either using SpeakerManager instance or using any other component
     * which support volume change. In the case, volume on the device is being updated by some
     * other component, this interface could be used to update the speaker settings of the
     * associated @c ChannelVolumeInterface. This interface does not modify/change the volume
     * or mute. It should be used to update speaker settings within SpeakerManager and notify
     * AVS/observers if required of this change.
     *
     * @param type The type of @c ChannelVolumeInterface to retrieve settings for.
     * @param speakerSettings New updated value. Values must be between [0,100]
     * @param properties Notification properties that specify how the volume change will be notified.
     */
    virtual void onExternalSpeakerSettingsUpdate(
        ChannelVolumeInterface::Type type,
        const SpeakerInterface::SpeakerSettings& speakerSettings,
        const NotificationProperties& properties){};

    /**
     * Adjusts the volume for ChannelVolumeInterfaces of a certain @c Type with a volume delta.
     *
     * @param type The type of @c ChannelVolumeInterface to modify.
     * @param delta The delta to modify volume by. Values must be between [-100,100].
     * @param properties Notification properties that specify how the volume change will be notified.
     * @return A future to be set with the operation's result. The future must be checked for validity
     * before attempting to obtain the shared state. An invalid future indicates an internal error,
     * and the caller should not assume the operation was successful.
     */
    virtual std::future<bool> adjustVolume(
        ChannelVolumeInterface::Type type,
        int8_t delta,
        const NotificationProperties& properties) = 0;

    /**
     * Sets the mute for ChannelVolumeInterfaces of a certain @c Type.
     *
     * @param type The type of @c ChannelVolumeInterface to modify.
     * @param mute A boolean indicating mute. true = mute, false = unmute.
     * @param properties Notification properties that specify how the mute change will be notified.
     * @return A future to be set with the operation's result. The future must be checked for validity
     * before attempting to obtain the shared state. An invalid future indicates an internal error,
     * and the caller should not assume the operation was successful.
     */
    virtual std::future<bool> setMute(
        ChannelVolumeInterface::Type type,
        bool mute,
        const NotificationProperties& properties) = 0;

    /**
     * @deprecated
     *
     * Set the volume for ChannelVolumeInterfaces of a certain @c Type.
     *
     * @param type The type of @c ChannelVolumeInterface to modify.
     * @param volume The volume to set. Values must be between [0,100].
     * @param forceNoNotifications Setting this to true will ensure no event is sent and no observer is notified.
     * Setting this to false does not guarantee that a notification will be sent.
     * @param source Whether the call is a result from an AVS directive or local interaction.
     * @return A future to be set with the operation's result. The future must be checked for validity
     * before attempting to obtain the shared state. An invalid future indicates an internal error,
     * and the caller should not assume the operation was successful.
     */
    virtual std::future<bool> setVolume(
        ChannelVolumeInterface::Type type,
        int8_t volume,
        bool forceNoNotifications = false,
        SpeakerManagerObserverInterface::Source source = SpeakerManagerObserverInterface::Source::LOCAL_API) = 0;

    /**
     * @deprecated
     *
     * Adjusts the volume for ChannelVolumeInterfaces of a certain @c Type with a volume delta.
     *
     * @param type The type of @c ChannelVolumeInterface to modify.
     * @param delta The delta to modify volume by. Values must be between [-100,100].
     * @param forceNoNotifications Setting this to true will ensure no event is sent and no observer is notified.
     * Setting this to false does not guarantee that a notification will be sent.
     * @param source Whether the call is a result from an AVS directive or local interaction.
     * @return A future to be set with the operation's result. The future must be checked for validity
     * before attempting to obtain the shared state. An invalid future indicates an internal error,
     * and the caller should not assume the operation was successful.
     */
    virtual std::future<bool> adjustVolume(
        ChannelVolumeInterface::Type type,
        int8_t delta,
        bool forceNoNotifications = false,
        SpeakerManagerObserverInterface::Source source = SpeakerManagerObserverInterface::Source::LOCAL_API) = 0;

    /**
     * @deprecated
     *
     * Sets the mute for ChannelVolumeInterfaces of a certain @c Type.
     *
     * @param type The type of @c ChannelVolumeInterface to modify.
     * @param mute A boolean indicating mute. true = mute, false = unmute.
     * @param forceNoNotifications Setting this to true will ensure no event is sent and no observer is notified.
     * Setting this to false does not guarantee that a notification will be sent.
     * @param source Whether the call is a result from an AVS directive or local interaction.
     * @return A future to be set with the operation's result. The future must be checked for validity
     * before attempting to obtain the shared state. An invalid future indicates an internal error,
     * and the caller should not assume the operation was successful.
     */
    virtual std::future<bool> setMute(
        ChannelVolumeInterface::Type type,
        bool mute,
        bool forceNoNotifications = false,
        SpeakerManagerObserverInterface::Source source = SpeakerManagerObserverInterface::Source::LOCAL_API) = 0;

#ifdef ENABLE_MAXVOLUME_SETTING
    /**
     * Sets maximum volume limit. This function should be called to handle @c setMaximumVolumeLimit
     * directive from AVS.
     *
     * @param maximumVolumeLimit The maximum volume level that ChannelVolumeInterfaces in this system can reach.
     * @note Upon success, previous volume exceeding the new limit will be decreased to be complied with the new limit.
     * @return A future to be set with the operation's result. The future must be checked for validity
     * before attempting to obtain the shared state. An invalid future indicates an internal error,
     * and the caller should not assume the operation was successful.
     */
    virtual std::future<bool> setMaximumVolumeLimit(const int8_t maximumVolumeLimit) = 0;
#endif  // ENABLE_MAXVOLUME_SETTING

    /**
     * Gets the speaker settings.
     *
     * @param type The type of @c ChannelVolumeInterface to retrieve settings for.
     * @param[out] settings The settings if the operation was successful.
     * @return A future to be set with the operation's result. The future must be checked for validity
     * before attempting to obtain the shared state. An invalid future indicates an internal error,
     * and the caller should not assume the operation was successful.
     */
    virtual std::future<bool> getSpeakerSettings(
        ChannelVolumeInterface::Type type,
        SpeakerInterface::SpeakerSettings* settings) = 0;

    /**
     * Adds an observer to be notified when the SpeakerManager changes the @c SpeakerInterface::SpeakerSettings of a
     * any @c ChannelVolumeInterface Objects.
     *
     * @param observer The observer to be notified when the @c SpeakerInterface::SpeakerSettings of a group successfully
     * changes.
     */
    virtual void addSpeakerManagerObserver(std::shared_ptr<SpeakerManagerObserverInterface> observer) = 0;

    /**
     * Removes an observer from being notified when the SpeakerManager changes the @c SpeakerInterface::SpeakerSettings
     * of any @c ChannelVolumeInterface Objects.
     *
     * @param observer The observer to be notified when the @c SpeakerInterface::SpeakerSettings of a group successfully
     * changes.
     */
    virtual void removeSpeakerManagerObserver(std::shared_ptr<SpeakerManagerObserverInterface> observer) = 0;

    /**
     * Adds a @c ChannelVolumeInterface object to be tracked by @c SpeakerManagerInterface.
     * This method is not guaranteed to be thread safe and should be called during the initialization step only.
     * All ChannelVolumeInterfaces added this way must be destroyed during the shutdown process.
     * @remarks
     * Note that after this method @c SpeakerManagerInterface instance will hold a reference to the @c
     * ChannelVolumeInterface added.
     *
     * @param channelVolumeInterface The @c ChannelVolumeInterface to be tracked
     */
    virtual void addChannelVolumeInterface(std::shared_ptr<ChannelVolumeInterface> channelVolumeInterface) = 0;

    /**
     * Destructor.
     */
    virtual ~SpeakerManagerInterface() = default;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEAKERMANAGERINTERFACE_H_
