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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_TEST_INCLUDE_ACSDK_SPEAKERMANAGER_TEST_MOCKSPEAKERMANAGEROBSERVER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_TEST_INCLUDE_ACSDK_SPEAKERMANAGER_TEST_MOCKSPEAKERMANAGEROBSERVER_H_

#include <acsdk/Test/GmockExtensions.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>

namespace alexaClientSDK {
namespace speakerManager {
namespace test {

/**
 * @brief Mock object for @a SpeakerManagerObserverInterface.
 *
 * @see avsCommon::sdkInterfaces::SpeakerManagerObserverInterface
 * @ingroup Lib_acsdkSpeakerManagerTestLib
 */
class MockSpeakerManagerObserver : public avsCommon::sdkInterfaces::SpeakerManagerObserverInterface {
public:
    MOCK_METHOD3(
        onSpeakerSettingsChanged,
        void(
            const Source&,
            const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type&,
            const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings&));
};

}  // namespace test
}  // namespace speakerManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_TEST_INCLUDE_ACSDK_SPEAKERMANAGER_TEST_MOCKSPEAKERMANAGEROBSERVER_H_
