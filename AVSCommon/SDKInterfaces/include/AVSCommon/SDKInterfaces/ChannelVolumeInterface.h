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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CHANNELVOLUMEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CHANNELVOLUMEINTERFACE_H_

#include <functional>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * @c ChannelVolumeInterface provides an interface that allows control of speaker
 * settings for the underlying @c SpeakerInterface. Implementations of this interface must be
 * thread safe.
 */
class ChannelVolumeInterface {
public:
    /**
     * This enum provides the type of the @c SpeakerInterface.
     */
    enum class Type {
        /// Volume type reflecting AVS Speaker API volume.
        AVS_SPEAKER_VOLUME,
        /// Volume type reflecting AVS Alerts API volume.
        AVS_ALERTS_VOLUME
    };

    /**
     * Start channel volume attenuation for the underlying speaker.
     * The interface consults the volume curve function set in @c ChannelVolumeManager to determine
     * the desired attenuated channel volume.
     *
     * @return @c true if the operation succeeded, @c false otherwise. The API returns true, if the
     * channel is already attenuated.
     */
    virtual bool startDucking() = 0;

    /**
     * Restores the channel volume for the underlying speaker.
     *
     * @return @c true if the operation succeeded, @c false otherwise. The API returns true, if the
     * channel was not attenuated.
     */
    virtual bool stopDucking() = 0;

    /**
     * Set the volume of the underlying speaker. This reflects the
     * baseline volume settings for underlying Channel when it is not attenuated.
     * If the underlying @c SpeakerInterface is ducked when this API is invoked,
     * the corresponding unduckedVolume setting change is reflected upon the next
     * stopDucking call.
     *
     * @param volume the volume level to be set.
     * @return @c true if the operation succeeded, @c false otherwise.
     */
    virtual bool setUnduckedVolume(int8_t volume) = 0;

    /**
     * Set the mute state of the underlying speaker.
     *
     * @param mute the mute state to be set.
     * @return @c true if the operation succeeded, @c false otherwise.
     */
    virtual bool setMute(bool mute) = 0;

    /**
     * Returns a @c SpeakerSettings object to indicate the current settings of the
     * underlying @c speaker.
     * Note that if the underlying Channel is attenuated, the settings returned
     * must still return the unducked volume of the underlying speaker, as set by
     * the most recent call to the setUnduckedVolume API.
     *
     * @param [out] settings A @c SpeakerSettings object if successful.
     * @return @c true if the operation succeeded, @c false otherwise.
     */
    virtual bool getSpeakerSettings(avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings) const = 0;

    /**
     * Get the @c Type associated with the underlying @c SpeakerInterface.
     *
     * @return The @c Type.
     */
    virtual Type getSpeakerType() const = 0;

    /**
     * Destructor
     */
    virtual ~ChannelVolumeInterface() = default;
};

/**
 * Write a @c Type value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param type The type value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, ChannelVolumeInterface::Type type) {
    switch (type) {
        case ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME:
            stream << "AVS_SPEAKER_VOLUME";
            return stream;
        case ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME:
            stream << "AVS_ALERTS_VOLUME";
            return stream;
    }
    stream << "UNKNOWN_CHANNEL_VOLUME_TYPE";
    return stream;
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CHANNELVOLUMEINTERFACE_H_
