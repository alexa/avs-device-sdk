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

#include <bitset>
#include <iostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
/**
 * A @c Blocking policy is a way to express what mediums are required by the policy owner
 * and whether the policy owner is blocking subsequent directives using the medium.
 */
class BlockingPolicy {
public:
    /**
     * A policy medium represents a resource the policy owner is using.
     */
    struct Medium {
        /// The medium enum.
        enum MediumEnum {
            /// Audio Medium.
            AUDIO,
            /// Visual Medium.
            VISUAL,
            /// Number of mediums. This MUST be the last enum member.
            COUNT
        };
    };

    /// The mediums used by the policy owner
    using Mediums = std::bitset<Medium::COUNT>;

    /// Policy uses @c AUDIO medium.
    static const Mediums MEDIUM_AUDIO;

    /// Policy uses @c VISUAL medium.
    static const Mediums MEDIUM_VISUAL;

    /// Policy uses @c AUDIO and @c VISUAL mediums.
    static const Mediums MEDIUMS_AUDIO_AND_VISUAL;

    /**
     * Policy uses no medium.
     * This should be used for System of setting-type directives.
     */
    static const Mediums MEDIUMS_NONE;

    /**
     * Constructor
     *
     * @param mediums The @c Mediums used by the policy owner.
     * @param isBlocking Should this policy block another usage of owned mediums until completion.
     */
    BlockingPolicy(const Mediums& mediums = MEDIUMS_NONE, bool isBlocking = true);

    /**
     * Is the policy valid.
     *
     * @return @c true if the policy is valid, @c false otherwise.
     */
    bool isValid() const;

    /**
     * Is this policy blocking a @c Medium.
     * @return @c true if the given policy is blocking, @c false otherwise.
     */
    bool isBlocking() const;

    /**
     * What @c Mediums are used by this policy.
     *
     * @return The @c Mediums used by the policy
     */
    Mediums getMediums() const;

private:
    /// The mediums used by the policy owner.
    Mediums m_mediums;

    /// Is this policy blocking other users of its mediums.
    bool m_isBlocking;
};

/**
 * Write a @c BlockingPolicy value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param policy The policy value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const BlockingPolicy& policy) {
    stream << " Mediums:";
    auto mediums = policy.getMediums();
    if (BlockingPolicy::MEDIUM_AUDIO == mediums) {
        stream << "MEDIUM_AUDIO";
    } else if (BlockingPolicy::MEDIUM_VISUAL == mediums) {
        stream << "MEDIUM_VISUAL";
    } else if (BlockingPolicy::MEDIUMS_AUDIO_AND_VISUAL == mediums) {
        stream << "MEDIUMS_AUDIO_AND_VISUAL";
    } else if (BlockingPolicy::MEDIUMS_NONE == mediums) {
        stream << "MEDIUMS_NONE";
    } else {
        stream << "Unknown";
    }

    stream << policy.getMediums() << " .isBlocking:" << (policy.isBlocking() ? "True" : "False");

    return stream;
}

/**
 * Equal-to operator comparing two @c BlockingPolicys
 *
 * @param lhs The left side argument.
 * @param rhs The right side argument.
 * @return true if @c lhs and @c rhs are equal
 */
bool operator==(const BlockingPolicy& lhs, const BlockingPolicy& rhs);

/**
 * Not Equal-to operator comparing two @c BlockingPolicys *
 *
 * @param lhs The left side argument.
 * @param rhs The right side argument.
 * @return true if @c lhs and @c rhs are not equal
 */
bool operator!=(const BlockingPolicy& lhs, const BlockingPolicy& rhs);

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_BLOCKINGPOLICY_H_
