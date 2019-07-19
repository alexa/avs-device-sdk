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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_AUDIO_MOCKALERTSAUDIOFACTORY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_AUDIO_MOCKALERTSAUDIOFACTORY_H_

#include <gmock/gmock.h>

#include "AVSCommon/SDKInterfaces/Audio/AlertsAudioFactoryInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace audio {
namespace test {

/// Mock class that implements AlertsAudioFactoryInterface
class MockAlertsAudioFactory : public AlertsAudioFactoryInterface {
    MOCK_CONST_METHOD0(alarmDefault, std::function<std::unique_ptr<std::istream>()>());
    MOCK_CONST_METHOD0(alarmShort, std::function<std::unique_ptr<std::istream>()>());
    MOCK_CONST_METHOD0(timerDefault, std::function<std::unique_ptr<std::istream>()>());
    MOCK_CONST_METHOD0(timerShort, std::function<std::unique_ptr<std::istream>()>());
    MOCK_CONST_METHOD0(reminderDefault, std::function<std::unique_ptr<std::istream>()>());
    MOCK_CONST_METHOD0(reminderShort, std::function<std::unique_ptr<std::istream>()>());
};

}  // namespace test
}  // namespace audio
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_AUDIO_MOCKALERTSAUDIOFACTORY_H_
