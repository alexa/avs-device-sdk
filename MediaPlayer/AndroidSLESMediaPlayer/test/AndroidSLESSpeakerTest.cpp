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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <SLES/OpenSLES.h>

#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AndroidUtilities/MockAndroidSLESObject.h>

#include "AndroidSLESMediaPlayer/AndroidSLESSpeaker.h"

/// String to identify log entries originating from this file.
static const std::string TAG{"AndroidSLESSpeakerTest"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {
namespace test {

using namespace ::testing;
using namespace applicationUtilities::androidUtilities;
using namespace applicationUtilities::androidUtilities::test;
using namespace avsCommon::avs::speakerConstants;

using MockVolumeInterface = MockInterfaceImpl<SLVolumeItf_>;

/// Represents an invalid device maximum value. OpenSL ES determines that device max volume is 0 or above.
static constexpr SLmillibel INVALID_MAX_VOLUME = -10;

static SLmillibel g_volume;
static SLmillibel g_maxVolume;
static SLboolean g_mute;

/// Mock @c GetVolume succeeds.
static SLresult mockGetVolume(SLVolumeItf self, SLmillibel* volume) {
    *volume = g_volume;
    return SL_RESULT_SUCCESS;
}

/// Mock @c GetVolume fails.
static SLresult mockGetVolumeFailed(SLVolumeItf self, SLmillibel* volume) {
    return SL_RESULT_INTERNAL_ERROR;
}

/// Mock @c SetVolume succeeds.
static SLresult mockSetVolume(SLVolumeItf self, SLmillibel volume) {
    g_volume = volume;
    return SL_RESULT_SUCCESS;
}

/// Mock @c SetVolume fails.
static SLresult mockSetVolumeFailed(SLVolumeItf self, SLmillibel volume) {
    return SL_RESULT_INTERNAL_ERROR;
}

/// Mock @c GetMute succeeds.
static SLresult mockGetMute(SLVolumeItf self, SLboolean* mute) {
    *mute = g_mute;
    return SL_RESULT_SUCCESS;
}

/// Mock @c GetMute fails.
static SLresult mockGetMuteFailed(SLVolumeItf self, SLboolean* mute) {
    return SL_RESULT_INTERNAL_ERROR;
}

/// Mock @c SetMute succeeds.
static SLresult mockSetMute(SLVolumeItf self, SLboolean mute) {
    g_mute = mute;
    return SL_RESULT_SUCCESS;
}

/// Mock @c SetMute fails.
static SLresult mockSetMuteFailed(SLVolumeItf self, SLboolean mute) {
    return SL_RESULT_INTERNAL_ERROR;
}

/// Mock @c GetMaxVolume succeeds.
static SLresult mockGetMaxVolume(SLVolumeItf self, SLmillibel* volume) {
    *volume = g_maxVolume;
    return SL_RESULT_SUCCESS;
}

class AndroidSLESSpeakerTest : public Test {
protected:
    void SetUp() override;

    /// Pointer to android engine.
    std::shared_ptr<AndroidSLESEngine> m_engine;

    /// Mock the output mix object.
    std::shared_ptr<AndroidSLESObject> m_outputMix;

    /// Mock the OpenSL ES speaker wrapper.
    std::shared_ptr<MockAndroidSLESObject> m_objectMock;

    /// Mock the actual OpenSL ES speaker object.
    std::shared_ptr<AndroidSLESObject> m_slObject;

    /// Mock the OpenSL ES volume interface.
    std::shared_ptr<MockVolumeInterface> m_volumeMock;

    /// The speaker that is under test.
    std::unique_ptr<AndroidSLESSpeaker> m_speaker;
};

void AndroidSLESSpeakerTest::SetUp() {
    g_volume = 0;
    g_mute = 0;
    g_maxVolume = 0;

    m_engine = AndroidSLESEngine::create();
    m_outputMix = AndroidSLESObject::create(MockAndroidSLESObject().getObject());
    m_objectMock = std::make_shared<MockAndroidSLESObject>();
    m_volumeMock = std::make_shared<MockVolumeInterface>();
    m_slObject = AndroidSLESObject::create(m_objectMock->getObject());
    m_objectMock->mockGetInterface(SL_IID_VOLUME, m_volumeMock);
    m_volumeMock->get().GetVolumeLevel = mockGetVolume;
    m_volumeMock->get().SetVolumeLevel = mockSetVolume;
    m_volumeMock->get().GetMaxVolumeLevel = mockGetMaxVolume;
    m_volumeMock->get().GetMute = mockGetMute;
    m_volumeMock->get().SetMute = mockSetMute;

    m_speaker =
        AndroidSLESSpeaker::create(m_engine, m_outputMix, m_slObject, AndroidSLESSpeaker::Type::AVS_SPEAKER_VOLUME);
    ASSERT_NE(m_speaker, nullptr);
}

/// Test speaker create with null engine.
TEST_F(AndroidSLESSpeakerTest, testCreateNullEngine) {
    auto speaker =
        AndroidSLESSpeaker::create(nullptr, m_outputMix, m_slObject, AndroidSLESSpeaker::Type::AVS_SPEAKER_VOLUME);
    EXPECT_EQ(speaker, nullptr);
}

/// Test speaker create with null output mix.
TEST_F(AndroidSLESSpeakerTest, testCreateNullSpeaker) {
    auto speaker =
        AndroidSLESSpeaker::create(m_engine, m_outputMix, nullptr, AndroidSLESSpeaker::Type::AVS_SPEAKER_VOLUME);
    EXPECT_EQ(speaker, nullptr);
}

/// Test speaker create with null engine.
TEST_F(AndroidSLESSpeakerTest, testCreateInterfaceUnavailable) {
    m_objectMock->mockGetInterface(SL_IID_VOLUME, nullptr);
    auto speaker =
        AndroidSLESSpeaker::create(m_engine, m_outputMix, m_slObject, AndroidSLESSpeaker::Type::AVS_SPEAKER_VOLUME);
    EXPECT_EQ(speaker, nullptr);
}

/// Test speaker create with invalid device configuration.
TEST_F(AndroidSLESSpeakerTest, testCreateInvalidMaxVolume) {
    auto mockInvalidMaxVolume = [](SLVolumeItf self, SLmillibel* volume) -> SLresult {
        *volume = INVALID_MAX_VOLUME;
        return SL_RESULT_SUCCESS;
    };
    m_volumeMock->get().GetMaxVolumeLevel = mockInvalidMaxVolume;
    auto speaker =
        AndroidSLESSpeaker::create(m_engine, m_outputMix, m_slObject, AndroidSLESSpeaker::Type::AVS_SPEAKER_VOLUME);
    EXPECT_EQ(speaker, nullptr);
}

/// Test set and get volume succeed.
TEST_F(AndroidSLESSpeakerTest, testSetVolumeSucceed) {
    int8_t volume = (AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) / 2;  // Set volume to 50% of max.
    EXPECT_TRUE(m_speaker->setVolume(volume));

    AndroidSLESSpeaker::SpeakerSettings settings;
    EXPECT_TRUE(m_speaker->getSpeakerSettings(&settings));
    EXPECT_EQ(settings.volume, volume);
}

/// Test set volume failed.
TEST_F(AndroidSLESSpeakerTest, testSetVolumeFailed) {
    m_volumeMock->get().SetVolumeLevel = mockSetVolumeFailed;

    int8_t volume = (AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) / 2;  // Set volume to 50% of max.
    EXPECT_FALSE(m_speaker->setVolume(volume));
}

/// Test get volume failed.
TEST_F(AndroidSLESSpeakerTest, testGetVolumeFailed) {
    m_volumeMock->get().GetVolumeLevel = mockGetVolumeFailed;

    AndroidSLESSpeaker::SpeakerSettings settings;
    EXPECT_FALSE(m_speaker->getSpeakerSettings(&settings));
}

/// Test set and get mute succeed.
TEST_F(AndroidSLESSpeakerTest, testSetMuteSucceed) {
    bool mute = !g_mute;  // Toggle mute.
    EXPECT_TRUE(m_speaker->setMute(mute));

    AndroidSLESSpeaker::SpeakerSettings settings;
    EXPECT_TRUE(m_speaker->getSpeakerSettings(&settings));
    EXPECT_EQ(settings.mute, mute);
}

/// Test set mute failed.
TEST_F(AndroidSLESSpeakerTest, testSetMuteFailed) {
    m_volumeMock->get().SetMute = mockSetMuteFailed;
    EXPECT_FALSE(m_speaker->setMute(!g_mute));
}

/// Test get mute failed.
TEST_F(AndroidSLESSpeakerTest, testGetMuteFailed) {
    m_volumeMock->get().GetMute = mockGetMuteFailed;
    AndroidSLESSpeaker::SpeakerSettings settings;
    EXPECT_FALSE(m_speaker->getSpeakerSettings(&settings));
}

/// Test adjust and get volume succeed.
TEST_F(AndroidSLESSpeakerTest, testAdjustVolumeSucceed) {
    int8_t volume = (AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) / 2;  // Set volume to 50% of max then add 50% more.
    EXPECT_TRUE(m_speaker->setVolume(volume));
    EXPECT_TRUE(m_speaker->adjustVolume(volume));

    AndroidSLESSpeaker::SpeakerSettings settings;
    EXPECT_TRUE(m_speaker->getSpeakerSettings(&settings));
    EXPECT_EQ(settings.volume, volume * 2);
}

/// Test adjust volume failed.
TEST_F(AndroidSLESSpeakerTest, testAdjustVolumeFailed) {
    m_volumeMock->get().SetVolumeLevel = mockSetVolumeFailed;

    int8_t volume = (AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.1;  // Adjust volume by 10% of max.
    EXPECT_FALSE(m_speaker->adjustVolume(volume));
}

/// Test adjust volume over maximum value. Speaker should be adjusted till maximum.
TEST_F(AndroidSLESSpeakerTest, testAdjustVolumeOverMax) {
    int8_t delta = 10;  // Try to adjust volume by a random value.
    int8_t volume = AVS_SET_VOLUME_MAX;
    EXPECT_TRUE(m_speaker->setVolume(volume));
    EXPECT_TRUE(m_speaker->adjustVolume(delta));

    AndroidSLESSpeaker::SpeakerSettings settings;
    EXPECT_TRUE(m_speaker->getSpeakerSettings(&settings));
    EXPECT_EQ(settings.volume, AVS_SET_VOLUME_MAX);
}

/// Test adjust volume below minimum value. Speaker should be adjusted to minimum.
TEST_F(AndroidSLESSpeakerTest, testAdjustVolumeUnderMin) {
    int8_t delta = -10;  // Try to adjust volume by a random value.
    int8_t volume = AVS_SET_VOLUME_MIN;
    EXPECT_TRUE(m_speaker->setVolume(volume));
    EXPECT_TRUE(m_speaker->adjustVolume(delta));

    AndroidSLESSpeaker::SpeakerSettings settings;
    EXPECT_TRUE(m_speaker->getSpeakerSettings(&settings));
    EXPECT_EQ(settings.volume, AVS_SET_VOLUME_MIN);
}

/// Test set and get volume on values in different ranges to guarantee accuracy.
TEST_F(AndroidSLESSpeakerTest, testSetVolumeAccuracy) {
    auto speakerPtr = m_speaker.get();
    auto check = [speakerPtr](int8_t avsVolume) {
        AndroidSLESSpeaker::SpeakerSettings settings;
        EXPECT_TRUE(speakerPtr->setVolume(avsVolume));
        EXPECT_TRUE(speakerPtr->getSpeakerSettings(&settings));
        EXPECT_EQ(settings.volume, avsVolume);
    };

    check(AVS_SET_VOLUME_MAX);
    check((AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.99);
    check((AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.95);
    check((AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.90);
    check((AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.50);
    check((AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.10);
    check((AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.05);
    check((AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.01);
    check(AVS_SET_VOLUME_MIN);
}

/// Test that the conversion from avs volume to device volume.
TEST_F(AndroidSLESSpeakerTest, testSetDeviceVolume) {
    auto speakerPtr = m_speaker.get();
    auto check = [speakerPtr](int8_t avsVolume, SLmillibel expected) {
        EXPECT_TRUE(speakerPtr->setVolume(avsVolume));
        EXPECT_EQ(g_volume, expected);
    };

    check(AVS_SET_VOLUME_MAX, g_maxVolume);
    check((AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.99, -8);
    check((AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.95, -44);
    check((AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.90, -91);
    check((AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.50, -602);
    check((AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.10, -2000);
    check((AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.05, -2602);
    check((AVS_SET_VOLUME_MAX - AVS_SET_VOLUME_MIN) * 0.01, -4000);
    check(AVS_SET_VOLUME_MIN, SL_MILLIBEL_MIN);
}

}  // namespace test
}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
