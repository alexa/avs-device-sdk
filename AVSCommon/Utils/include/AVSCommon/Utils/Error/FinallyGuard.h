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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ERROR_FINALLYGUARD_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ERROR_FINALLYGUARD_H_

#include <functional>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace error {

/**
 * Define a class that can be used to run a function when the object goes out of scope.
 *
 * This simulates try-finally statements. The following structure:
 *
 * @code
 * try {
 *   <try_block>
 * } finally {
 *   <finally_block>
 * }
 * @endcode
 *
 * can be replaced by:
 *
 * @code
 *   FinallyGuard guard { []{ <finally_block> }};
 *   <try_block>
 * @endcode
 */
class FinallyGuard {
public:
    /**
     * Constructor.
     *
     * @param finallyFunction The function to be executed when the object goes out of scope.
     */
    FinallyGuard(const std::function<void()>& finallyFunction);

    /**
     * Destructor. Runs @c m_function during destruction.
     */
    ~FinallyGuard();

private:
    /// The function to be run when this object goes out of scope.
    std::function<void()> m_function;
};

FinallyGuard::FinallyGuard(const std::function<void()>& finallyFunction) : m_function{finallyFunction} {
}

FinallyGuard::~FinallyGuard() {
    if (m_function) {
        m_function();
    }
}

}  // namespace error
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ERROR_FINALLYGUARD_H_
