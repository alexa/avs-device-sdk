/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_ANDROIDSLESSPEAKER_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_ANDROIDSLESSPEAKER_H_

#include <memory>
#include <mutex>
#include <utility>

#include <SLES/OpenSLES.h>

#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AndroidUtilities/AndroidSLESEngine.h>
#include <AndroidUtilities/AndroidSLESObject.h>

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

/**
 * This class implements an android compatible speaker.
 *
 * The implementation uses Android OpenSL ES to control speaker volume. We assume that OpenSL ES for Android is
 * thread-safe as stated <a href=https://developer.android.com/ndk/guides/audio/opensl/opensl-for-android> here </a>.
 */
class AndroidSLESSpeaker : public avsCommon::sdkInterfaces::SpeakerInterface {
public:
    /**
     * Create AndroidSLESSpeaker.
     *
     * @param engine The OpenSL ES engine that the @c speakerObject depends on.
     * @param outputMixObject The OpenSL ES output mix that the @c speakerObject depends on.
     * @param speakerObject The OpenSL ES object responsible for controlling the speaker output configurations.
     * @param type The type used to categorize the speaker for volume control.
     * @return An instance of the @c AndroidSLESSpeaker if successful else @c nullptr.
     */
    static std::unique_ptr<AndroidSLESSpeaker> create(
        std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESEngine> engine,
        std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> outputMixObject,
        std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> speakerObject,
        SpeakerInterface::Type type);

    /// @name SpeakerInterface methods.
    ///@{
    bool setVolume(int8_t volume) override;
    bool adjustVolume(int8_t delta) override;
    bool setMute(bool mute) override;
    bool getSpeakerSettings(SpeakerSettings* settings) override;
    Type getSpeakerType() override;
    ///@}

private:
    /**
     * Constructor.
     *
     * @param engine The OpenSL ES engine that the @c speakerObject depends on.
     * @param outputMixObject The OpenSL ES output mix that the @c speakerObject depends on.
     * @param speakerObject The speaker object used to control the media player volume.
     * @param type The type used to categorize the speaker for volume control.
     * @param volumeInterface The interface object used to change the speaker volume.
     */
    AndroidSLESSpeaker(
        std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESEngine> engine,
        std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> outputMixObject,
        std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> speakerObject,
        SpeakerInterface::Type type,
        SLVolumeItf volumeInterface);

    /**
     * Get device volume converted to AVS volume.
     *
     * @return First element indicates if the operation succeeded, and second contains the avs volume.
     */
    std::pair<bool, int8_t> getAvsVolume() const;

    /**
     * Converts device volume to avs volume scale.
     *
     * @param deviceVolume The device volume that we would like to convert to AVS volume.
     * @return The avs normalized volume.
     */
    int8_t toAvsVolume(SLmillibel deviceVolume) const;

    /**
     * Converts avs volume to device volume scale.
     *
     * @param avsVolume The AVS volume that we would like to convert to device volume.
     * @return The device normalized volume.
     */
    SLmillibel toDeviceVolume(int8_t avsVolume) const;

    /// Keep a pointer to the OpenSL ES engine to guarantee that it doesn't get destroyed before other OpenSL ES
    /// objects.
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESEngine> m_engine;

    /// Save pointer to the OpenSL ES output mix object which should be destroyed only after @c m_speakerObject.
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> m_outputMixObject;

    /// Save pointer to the OpenSL ES speaker object which must be valid for us to use the volume interface.
    /// See "Object and Interfaces" section of OpenSL ES for Android
    /// <a href=https://googlesamples.github.io/android-audio-high-performance/guides/opensl_es.html> documentation </a>
    /// for a more detailed explanation.
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> m_speakerObject;

    /// The OpenSL ES volume interface.
    SLVolumeItf m_volumeInterface;

    /// The speaker type.
    const SpeakerInterface::Type m_type;
};

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_ANDROIDSLESSPEAKER_H_
