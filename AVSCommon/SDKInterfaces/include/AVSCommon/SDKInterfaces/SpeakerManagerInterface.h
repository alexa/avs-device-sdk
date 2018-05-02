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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEAKERMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEAKERMANAGERINTERFACE_H_

#include <future>
#include <memory>

#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

class SpeakerManagerInterface {
public:
    /**
     * Set the volume for speakers of a certain @c Type.
     *
     * @param type The type of speakers to modify.
     * @param volume The volume to set. Values must be between [0,100].
     * @param forceNoNotifications Setting this to true will ensure no event is sent and no observer is notified.
     * Setting this to false does not guarantee that a notification will be sent.
     * @return A future to be set with the operation's result. The future must be checked for validity
     * before attempting to obtain the shared state. An invalid future indicates an internal error,
     * and the caller should not assume the operation was succcessful.
     */
    virtual std::future<bool> setVolume(
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        int8_t volume,
        bool forceNoNotifications = false) = 0;

    /**
     * Adjusts the volume for speakers of a certain @c Type with a volume delta.
     *
     * @param type The type of speakers to modify.
     * @param delta The delta to modify volume by. Values must be between [-100,100].
     * @param forceNoNotifications Setting this to true will ensure no event is sent and no observer is notified.
     * Setting this to false does not guarantee that a notification will be sent.
     * @return A future to be set with the operation's result. The future must be checked for validity
     * before attempting to obtain the shared state. An invalid future indicates an internal error,
     * and the caller should not assume the operation was succcessful.
     */
    virtual std::future<bool> adjustVolume(
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        int8_t delta,
        bool forceNoNotifications = false) = 0;

    /**
     * Sets the mute for speakers of a certain @c Type.
     *
     * @param type The type of speakers to modify.
     * @param mute A boolean indicating mute. true = mute, false = unmute.
     * @param forceNoNotifications Setting this to true will ensure no event is sent and no observer is notified.
     * Setting this to false does not guarantee that a notification will be sent.
     * @return A future to be set with the operation's result. The future must be checked for validity
     * before attempting to obtain the shared state. An invalid future indicates an internal error,
     * and the caller should not assume the operation was succcessful.
     */
    virtual std::future<bool> setMute(
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        bool mute,
        bool forceNoNotifications = false) = 0;

    /**
     * Gets the speaker settings.
     *
     * @param type The type of speakers to retrieve settings for.
     * @param[out] settings The settings if the operation was successful.
     * @return A future to be set with the operation's result. The future must be checked for validity
     * before attempting to obtain the shared state. An invalid future indicates an internal error,
     * and the caller should not assume the operation was succcessful.
     */
    virtual std::future<bool> getSpeakerSettings(
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings) = 0;

    /**
     * Adds an observer to be notified when the SpeakerManager changes the @c SpeakerInterface::SpeakerSettings of a
     * group of @c SpeakerInterface::Type speakers.
     *
     * @param observer The observer to be notified when the @c SpeakerInterface::SpeakerSettings of a group successfully
     * changes.
     */
    virtual void addSpeakerManagerObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer) = 0;

    /**
     * Removes an observer from being notified when the SpeakerManager changes the @c SpeakerInterface::SpeakerSettings
     * of a group of @c SpeakerInterface::Type speakers.
     *
     * @param observer The observer to be notified when the @c SpeakerInterface::SpeakerSettings of a group successfully
     * changes.
     */
    virtual void removeSpeakerManagerObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer) = 0;

    /**
     * Adds a speaker to be tracked by @c SpeakerManagerInterface.
     * This method is not guaranteed to be thread safe and should be called during the initialization step only.
     * All speakers added this way must be destroyed during the shutdown process.
     * @remarks
     * Note that after this method @c SpeakerManagerInterface instance will hold a reference to the @c SpeakerInterface
     * added.
     * @param speaker
     */
    virtual void addSpeaker(std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEAKERMANAGERINTERFACE_H_
