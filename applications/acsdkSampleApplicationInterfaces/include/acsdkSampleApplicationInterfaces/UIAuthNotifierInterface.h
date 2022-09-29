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

#ifndef ACSDKSAMPLEAPPLICATIONINTERFACES_UIAUTHNOTIFIERINTERFACE_H_
#define ACSDKSAMPLEAPPLICATIONINTERFACES_UIAUTHNOTIFIERINTERFACE_H_

#include <string>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>

namespace alexaClientSDK {
namespace acsdkSampleApplicationInterfaces {

/*
 * Contract to notify user interface about relevant authorization state changes.
 */
class UIAuthNotifierInterface {
public:
    /**
     * Destructor.
     */
    virtual ~UIAuthNotifierInterface() = default;

    /**
     * Notify user interface about the authorization request. The request url and code will be used by the user to
     * authorize.
     *
     * @param url The URL that the user needs to navigate to.
     * @param code The code that the user needs to enter once authorized.
     */
    virtual void notifyAuthorizationRequest(const std::string& url, const std::string& code) = 0;

    /**
     * Notify authorization state changes to GUI.
     *
     * @param state Changed authorization state.
     */
    virtual void notifyAuthorizationStateChange(avsCommon::sdkInterfaces::AuthObserverInterface::State state) = 0;
};

}  // namespace acsdkSampleApplicationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKSAMPLEAPPLICATIONINTERFACES_UIAUTHNOTIFIERINTERFACE_H_
