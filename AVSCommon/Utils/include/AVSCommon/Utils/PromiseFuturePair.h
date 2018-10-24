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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PROMISEFUTUREPAIR_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PROMISEFUTUREPAIR_H_

#include <future>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * Template to pair a promise and it's future that can store a value.
 */
template <typename Type>
class PromiseFuturePair {
public:
    /**
     * Constructor
     */
    PromiseFuturePair() : m_future{m_promise.get_future()} {
    }

    /**
     * Set the value in promise.
     * @param val The value to be set.
     */
    void setValue(Type val) {
        m_promise.set_value(val);
    }

    /**
     * Wait for promise to be set.
     * @param timeout Timeout for waiting for promise to be set.
     * @return True if promise has been set before timeout, otherwise false.
     */
    bool waitFor(std::chrono::milliseconds timeout) {
        auto future = m_future;
        return future.wait_for(timeout) == std::future_status::ready;
    }

    /**
     * Retrieved the promised value.
     */
    Type getValue() {
        auto future = m_future;
        return future.get();
    }

private:
    /// The promise that will be set later asynchronously with a value.
    std::promise<Type> m_promise;

    /// The future object based from the @c m_promise.
    std::shared_future<Type> m_future;
};

/**
 * Template to pair a promise and it's future with Void values.
 */
template <>
class PromiseFuturePair<void> {
public:
    /**
     * Constructor
     */
    PromiseFuturePair() : m_future{m_promise.get_future()} {
    }

    /**
     * Set the value in promise.
     */
    void setValue() {
        m_promise.set_value();
    }

    /**
     * Wait for promise to be set.
     * @param timeout Timeout for waiting for promise to be set.
     * @return True if promise has been set before timeout, otherwise false.
     */
    bool waitFor(std::chrono::milliseconds timeout) {
        auto future = m_future;
        return future.wait_for(timeout) == std::future_status::ready;
    }

private:
    /// The promise that will be set later asynchronously.
    std::promise<void> m_promise;

    /// The future object based from the @c m_promise.
    std::shared_future<void> m_future;
};

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PROMISEFUTUREPAIR_H_