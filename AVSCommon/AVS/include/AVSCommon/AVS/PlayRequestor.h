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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_PLAYREQUESTOR_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_PLAYREQUESTOR_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * Contains the values for the optional playRequestor object that is found in the @c Play directive for the various
 * players available
 */
struct PlayRequestor {
    /// The playRequestor type. In the case of "ALERT" as the type, that means AudioPlayer is playing a music
    /// alarm.
    std::string type;

    /// The playRequestor id. In the case of "ALERT" as the type, this id will be the same as the "token" parameter
    /// in @c SetAlert directive.
    std::string id;
};

inline bool operator==(const PlayRequestor& playRequestorA, const PlayRequestor& playRequestorB) {
    return playRequestorA.type == playRequestorB.type && playRequestorA.id == playRequestorB.id;
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_PLAYREQUESTOR_H_
