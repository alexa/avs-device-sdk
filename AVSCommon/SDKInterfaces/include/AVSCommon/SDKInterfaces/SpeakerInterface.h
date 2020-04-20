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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEAKERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEAKERINTERFACE_H_

#include <ostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * The SpeakerInterface is concerned with the control of volume and mute settings of a speaker.
 * The two settings are independent of each other, and the respective APIs shall not affect
 * the other setting in any way. Compound behaviors (such as unmuting when volume is adjusted) will
 * be handled at a layer above this interface.
 *
 * The methods in this interface MUST be implemented as thread safe implementations.
 */
class SpeakerInterface {
public:
    /**
     * This contains the current settings of the @c SpeakerInterface.
     * The minimum volume level should correspond to no volume output, but this setting should still be independent
     * from mute. For speakers which do not have independent mute and volume settings, the interface must be implemented
     * as if they did so. For example, when unmuting a speaker, the volume setting should correspond to the level it was
     * at before muting.
     */
    struct SpeakerSettings {
        /// Must be within [AVS_SET_VOLUME_MIN, AVS_SET_VOLUME_MAX].
        int8_t volume;
        /// True means muted, false means unmuted.
        bool mute;
        /// Operator overload to compare two SpeakerSettings objects.
        bool operator==(const SpeakerSettings& rhs) const {
            return volume == rhs.volume && mute == rhs.mute;
        }
    };

    /**
     * Set the absolute volume of the speaker. @c volume
     * will be [AVS_SET_VOLUME_MIN, AVS_SET_VOLUME_MAX], and implementers of the interface must normalize
     * the volume to fit the needs of their drivers.
     *
     * @param volume A volume to set the speaker to.
     * @return Whether the operation was successful.
     */
    virtual bool setVolume(int8_t volume) = 0;

    /**
     * Set the mute of the speaker.
     *
     * @param mute Represents whether the speaker should be muted (true) or unmuted (false).
     * @return Whether the operation was successful.
     */
    virtual bool setMute(bool mute) = 0;

    /**
     * Return a @c SpeakerSettings object to indicate the current settings of the SpeakerInterface.
     *
     * @param [out] settings A @c SpeakerSettings object if successful.
     * @return Whether the operation was successful.
     */
    virtual bool getSpeakerSettings(SpeakerSettings* settings) = 0;

    /**
     * Destructor.
     */
    virtual ~SpeakerInterface() = default;
};
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEAKERINTERFACE_H_
