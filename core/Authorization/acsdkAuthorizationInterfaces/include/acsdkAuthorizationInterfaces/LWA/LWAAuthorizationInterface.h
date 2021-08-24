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

#ifndef ACSDKAUTHORIZATIONINTERFACES_LWA_LWAAUTHORIZATIONINTERFACE_H_
#define ACSDKAUTHORIZATIONINTERFACES_LWA_LWAAUTHORIZATIONINTERFACE_H_

#include <memory>

#include <acsdkAuthorizationInterfaces/AuthorizationInterface.h>
#include "CBLAuthorizationObserverInterface.h"

namespace alexaClientSDK {
namespace acsdkAuthorizationInterfaces {
namespace lwa {

/**
 * This adapter provides mechanisms to authorize with LWA.
 */
class LWAAuthorizationInterface : public acsdkAuthorizationInterfaces::AuthorizationInterface {
public:
    /// Destructor.
    virtual ~LWAAuthorizationInterface() = default;

    /**
     * Authorize using LWA and the Code Based Linking method.
     *
     * @param observer The CBL observer which will receive a callback to display information back to the user.
     * This will be be sent in an @c CBLAuthorizationObserverInterface callback.
     */
    virtual bool authorizeUsingCBL(const std::shared_ptr<CBLAuthorizationObserverInterface>& observer) = 0;

    /**
     * Authorize using LWA and the Code Based Linking method. This API additionally requets customer profile information
     * (name, email).
     *
     * @attention THE USER MUST CONSENT (EX. UI DIALOG) BEFORE APPLICATION CAN REQUEST CUSTOMER PROFILE INFORMATION.
     *
     * @param observer The CBL observer which will receive a callback to display information back to the user.
     * This will be sent in an @c CBLAuthorizationObserverInterface callback.
     */
    virtual bool authorizeUsingCBLWithCustomerProfile(
        const std::shared_ptr<CBLAuthorizationObserverInterface>& observer) = 0;
};

}  // namespace lwa
}  // namespace acsdkAuthorizationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATIONINTERFACES_LWA_LWAAUTHORIZATIONINTERFACE_H_
