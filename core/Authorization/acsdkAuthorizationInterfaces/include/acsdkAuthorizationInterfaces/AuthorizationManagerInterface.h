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

#ifndef ACSDKAUTHORIZATIONINTERFACES_AUTHORIZATIONMANAGERINTERFACE_H_
#define ACSDKAUTHORIZATIONINTERFACES_AUTHORIZATIONMANAGERINTERFACE_H_

#include <memory>
#include <string>

#include <acsdkAuthorizationInterfaces/AuthorizationAdapterInterface.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>

namespace alexaClientSDK {
namespace acsdkAuthorizationInterfaces {

class AuthorizationAdapterInterface;

/**
 * This non-application facing interfaces manages between multiple authorization mechanisms and ensures that the
 * AVS SDK utilizes a single authorization mode.
 */
class AuthorizationManagerInterface {
public:
    /// Destructor.
    virtual ~AuthorizationManagerInterface() = default;

    /**
     * Reports the state change. This should be called in accordance with state transitions documented in
     * @c AuthObserverInterface::State. UserId may not initially be available in the AUTHORIZING state. The values
     * obtained in REFRESHED will be carried forward for future callbacks for validation purposes.
     *
     * @param state The state.
     * @param authId The unique auth id.
     * @param userId The user id associated with this.
     */
    virtual void reportStateChange(
        const avsCommon::sdkInterfaces::AuthObserverInterface::FullState& state,
        const std::string& authId,
        const std::string& userId) = 0;

    /**
     * Add an adapter with the @c AuthorizationManagerInterface. This must be called before @c reportStateChange.
     *
     * @param adapter The authorization adapter.
     */
    virtual void add(const std::shared_ptr<acsdkAuthorizationInterfaces::AuthorizationAdapterInterface>& adapter) = 0;
};

}  // namespace acsdkAuthorizationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATIONINTERFACES_AUTHORIZATIONMANAGERINTERFACE_H_
