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

#include <AVSCommon/SDKInterfaces/Audio/EqualizerStorageInterfaceTest.h>
#include <AVSCommon/SDKInterfaces/Storage/StubMiscStorage.h>

#include "EqualizerImplementations/MiscDBEqualizerStorage.h"

namespace alexaClientSDK {
namespace equalizer {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces::audio;
using namespace avsCommon::sdkInterfaces::audio::test;
using namespace avsCommon::sdkInterfaces::storage::test;

/// Apply @c EqualizerStorageInterfaceTest test fixture to @c MiscDBEqualizerStorage.
INSTANTIATE_TEST_CASE_P(MiscDBStorageTests, EqualizerStorageInterfaceTest, ::testing::Values([]() {
                            return MiscDBEqualizerStorage::create(StubMiscStorage::create());
                        }));

}  // namespace test
}  // namespace equalizer
}  // namespace alexaClientSDK
