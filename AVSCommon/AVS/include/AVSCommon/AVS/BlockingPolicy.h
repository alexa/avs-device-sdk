/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_BLOCKINGPOLICY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_BLOCKINGPOLICY_H_

#include <iostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/// Enumeration of 'blocking policy' values to be associated with @c AVSDirectives.
enum class BlockingPolicy {
    /**
     * Handling of an @c AVSDirective with this @c BlockingPolicy does NOT block the handling of
     * subsequent @c AVSDirectives.
     */
    NON_BLOCKING,

    /**
     * Handling of an @c AVSDirective with this @c BlockingPolicy blocks the handling of subsequent @c AVSDirectives
     * that have the same @c DialogRequestId.
     */
    BLOCKING,

    /**
     * Handling of an @c AVSDirective with this @c BlockingPolicy is done immediately and does NOT block the handling of
     * subsequent @c AVSDirectives.
     */
    HANDLE_IMMEDIATELY,

    /**
     * BlockingPolicy not specified.
     */
    NONE
};

/**
 * Write a @c BlockingPolicy value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param policy The policy value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, BlockingPolicy policy) {
    switch (policy) {
        case BlockingPolicy::NON_BLOCKING:
            stream << "NON_BLOCKING";
            break;
        case BlockingPolicy::BLOCKING:
            stream << "BLOCKING";
            break;
        case BlockingPolicy::HANDLE_IMMEDIATELY:
            stream << "HANDLE_IMMEDIATELY";
            break;
        case BlockingPolicy::NONE:
            stream << "NONE";
            break;
    }
    return stream;
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_BLOCKINGPOLICY_H_
