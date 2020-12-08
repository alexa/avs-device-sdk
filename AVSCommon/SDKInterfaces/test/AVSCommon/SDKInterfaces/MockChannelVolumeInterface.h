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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKCHANNELVOLUMEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKCHANNELVOLUMEINTERFACE_H_

#include <gmock/gmock.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>
#include <AVSCommon/SDKInterfaces/MockSpeakerInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace test {
using namespace ::testing;

class MockChannelVolumeManager : public avsCommon::sdkInterfaces::ChannelVolumeInterface {
public:
    avsCommon::sdkInterfaces::ChannelVolumeInterface::Type getSpeakerType() const {
        return m_type;
    }
    size_t getId() const {
        return (size_t)m_speaker.get();
    }
    bool startDucking() {
        return true;
    }
    bool stopDucking() {
        return true;
    }
    bool setUnduckedVolume(int8_t volume) {
        m_settings.volume = volume;
        return true;
    }
    bool setMute(bool mute) {
        m_settings.mute = mute;
        return true;
    }
    bool getSpeakerSettings(avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings) const {
        if (!settings) {
            return false;
        }
        settings->volume = m_settings.volume;
        settings->mute = m_settings.mute;
        return true;
    }
    MockChannelVolumeManager(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker) :
            ChannelVolumeInterface(),
            m_settings{avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MIN, false},
            m_type{type},
            m_speaker{speaker} {
    }

private:
    avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings m_settings;
    const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type m_type;
    const std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> m_speaker;
};

class MockChannelVolumeInterface : public avsCommon::sdkInterfaces::ChannelVolumeInterface {
public:
    MOCK_CONST_METHOD0(getSpeakerType, avsCommon::sdkInterfaces::ChannelVolumeInterface::Type());
    MOCK_CONST_METHOD0(getId, size_t());
    MOCK_METHOD0(startDucking, bool());
    MOCK_METHOD0(stopDucking, bool());
    MOCK_METHOD1(setUnduckedVolume, bool(int8_t));
    MOCK_METHOD1(setMute, bool(bool));
    MOCK_CONST_METHOD1(getSpeakerSettings, bool(avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings*));
    void DelegateToReal();
    MockChannelVolumeInterface(
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type =
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker =
            std::make_shared<NiceMock<MockSpeakerInterface>>()) :
            ChannelVolumeInterface(),
            m_manager{type, speaker} {
    }

private:
    MockChannelVolumeManager m_manager;
};

inline void MockChannelVolumeInterface::DelegateToReal() {
    ON_CALL(*this, getSpeakerType()).WillByDefault(Invoke(&m_manager, &ChannelVolumeInterface::getSpeakerType));
    ON_CALL(*this, getId()).WillByDefault(Invoke(&m_manager, &ChannelVolumeInterface::getId));
    ON_CALL(*this, startDucking()).WillByDefault(Invoke(&m_manager, &ChannelVolumeInterface::startDucking));
    ON_CALL(*this, stopDucking()).WillByDefault(Invoke(&m_manager, &ChannelVolumeInterface::stopDucking));
    ON_CALL(*this, setUnduckedVolume(_)).WillByDefault(Invoke(&m_manager, &ChannelVolumeInterface::setUnduckedVolume));
    ON_CALL(*this, setMute(_)).WillByDefault(Invoke(&m_manager, &ChannelVolumeInterface::setMute));
    ON_CALL(*this, getSpeakerSettings(_))
        .WillByDefault(Invoke(&m_manager, &ChannelVolumeInterface::getSpeakerSettings));
}

}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKCHANNELVOLUMEINTERFACE_H_
