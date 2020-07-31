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

#include <gtest/gtest.h>
#include <memory>

#include <AVSCommon/AVS/ComponentConfiguration.h>
#include <AVSCommon/SDKInterfaces/MockComponentReporterInterface.h>
#include <AVSCommon/Utils/SDKVersion.h>

#include "SDKComponent/SDKComponent.h"

namespace alexaClientSDK {
namespace applicationUtilities {
namespace SDKComponent {
namespace test {

using namespace avsCommon::sdkInterfaces::test;
using namespace ::testing;

using namespace avsCommon::avs;

// Name of the SDK Component Added to Component Reporter
static const std::string SDK_COMPONENT_NAME = "com.amazon.sdk";

/**
 * Test SDKComponent correctly returns sdk version in SDKVersion.h
 */
TEST(SDKComponentTest, test_returnSDKVersion) {
    std::shared_ptr<MockComponentReporterInterface> mockComponentReporter =
        std::make_shared<MockComponentReporterInterface>();
    const std::shared_ptr<ComponentConfiguration> sdkConfig = ComponentConfiguration::createComponentConfiguration(
        SDK_COMPONENT_NAME, avsCommon::utils::sdkVersion::getCurrentVersion());

    EXPECT_CALL(*mockComponentReporter, addConfiguration(sdkConfig)).Times(1).WillOnce(Return(true));

    EXPECT_TRUE(SDKComponent::registerComponent(mockComponentReporter));
}

}  // namespace test
}  // namespace SDKComponent
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
