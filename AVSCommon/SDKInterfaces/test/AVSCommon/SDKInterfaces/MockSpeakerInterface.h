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

/// String value for unmute.
static const std::string UNMUTE_STRING("false");

/// Value for default volume settings.
static const SpeakerInterface::SpeakerSettings DEFAULT_SETTINGS{AVS_SET_VOLUME_MIN, UNMUTE};

/*
 * Mock class that implements the SpeakerInterface. This is the delegate speaker class which
 * implements the SpeakerInterface methods.
 */
class MockSpeaker : public SpeakerInterface {
public:
    bool setVolume(int8_t volume) override;
    bool setMute(bool mute) override;
    bool getSpeakerSettings(SpeakerInterface::SpeakerSettings* settings) override;

    /// Constructor.
    MockSpeaker();

private:
    /// The current speaker settings of the interface.
    SpeakerInterface::SpeakerSettings m_settings;
};

inline bool MockSpeaker::setVolume(int8_t volume) {
    m_settings.volume = volume;
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

inline MockSpeaker::MockSpeaker() {
    m_settings = DEFAULT_SETTINGS;
}

/*
 * Mock class that implements the SpeakerInterface. This class delegates the Speaker
 * methods to the MockSpeaker object.
 */
class MockSpeakerInterface : public SpeakerInterface {
public:
    MOCK_METHOD1(setVolume, bool(int8_t));

    MOCK_METHOD1(setMute, bool(bool));

    MOCK_METHOD1(getSpeakerSettings, bool(SpeakerInterface::SpeakerSettings*));

    /// Delegate call from Mock Speaker object to an object with actual implementation.
    void DelegateToReal();

private:
    /// Implementation of Speaker object to handle calls.
    MockSpeaker m_speaker;
};

inline void MockSpeakerInterface::DelegateToReal() {
    ON_CALL(*this, setVolume(_)).WillByDefault(Invoke(&m_speaker, &SpeakerInterface::setVolume));
    ON_CALL(*this, setMute(_)).WillByDefault(Invoke(&m_speaker, &SpeakerInterface::setMute));
    ON_CALL(*this, getSpeakerSettings(_)).WillByDefault(Invoke(&m_speaker, &SpeakerInterface::getSpeakerSettings));
}
}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKSPEAKERINTERFACE_H_
