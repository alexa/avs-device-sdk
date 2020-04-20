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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_AUDIO_MOCKSYSTEMSOUNDAUDIOFACTORY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_AUDIO_MOCKSYSTEMSOUNDAUDIOFACTORY_H_

#include <gmock/gmock.h>

#include "AVSCommon/SDKInterfaces/Audio/SystemSoundAudioFactoryInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace audio {
namespace test {

/// Mock class that implements SystemSoundAudioFactoryInterface
class MockSystemSoundAudioFactory : public SystemSoundAudioFactoryInterface {
public:
    static std::shared_ptr<testing::NiceMock<MockSystemSoundAudioFactory>> create();
    MOCK_CONST_METHOD0(
        endSpeechTone,
        std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()>());
    MOCK_CONST_METHOD0(
        wakeWordNotificationTone,
        std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()>());

private:
    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>
    createWakeWordNotificationTone() {
        return std::make_pair(
            std::unique_ptr<std::stringstream>(new std::stringstream("testWakeTone")),
            avsCommon::utils::MediaType::MPEG);
    }
    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> createEndSpeechTone() {
        return std::make_pair(
            std::unique_ptr<std::stringstream>(new std::stringstream("testEndSpeech")),
            avsCommon::utils::MediaType::MPEG);
    }
};

std::shared_ptr<testing::NiceMock<MockSystemSoundAudioFactory>> MockSystemSoundAudioFactory::create() {
    auto result = std::make_shared<testing::NiceMock<MockSystemSoundAudioFactory>>();

    ON_CALL(*result.get(), endSpeechTone()).WillByDefault(testing::Return(createEndSpeechTone));
    ON_CALL(*result.get(), wakeWordNotificationTone()).WillByDefault(testing::Return(createWakeWordNotificationTone));

    return result;
}

}  // namespace test
}  // namespace audio
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_AUDIO_MOCKSYSTEMSOUNDAUDIOFACTORY_H_
