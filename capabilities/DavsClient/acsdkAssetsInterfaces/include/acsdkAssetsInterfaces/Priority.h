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

#ifndef ACSDKASSETSINTERFACES_PRIORITY_H_
#define ACSDKASSETSINTERFACES_PRIORITY_H_

namespace alexaClientSDK {
namespace acsdkAssets {
namespace commonInterfaces {

/**
 * The priority given to an artifact that determines the order of its deletion when space is low.
 * ACTIVE and PENDING_ACTIVATION are never to be deleted.
 */
enum class Priority { ACTIVE, PENDING_ACTIVATION, LIKELY_TO_BE_ACTIVE, UNUSED_IMPORTANT, UNUSED };

inline bool isValidPriority(int value) {
    return value >= static_cast<int>(Priority::ACTIVE) && value <= static_cast<int>(Priority::UNUSED);
}

inline std::string toString(Priority priority) {
    switch (priority) {
        case Priority::ACTIVE:
            return "ACTIVE";
        case Priority::PENDING_ACTIVATION:
            return "PENDING_ACTIVATION";
        case Priority::LIKELY_TO_BE_ACTIVE:
            return "LIKELY_TO_BE_ACTIVE";
        case Priority::UNUSED_IMPORTANT:
            return "UNUSED_IMPORTANT";
        case Priority::UNUSED:
            return "UNUSED";
    }
    return "";
}

inline std::ostream& operator<<(std::ostream& os, Priority value) {
    return os << toString(value);
}

}  // namespace commonInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSINTERFACES_PRIORITY_H_
