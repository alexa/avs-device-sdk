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

#ifndef ACSDKAUTHORIZATIONINTERFACES_AUTHORIZATIONAUTHORITYINTERFACE_H_
#define ACSDKAUTHORIZATIONINTERFACES_AUTHORIZATIONAUTHORITYINTERFACE_H_

#include <memory>
#include <string>

#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <RegistrationManager/RegistrationManagerInterface.h>

namespace alexaClientSDK {
namespace acsdkAuthorizationInterfaces {

/**
 * This interface is a centralized object that acts as the source of truth for authorization state in the AVS SDK. This
 * acts as:
 *
 * -Source of truth of the current active authorization method
 * -Source of truth of the access token (by obtaining from the active authorization method)
 * -Source of truth of the current authorization state of the AVS SDK
 * -Controller of logout across the SDK including deauthorization and clearing of user data.
 */
class AuthorizationAuthorityInterface
        : public avsCommon::sdkInterfaces::AuthDelegateInterface
        , public registrationManager::RegistrationManagerInterface {
public:
    /// Destructor.
    virtual ~AuthorizationAuthorityInterface() = default;

    /**
     * Returns the current authorization state.
     *
     * @return The current authorization state.
     */
    virtual avsCommon::sdkInterfaces::AuthObserverInterface::State getState() = 0;

    /**
     * Returns the string identifying the current active @c AuthorizationInterface.
     * An active adapter can be in the process of obtaining authorization tokens.
     *
     * @return The active authorization.
     */
    virtual std::string getActiveAuthorization() = 0;
};

}  // namespace acsdkAuthorizationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATIONINTERFACES_AUTHORIZATIONAUTHORITYINTERFACE_H_
