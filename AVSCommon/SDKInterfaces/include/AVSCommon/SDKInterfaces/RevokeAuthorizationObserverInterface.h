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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_REVOKEAUTHORIZATIONOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_REVOKEAUTHORIZATIONOBSERVERINTERFACE_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
/**
 * A RevokeAuthorizationObserverInterface is an interface class that clients can extend to receive revoke authorization
 * requests.
 */
class RevokeAuthorizationObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~RevokeAuthorizationObserverInterface() = default;

    /**
     * Used to notify the observer of revoke authorization requests. The client is expected to clear access tokens and
     * any registration information the client may hold. It should put itself back into the registration flow, if
     * appropriate for the device (Alexa For Business devices).
     */
    virtual void onRevokeAuthorization() = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_REVOKEAUTHORIZATIONOBSERVERINTERFACE_H_
