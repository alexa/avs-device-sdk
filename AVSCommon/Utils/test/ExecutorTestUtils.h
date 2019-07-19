/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_EXECUTORTESTUTILS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_EXECUTORTESTUTILS_H_

#include <chrono>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {
namespace test {

/// Simple integer value to be reused in tests
static const int VALUE = 1234;

/// Timeout to use for waiting on calls within tests
static const std::chrono::milliseconds SHORT_TIMEOUT_MS = std::chrono::milliseconds(100);

/// A simple task which takes an integer argument and returns the argument when executed
static auto TASK = [](int arg) { return arg; };

/// Simple example function to be referenced in Executor related tests
void exampleFunction() {
}

/// Simple example function which takes a parameter to be referenced in Executor related tests
void exampleFunctionParams(int param) {
}

/**
 * ExampleFunctor is an example of a functor object, to be used in Executor related tests
 */
class ExampleFunctor {
public:
    void operator()() {
    }
};

/**
 * SimpleObject is an object intended to be used as an argument or return type of functions used in Executor related
 * tests
 */
class SimpleObject {
public:
    SimpleObject() : m_value{0} {};
    SimpleObject(int value) : m_value{value} {};

    int getValue() {
        return m_value;
    }

private:
    int m_value;
};

}  // namespace test
}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_EXECUTORTESTUTILS_H_
