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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERSTORAGEINTERFACETEST_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERSTORAGEINTERFACETEST_H_

#include <gmock/gmock.h>

#include <memory>

#include <AVSCommon/SDKInterfaces/Audio/EqualizerStorageInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace audio {
namespace test {

/**
 * Alias for the factory providing an instance of @c EqualizerStorageInterface. This could be a any code wrappable with
 * std::function: static or member function, lambda, etc...
 */
using EqualizerStorageFactory = std::function<std::shared_ptr<EqualizerStorageInterface>()>;

/**
 * @c EqualizerStorageInterface tests.
 *
 * Usage:
 * Go to the folder with the tests for your component. Append additional parameter "SDKInterfacesTests" (with quotes)
 * to "discover_unit_tests()" CMake directive in CMakeLists.txt file.
 * In *Test.cpp file in a global scope add the following line:
 * INSTANTIATE_TEST_CASE_P(<TestSequenceName>, EqualizerStorageInterfaceTest, ::testing::Values(<FactoryList>));
 * Where:
 * <TestSequenceName> is a test group's name you want to use. Without quotes. Example: MyEQTests
 * <FactoryList> is a comma-separated list of @c EqualizerStorageInterfaceFactory instances, one for each
 * implementation you want to test.
 *
 * See example in EqualizerImplementations/test/MiscDBEqualizerStorageTest.cpp
 */
class EqualizerStorageInterfaceTest : public ::testing::TestWithParam<EqualizerStorageFactory> {
public:
    /// SetUp before each test case.
    void SetUp() override;

protected:
    /**
     * Instance of the @c EqualizerStorageInterface being tested.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerStorageInterface> m_storage = nullptr;
};

}  // namespace test
}  // namespace audio
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERSTORAGEINTERFACETEST_H_
