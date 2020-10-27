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

#ifndef APPLICATIONS_ACSDKAPPLICATIONAUDIOPIPELINEFACTORYINTERFACES_TEST_ACSDKAPPLICATIONAUDIOPIPELINEFACTORYINTERFACES_MOCKAPPLICATIONAUDIOPIPELINEFACTORY_H_
#define APPLICATIONS_ACSDKAPPLICATIONAUDIOPIPELINEFACTORYINTERFACES_TEST_ACSDKAPPLICATIONAUDIOPIPELINEFACTORYINTERFACES_MOCKAPPLICATIONAUDIOPIPELINEFACTORY_H_

#include <gmock/gmock.h>

#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/ApplicationMediaInterfaces.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>

namespace alexaClientSDK {
namespace acsdkApplicationAudioPipelineFactoryInterfaces {
namespace test {

class MockApplicationAudioPipelineFactory
        : public acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface {
public:
    MOCK_METHOD6(
        createApplicationMediaInterfaces,
        std::shared_ptr<avsCommon::sdkInterfaces::ApplicationMediaInterfaces>(
            const std::string& name,
            bool equalizerAvailable,
            bool enableLiveMode,
            bool isCaptionable,
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type channelVolumeType,
            std::function<int8_t(int8_t)> volumeCurve));

    MOCK_METHOD7(
        createPooledApplicationMediaInterfaces,
        std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::PooledApplicationMediaInterfaces>(
            const std::string& name,
            int numMediaPlayers,
            bool equalizerAvailable,
            bool enableLiveMode,
            bool isCaptionable,
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type channelVolumeType,
            std::function<int8_t(int8_t)> volumeCurve));
};

}  // namespace test
}  // namespace acsdkApplicationAudioPipelineFactoryInterfaces
}  // namespace alexaClientSDK

#endif  // APPLICATIONS_ACSDKAPPLICATIONAUDIOPIPELINEFACTORYINTERFACES_TEST_ACSDKAPPLICATIONAUDIOPIPELINEFACTORYINTERFACES_MOCKAPPLICATIONAUDIOPIPELINEFACTORY_H_
