/*
 * ThreadIdAsString.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSUTILS_INCLUDE_AVSUTILS_LOGGER_THREAD_ID_AS_STRING_H_
#define ALEXA_CLIENT_SDK_AVSUTILS_INCLUDE_AVSUTILS_LOGGER_THREAD_ID_AS_STRING_H_

#include <string>

namespace alexaClientSDK {
namespace avsUtils {
namespace logger {

/// Class to provide 'this_thread' access to a pre-rendered string representing its thread ID.
class ThreadIdAsString {
public:
    /// Construct an instance of ThreadIdAsString.
    ThreadIdAsString();

    /**
     * Get the current threads ID as a string.
     * @return The instance represented as a string.
     */
    static inline const std::string &getThisThreadIdAsAString();

private:
    /// The current thread's ID rendered as a string.
    std::string m_string;

    /// Per-thread static instance so that m_instance.m_string represents 'this_thread's ID.
    static thread_local ThreadIdAsString m_instance;
};

const std::string &ThreadIdAsString::getThisThreadIdAsAString() {
    return m_instance.m_string;
}

} // namespace loger
} // namespace avsUtils
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_AVSUTILS_INCLUDE_AVSUTILS_LOGGER_THREAD_ID_AS_STRING_H_

