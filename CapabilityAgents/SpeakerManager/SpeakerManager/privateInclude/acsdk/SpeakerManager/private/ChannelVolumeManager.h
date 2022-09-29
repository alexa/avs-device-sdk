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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_CHANNELVOLUMEMANAGER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_CHANNELVOLUMEMANAGER_H_

#include <mutex>

#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>

namespace alexaClientSDK {
namespace speakerManager {
/**
 * @brief Default implementation for @c ChannelVolumeInterface.
 *
 * ChannelVolumeManager provides a concrete implementation of the @c ChannelVolumeInterface.
 * It controls an underlying @c SpeakerInterface object and provides functionality to be able
 * to set SpeakerSettings and control Channel Volume Attenuation for this underlying SpeakerInterface.
 *
 * @ingroup Lib_acsdkSpeakerManager
 */
class ChannelVolumeManager : public avsCommon::sdkInterfaces::ChannelVolumeInterface {
public:
    /**
     * Creates a ChannelVolumeManager that manages a single @c SpeakerInterface.
     *
     * @param speaker The @c SpeakerInterface associated with this instance.
     * @param type The @c ChannelVolumeInterface type associated with this instance.
     * @param volumeCurve The volume curve mapping to be used for channel attenuation.
     * @return ChannelVolumeManager
     */
    static std::shared_ptr<ChannelVolumeManager> create(
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker,
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type =
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
        std::function<int8_t(int8_t)> volumeCurve = nullptr) noexcept;

    /// ChannelVolumeInterface Functions.
    /// @{
    avsCommon::sdkInterfaces::ChannelVolumeInterface::Type getSpeakerType() const override;
    size_t getId() const override;
    bool startDucking() override;
    bool stopDucking() override;
    bool setUnduckedVolume(int8_t volume) override;
    bool setMute(bool mute) override;
    bool getSpeakerSettings(avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings) const override;
    /// @}

private:
    /**
     * Function which calculates the volume to use when ducked.  Implementations do not need to be thread-safe, but are
     * expected to return quickly.
     *
     * @param unduckedVolume The volume that would be used if when unducked.
     * @return The volume to use when ducked.
     */
    using VolumeCurveFunction = std::function<int8_t(int8_t unduckedVolume)>;

    /**
     * Constructor
     * @param speaker The @c SpeakerInterface associated with this instance.
     * @param type The @c ChannelVolumeInterface type associated with this instance.
     * @param volumeCurve The volume curve mapping to be used for channel attenuation.
     */
    ChannelVolumeManager(
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker,
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        VolumeCurveFunction volumeCurve) noexcept;

    /**
     * Default Volume Curve Implementation that determines the desired attenuated
     * channel volume for a given input volume.
     *
     * AVS_SET_VOLUME_MAX : avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MAX
     * AVS_SET_VOLUME_MIN : avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MIN
     *
     * The default piece-wise volume curve is as follows :
     * - @param unduckedVolume [0 - 0.20*AVS_SET_VOLUME_MAX)  : output = AVS_SET_VOLUME_MIN
     * - @param unduckedVolume [0.20*AVS_SET_VOLUME_MAX - 0.40*AVS_SET_VOLUME_MAX] : output = unduckedVolume -
     * (0.20*AVS_SET_VOLUME_MAX)
     * - @param unduckedVolume else : output = (0.20*AVS_SET_VOLUME_MAX)
     *
     * @note If an alternate volume curve is used, it is the responsibility of they
     * implementor to pass in a VolumeCurveFunction to ChannelVolumeManager.
     *
     * @param unduckedVolume current volume of the channel
     * @return int8_t volume to be attenuated to as per the volume curve
     */
    static int8_t defaultVolumeAttenuateFunction(int8_t unduckedVolume) noexcept;

    /// Mutex to synchronize the operations.
    mutable std::mutex m_mutex;

    /// The underlying @c SpeakerInterface
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> m_speaker;

    /// Bool to track if the channel volume is attenuated.
    bool m_isDucked;

    /// The Current Unducked channel volume.
    int8_t m_unduckedVolume;

    /// Volume Curve function
    VolumeCurveFunction m_volumeCurveFunction;

    /// Speaker Type
    const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type m_type;
};
}  // namespace speakerManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_CHANNELVOLUMEMANAGER_H_
