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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_TEST_INCLUDE_ACSDK_SPEAKERMANAGER_TEST_MOCKSPEAKERMANAGERCONFIG_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_TEST_INCLUDE_ACSDK_SPEAKERMANAGER_TEST_MOCKSPEAKERMANAGERCONFIG_H_

#include <acsdk/Test/GmockExtensions.h>

namespace alexaClientSDK {
namespace speakerManager {
namespace test {

/**
 * @brief Mock object for @a SpeakerManagerConfigInterface.
 *
 * @see SpeakerManagerConfigInterface
 * @ingroup Lib_acsdkSpeakerManagerTestLib
 */
class MockSpeakerManagerConfig : public SpeakerManagerConfigInterface {
public:
    MOCK_NOEXCEPT_METHOD1(getPersistentStorage, bool(bool&));
    MOCK_NOEXCEPT_METHOD1(getMinUnmuteVolume, bool(std::uint8_t&));
    MOCK_NOEXCEPT_METHOD1(getRestoreMuteState, bool(bool&));
    MOCK_NOEXCEPT_METHOD1(getDefaultSpeakerVolume, bool(std::uint8_t&));
    MOCK_NOEXCEPT_METHOD1(getDefaultAlertsVolume, bool(std::uint8_t&));
};

}  // namespace test
}  // namespace speakerManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_TEST_INCLUDE_ACSDK_SPEAKERMANAGER_TEST_MOCKSPEAKERMANAGERCONFIG_H_
