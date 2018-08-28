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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_CBLAUTHREQUESTERINTERFACE_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_CBLAUTHREQUESTERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace authorization {
namespace cblAuthDelegate {

/**
 * Interface for receiving requests for authorization from a CBLAuthDelegate.
 */
class CBLAuthRequesterInterface {
public:
    /**
     * Notification of a request for authorization.
     *
     * @param url The URL that the user needs to navigate to.
     * @param code The code that the user needs to enter once authorized.
     */
    virtual void onRequestAuthorization(const std::string& url, const std::string& code) = 0;

    /**
     * Notification that we are polling @c LWA to see if the client has been authorized.
     */
    virtual void onCheckingForAuthorization() = 0;

    /**
     * Destructor.
     */
    virtual ~CBLAuthRequesterInterface() = default;
};

}  // namespace cblAuthDelegate
}  // namespace authorization
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_CBLAUTHREQUESTERINTERFACE_H_
