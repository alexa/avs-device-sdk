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

#ifndef ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_GATEWAYVERIFYSTATE_H_
#define ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_GATEWAYVERIFYSTATE_H_

#include <string>

namespace alexaClientSDK {
namespace avsGatewayManager {

/**
 * Structure used to store Gateway Verification state.
 */
struct GatewayVerifyState {
    /// The AVS Gateway URL string ex:
    std::string avsGatewayURL;
    /// Flag to indicate if the AVS gateway has been verified using the verify gateway sequence.
    bool isVerified;

    /**
     * Constructor.
     * @param gatewayURL The AVS Gateway URL string.
     * @param isVerified The flag indicating if the AVS Gateway has been verified.
     */
    GatewayVerifyState(const std::string& gatewayURL, bool isVerified = false);
};

inline GatewayVerifyState::GatewayVerifyState(const std::string& gatewayURL, bool isVerified) :
        avsGatewayURL{gatewayURL},
        isVerified{isVerified} {
}

}  // namespace avsGatewayManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_GATEWAYVERIFYSTATE_H_
