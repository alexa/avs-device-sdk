/*
 * AuthObserverInterface.h
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_AUTH_OBSERVER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_AUTH_OBSERVER_INTERFACE_H_

namespace alexaClientSDK {
namespace acl {

/**
 * This interface is used to observe changes to the state of authorization.
 */
class AuthObserverInterface {
public:
    /// The enum State describes the state of authorization.
    enum class State {
        /// Authorization not yet acquired.
        UNINITIALIZED,
        /// Authorization has been refreshed.
        REFRESHED,
        /// Authorization has expired.
        EXPIRED,
        /// Authorization failed in a manner that cannot be corrected by retry.
        UNRECOVERABLE_ERROR
    };

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~AuthObserverInterface() = default;

    /**
     * Notification that an authorization state has changed.
     *
     * @param newState The new state of the authorization token.
     */
    virtual void onAuthStateChange(State newState) = 0;
};

} // namespace acl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_AUTH_OBSERVER_INTERFACE_H_


