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

#ifndef ACSDKAUTHORIZATIONINTERFACES_AUTHORIZATIONADAPTERINTERFACE_H_
#define ACSDKAUTHORIZATIONINTERFACES_AUTHORIZATIONADAPTERINTERFACE_H_

#include <memory>
#include <string>

#include <acsdkAuthorizationInterfaces/AuthorizationInterface.h>
#include <acsdkAuthorizationInterfaces/AuthorizationManagerInterface.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>

namespace alexaClientSDK {
namespace acsdkAuthorizationInterfaces {

class AuthorizationManagerInterface;

/**
 * The @c AuthorizationAdapterInterface is an abstraction for an authorization mechanism. This interface will be
 * used for the @c AuthorizationManagerInterface to communicate. The application will not need to call these
 * methods directly. The class provides methods to query the current state and token information. Additionally, control
 * methods are provided to instruct the object when to reset().
 *
 * The other side of this communication happens using the @c AuthorizationManagerInterface::reportStateChange()
 * method. Once the manager is ready to receive messages, expect the
 * @c AuthorizationManagerInterface::onManagerReady() callback. This will pass an instance of the manager
 * that is to be used for further interactions.
 */
class AuthorizationAdapterInterface {
public:
    /// Destructor.
    virtual ~AuthorizationAdapterInterface() = default;

    /**
     * Returns the auth token if authorized, otherwise returns an empty string.
     *
     * @return The auth token or an empty string.
     */
    virtual std::string getAuthToken() = 0;

    /**
     * Logs out and clears the data within this adapter. This should not initiate a device wide deregistration,
     * that will be handled by the @c AuthorizationOrchestratorInterface. If this is called, the adapter must stop
     * any ongoing authorization.
     */
    virtual void reset() = 0;

    /**
     * Indicates if the authToken returned in @c AuthorizationAdapterInterface::getAuthToken() is invalid.
     * If the adapter is authorized, this should cause a reportStateChange() call
     * (please refer to function documentation for further details), and attempt to obtain a valid access token.
     *
     * If the adapter is not authorized, this call should be ignored.
     */
    virtual void onAuthFailure(const std::string& authToken) = 0;

    /**
     * Returns the current state of the @c AuthorizationAdapterInterface.
     *
     * @return The state.
     */
    virtual avsCommon::sdkInterfaces::AuthObserverInterface::FullState getState() = 0;

    /**
     * Returns the associated application facing @c AuthorizationInterface. This must never return null, as it is used
     * by @c AuthorizationManagerInterface to further retrieve the unique identifier for the
     * @c AuthorizationAdapterInterface instance.
     *
     * @return The @c AuthorizationInterface instance.
     */
    virtual std::shared_ptr<AuthorizationInterface> getAuthorizationInterface() = 0;

    /**
     * Sets the @c AuthorizationManagerInterface and communicates to the adapter the manager is now ready to receive
     * messages. Adapters must ensure that no @c AuthorizationManagerInterface calls are made in this callback as the
     * implementation makes no guarantee about re-entrancy.
     *
     * @param AuthorizationManagerInterface manager
     * @return The current state of the adapter.
     */
    virtual avsCommon::sdkInterfaces::AuthObserverInterface::FullState onAuthorizationManagerReady(
        const std::shared_ptr<AuthorizationManagerInterface>& manager) = 0;
};

}  // namespace acsdkAuthorizationInterfaces
}  // namespace alexaClientSDK

#endif  //  ACSDKAUTHORIZATIONINTERFACES_AUTHORIZATIONADAPTERINTERFACE_H_
