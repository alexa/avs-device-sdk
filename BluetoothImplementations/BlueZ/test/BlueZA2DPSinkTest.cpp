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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "BlueZ/BlueZA2DPSink.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {
namespace test {

using namespace ::testing;

/// Test that a valid object is returned.
TEST(BlueZA2DPSinkTest, createSucceeds) {
    ASSERT_THAT(BlueZA2DPSink::create(), NotNull());
}

/// Test the correct SDP record is returned.
TEST(BlueZA2DPSinkTest, checkSDPRecord) {
    auto sdp = BlueZA2DPSink::create()->getRecord();
    ASSERT_EQ(sdp->getUuid(), std::string(avsCommon::sdkInterfaces::bluetooth::services::A2DPSinkInterface::UUID));
    ASSERT_EQ(sdp->getName(), std::string(avsCommon::sdkInterfaces::bluetooth::services::A2DPSinkInterface::NAME));
}

}  // namespace test
}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
