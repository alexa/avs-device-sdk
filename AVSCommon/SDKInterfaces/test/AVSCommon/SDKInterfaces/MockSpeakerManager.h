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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKSPEAKERMANAGER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKSPEAKERMANAGER_H_

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace test {

class MockSpeakerManager : public SpeakerManagerInterface {
public:
    MOCK_METHOD3(
        setVolume,
        std::future<bool>(
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
            int8_t volume,
            const avsCommon::sdkInterfaces::SpeakerManagerInterface::NotificationProperties& properties));

    MOCK_METHOD3(
        adjustVolume,
        std::future<bool>(
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
            int8_t delta,
            const avsCommon::sdkInterfaces::SpeakerManagerInterface::NotificationProperties& properties));

    MOCK_METHOD3(
        setMute,
        std::future<bool>(
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
            bool mute,
            const avsCommon::sdkInterfaces::SpeakerManagerInterface::NotificationProperties& properties));

    MOCK_METHOD4(
        setVolume,
        std::future<bool>(
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
            int8_t volume,
            bool forceNoNotifications,
            avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source source));

    MOCK_METHOD4(
        adjustVolume,
        std::future<bool>(
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
            int8_t delta,
            bool forceNoNotifications,
            avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source source));

    MOCK_METHOD4(
        setMute,
        std::future<bool>(
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
            bool mute,
            bool forceNoNotifications,
            avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source source));

#ifdef ENABLE_MAXVOLUME_SETTING
    MOCK_METHOD1(setMaximumVolumeLimit, std::future<bool>(const int8_t maximumVolumeLimit));
#endif  // ENABLE_MAXVOLUME_SETTING

    MOCK_METHOD2(
        getSpeakerSettings,
        std::future<bool>(
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type,
            avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings));

    MOCK_METHOD1(
        addSpeakerManagerObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer));
    MOCK_METHOD1(
        removeSpeakerManagerObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer));
    MOCK_METHOD1(
        addChannelVolumeInterface,
        void(std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> channelVolumeInterface));
};

}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKSPEAKERMANAGER_H_
