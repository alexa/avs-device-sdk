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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_MOCKSYSTEMCLOCKDELAY_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_MOCKSYSTEMCLOCKDELAY_H_

#include <gmock/gmock.h>

#include <Captions/DelayInterface.h>

namespace alexaClientSDK {
namespace captions {
namespace test {

using namespace ::testing;

/**
 * A mock DelayInterface for validating timing.
 */
class MockSystemClockDelay : public DelayInterface {
public:
    MockSystemClockDelay() = default;
    /// @name DelayInterface methods
    /// @{
    MOCK_METHOD1(delay, void(std::chrono::milliseconds));
    /// }@
};

}  // namespace test
}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_MOCKSYSTEMCLOCKDELAY_H_