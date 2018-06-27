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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKSPEAKERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKSPEAKERINTERFACE_H_

#include <gmock/gmock.h>

#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace test {

using namespace avsCommon::avs::speakerConstants;
using namespace ::testing;

/// Value for mute.
static const bool MUTE(true);

/// String value for mute.
static const std::string MUTE_STRING("true");

/// Value for unmute.
static const bool UNMUTE(false);

/// Value for default volume settings.
static const SpeakerInterface::SpeakerSettings DEFAULT_SETTINGS{AVS_SET_VOLUME_MIN, UNMUTE};

/*
 * Mock class that implements the SpeakerInterface. This is the delegate speaker class which
 * implements the SpeakerInterface methods.
 */
class MockSpeaker : public SpeakerInterface {
public:
    bool setVolume(int8_t volume) override;
    bool adjustVolume(int8_t delta) override;
    bool setMute(bool mute) override;
    bool getSpeakerSettings(SpeakerInterface::SpeakerSettings* settings) override;
    SpeakerInterface::Type getSpeakerType() override;

    /// Constructor.
    MockSpeaker(SpeakerInterface::Type type);

private:
    /// The type of the Speaker interface.
    SpeakerInterface::Type m_type;

    /// The current speaker settings of the interface.
    SpeakerInterface::SpeakerSettings m_settings;
};

inline bool MockSpeaker::setVolume(int8_t volume) {
    m_settings.volume = volume;
    return true;
}

inline bool MockSpeaker::adjustVolume(int8_t delta) {
    int8_t curVolume = m_settings.volume + delta;
    curVolume = std::min(curVolume, AVS_SET_VOLUME_MAX);
    curVolume = std::max(curVolume, AVS_SET_VOLUME_MIN);
    m_settings.volume = curVolume;
    return true;
}

inline bool MockSpeaker::setMute(bool mute) {
    m_settings.mute = mute;
    return true;
}

inline bool MockSpeaker::getSpeakerSettings(SpeakerInterface::SpeakerSettings* settings) {
    if (!settings) {
        return false;
    }

    settings->volume = m_settings.volume;
    settings->mute = m_settings.mute;

    return true;
}

inline SpeakerInterface::Type MockSpeaker::getSpeakerType() {
    return m_type;
}

inline MockSpeaker::MockSpeaker(SpeakerInterface::Type type) : m_type{type} {
    m_settings = DEFAULT_SETTINGS;
}

/*
 * Mock class that implements the SpeakerInterface. This class delegates the Speaker
 * methods to the MockSpeaker object.
 */
class MockSpeakerInterface : public SpeakerInterface {
public:
    MOCK_METHOD1(setVolume, bool(int8_t));

    MOCK_METHOD1(adjustVolume, bool(int8_t));

    MOCK_METHOD1(setMute, bool(bool));

    MOCK_METHOD1(getSpeakerSettings, bool(SpeakerInterface::SpeakerSettings*));

    MOCK_METHOD0(getSpeakerType, SpeakerInterface::Type());

    /// Delegate call from Mock Speaker object to an object with actual implementation.
    void DelegateToReal();

    /// Constructor.
    MockSpeakerInterface(SpeakerInterface::Type type);

private:
    /// Implementation of Speaker object to handle calls.
    MockSpeaker m_speaker;
};

inline void MockSpeakerInterface::DelegateToReal() {
    ON_CALL(*this, setVolume(_)).WillByDefault(Invoke(&m_speaker, &SpeakerInterface::setVolume));
    ON_CALL(*this, adjustVolume(_)).WillByDefault(Invoke(&m_speaker, &SpeakerInterface::adjustVolume));
    ON_CALL(*this, setMute(_)).WillByDefault(Invoke(&m_speaker, &SpeakerInterface::setMute));
    ON_CALL(*this, getSpeakerSettings(_)).WillByDefault(Invoke(&m_speaker, &SpeakerInterface::getSpeakerSettings));
    ON_CALL(*this, getSpeakerType()).WillByDefault(Invoke(&m_speaker, &SpeakerInterface::getSpeakerType));
}

inline MockSpeakerInterface::MockSpeakerInterface(SpeakerInterface::Type type) : m_speaker{type} {
}

}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKSPEAKERINTERFACE_H_
